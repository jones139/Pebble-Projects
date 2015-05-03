/**
  pacer - running pacemaker app.

  Copyright Graham Jones, 2015.


  Pacer is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Pacer is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with pebble_sd.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <pebble.h>

static Window *window;
static TextLayer *msg_layer;
static TextLayer *top_layer;
static TextLayer *bot_layer;

#define APPSTATE_STOPPED 0
#define APPSTATE_RUNNING 1
#define APPSTATE_DISTSET 2
#define APPSTATE_TIMESET 3

#define DEFAULT_DISTNO 7  // Default distance is 5000m
#define DEFAULT_PACE 5 // min/km.

/** Keys for the location update messages */
#define KEY_TIMESTAMP 10
#define KEY_LAT 11
#define KEY_LON 12
#define KEY_SPEED 13
#define KEY_HEADING 14
#define KEY_ALT 15
#define KEY_ACC 16
#define KEY_ALT_ACC 17
#define KEY_DATETIME 18
#define KEY_DAY 19
#define KEY_MON 20
#define KEY_YEAR 21
#define KEY_HOUR 22
#define KEY_MIN 23
#define KEY_SEC 24
#define KEY_COMMAND 25
#define KEY_START 26
#define KEY_STOP 27
#define KEY_RESET 28

/** A structure to hold position data received from phone
 */
typedef struct {
  time_t ts;  // timestamp (sec)
  long lat,lon, acc; // location and accuracy (deg and m).
  long speed,heading; // speed (m/s),heading
  long alt,alt_acc;  // altitude and altitude accuracy (m)
} Pos;

/** race distances in metres */
int distm[] = {100, 200, 400, 800, 1500, 1600, 3000, 5000, 10000};
char* distStr[] = {"100m","200m","400m","800m","1500m","1 mile","3000m","5000m","10000m","half marathon", "marathon"};
int nDists = 9;  // Number of distance options.


/** Application state variables */
int appState = APPSTATE_STOPPED;   // 0 = stopped, 1 = running, 2 = distance setting, 3 = time setting.
int distNo = DEFAULT_DISTNO;  // Index of the race distance in the distm and distStr arrays.
int targetTime = 0; // set in init to match default distance.
int currDist = 0;  // Current distance covered in metres.
int currTime = 0;  // Current elapsed time in seconds.
char msgStr[50];
time_t startTime = 0;  // Start time in seconds from start 1970.

Pos curPos;   // current position
Pos prevPos;   // previous position


/** 
 * Send a command to the phone
 */
void sendCommand(int command) {
  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);
  dict_write_int(iterator,KEY_COMMAND,&command,sizeof(int),true);
  app_message_outbox_send();
}


/*************************************************************
 * Communications with Phone
 *************************************************************/
void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  char strBuf[50];
  int day=0,mon=1,year=1900,hour=0,min=0,sec=0;
  struct tm tmstr;
  time_t dataTime;

  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  // Get the first pair
  Tuple *t = dict_read_first(iterator);

  // Process all pairs present
  while(t != NULL) {
    // Process this pair's key
    //APP_LOG(APP_LOG_LEVEL_INFO,"Key=%d",(int) t->key);
    switch (t->key) {
    case KEY_TIMESTAMP:
      curPos.ts = (long)t->value->uint32;
      strncpy(strBuf,t->value->cstring,sizeof(strBuf));
      APP_LOG(APP_LOG_LEVEL_INFO,"ts=%s",strBuf);
      //APP_LOG(APP_LOG_LEVEL_INFO,"curPos.ts = %lu",curPos.ts);
      break;
    case KEY_DAY:
      day = (int)t->value->int32;
      //APP_LOG(APP_LOG_LEVEL_INFO,"day = %d",day);
      break;
    case KEY_MON:
      mon = (int)t->value->int32;
      //APP_LOG(APP_LOG_LEVEL_INFO,"mon = %d",mon);
      break;
    case KEY_YEAR:
      year = (int)t->value->int32;
      //APP_LOG(APP_LOG_LEVEL_INFO,"year = %d",year);
      break;
    case KEY_HOUR:
      hour = (int)t->value->int32;
      //APP_LOG(APP_LOG_LEVEL_INFO,"hour = %d",hour);
      break;
    case KEY_MIN:
      min = (int)t->value->int32;
      //APP_LOG(APP_LOG_LEVEL_INFO,"min = %d",min);
      break;
    case KEY_SEC:
      sec = (int)t->value->int32;
      //APP_LOG(APP_LOG_LEVEL_INFO,"sec = %d",sec);
      break;
    case KEY_LAT:
      curPos.lat = (int)t->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO,"curPos.lat = %ld",curPos.lat);
      break;
    case KEY_LON:
      curPos.lon = (long)t->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO,"curPos.lon = %ld",curPos.lon);
      break;
    case KEY_ACC:
      curPos.acc = (long)t->value->uint32;
      APP_LOG(APP_LOG_LEVEL_INFO,"curPos.acc = %ld",curPos.acc);
      break;
    case KEY_SPEED:
      curPos.speed = (long)t->value->uint16;
      APP_LOG(APP_LOG_LEVEL_INFO,"curPos.speed = %ld",curPos.speed);
      break;
    case KEY_HEADING:
      curPos.heading = (long)t->value->int16;
      APP_LOG(APP_LOG_LEVEL_INFO,"curPos.heading = %ld",curPos.heading);
      break;
    case KEY_ALT:
      curPos.alt = (long)t->value->uint32;
      APP_LOG(APP_LOG_LEVEL_INFO,"curPos.alt = %ld",curPos.alt);
      break;
    case KEY_ALT_ACC:
      curPos.alt_acc = (long)t->value->uint32;
      APP_LOG(APP_LOG_LEVEL_INFO,"curPos.alt_acc = %ld",curPos.alt_acc);
      break;
    }
    // Get next pair, if any
    t = dict_read_next(iterator);
  }
  
  tmstr.tm_mday = day;
  tmstr.tm_mon = mon-1;
  tmstr.tm_year = year-1900;
  tmstr.tm_hour = hour;
  tmstr.tm_min = min;
  tmstr.tm_sec = sec;
  tmstr.tm_isdst = 0;

  APP_LOG(APP_LOG_LEVEL_INFO,"%02d/%02d/%04d %02d:%02d:%02d",
	  tmstr.tm_mday,tmstr.tm_mon,tmstr.tm_year,
	  tmstr.tm_hour,tmstr.tm_min,tmstr.tm_sec);

  // mktime crashes pebble - may be a bug in pebble c implementation?
  dataTime = 0;
  dataTime = mktime(&tmstr);
  APP_LOG(APP_LOG_LEVEL_INFO,"dataTime = %ld",dataTime);
  
}

void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}



/************************************************************************
 * clock_tick_handler() - 
 * This function is the handler for tick events.
*/
static void clock_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  static char s_time_buffer[16];
  static char s_alarm_buffer[64];
  static char s_buffer[256];
  static int analysisCount=0;

  text_layer_set_text(msg_layer, "PACER");

  // and update clock display.
  if (clock_is_24h_style()) {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M:%S", tick_time);
  } else {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%I:%M:%S", tick_time);
  }
  BatteryChargeState charge_state = battery_state_service_peek();
  snprintf(s_time_buffer,sizeof(s_time_buffer),
	   "%s %d%%", 
	   s_time_buffer,
	   charge_state.charge_percent);
  text_layer_set_text(bot_layer, s_time_buffer);
}



/**
 * Reset the pacer app data
 */
void reset_pacer() {
  startTime = 0;
  currDist = 0;
  currTime = 0;
  sendCommand(KEY_RESET);
}

/**
 * Start the pacer app running
 */
void start_pacer() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"start_pacer()");
  startTime = time(NULL);
  sendCommand(KEY_START);
}

/** 
 * Stop the pacer app running
 */
void stop_pacer() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"stop_pacer()");
  sendCommand(KEY_STOP);
}


/**
 * Display the User Interface
 * what is displayed depends on the application state stored in appState.
 */
static void displayUI() {
  char strBuf[100];

  text_layer_set_text(msg_layer, msgStr);      

  if (appState == APPSTATE_RUNNING) {
    snprintf(strBuf,sizeof(strBuf),"RUNNING");
  } else if (appState == APPSTATE_STOPPED) {
    snprintf(strBuf,sizeof(strBuf),"STOPPED");
  } else if (appState == APPSTATE_DISTSET) {
    snprintf(strBuf,sizeof(strBuf),"Distance: %s", distStr[distNo]);
  } else if (appState == APPSTATE_TIMESET) {
    snprintf(strBuf,sizeof(strBuf),"Target Time: %d min",targetTime);
  } 
  text_layer_set_text(top_layer, strBuf);      
}

/**
 * Simple function to write a string to the msgStr message buffer
 */
static void setMsg(char *msg) {
  snprintf(msgStr,sizeof(msgStr),"Msg=%s",msg);
}

/**
 * Handle the select (midle right) button
 */
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (appState == APPSTATE_STOPPED) {
    appState = APPSTATE_DISTSET;
    setMsg("SET DISTANCE");
  } else if (appState == APPSTATE_DISTSET) {
    appState = APPSTATE_TIMESET;
    setMsg("SET TIME");
  } else if (appState == APPSTATE_TIMESET) {
    appState = APPSTATE_STOPPED;
    setMsg("STOPPED");
  } 
  else {
    setMsg("SELECT IGNORED - RUNNING");
  }
  displayUI();
}

/**
 * Handle the Up (top right) button
 */
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (appState == APPSTATE_STOPPED) {
    start_pacer();
    appState = APPSTATE_RUNNING;
    setMsg("RUNNING");
  } else if (appState == APPSTATE_RUNNING) {
    stop_pacer();
    appState = APPSTATE_STOPPED;
    setMsg("STOPPED");
  } else if (appState == APPSTATE_DISTSET) {
    distNo ++;
    if (distNo == nDists) distNo = nDists-1;
    setMsg("DIST UP");
  } else if (appState == APPSTATE_TIMESET) {
    targetTime ++;
    setMsg("TIME UP");
  }
  displayUI();
}

/**
 * Handle the down (bottom right) button.
 */
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (appState == APPSTATE_STOPPED) {
    setMsg("RESET");
    reset_pacer();
  } else if (appState == APPSTATE_RUNNING) {
    setMsg("IGNORING - RUNNING");
  } else if (appState == APPSTATE_DISTSET) {
    setMsg("DISTANCE DOWN");
    distNo --;
    if (distNo<0) distNo = 0;
  } else if (appState == APPSTATE_TIMESET) {
    targetTime --;
    if (targetTime<0) targetTime=0;
    setMsg("TIME DOWN");
  }
  displayUI();
}

/**
 * Handle the back (top left) button
 */
static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (appState == APPSTATE_DISTSET || appState == APPSTATE_TIMESET) {
    appState = APPSTATE_STOPPED;
    setMsg("STOPPED");
  } else if (appState == APPSTATE_RUNNING) {
    setMsg("IGNORING BACK - RUNNING");
  } else {
    setMsg("BACK");
  }
  displayUI();
}

/**
 * Register us to receive button click events
 */
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

/**
 * Intialise the UI when the window is loaded.
 */
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  msg_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 20 } });
  layer_add_child(window_layer, text_layer_get_layer(msg_layer));

  top_layer = text_layer_create((GRect) { 
      .origin={ 0, 21 },
	.size = { bounds.size.w,60 } });
  bot_layer = text_layer_create((GRect) { 
      .origin={ 0, 81 },
	.size = { bounds.size.w,60 } });

  text_layer_set_text(top_layer, "Top Layer");
  text_layer_set_text_alignment(top_layer, GTextAlignmentCenter);
  text_layer_set_font(top_layer, 
		      fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_layer, text_layer_get_layer(top_layer));

  text_layer_set_text(bot_layer, "Bottom Layer");
  text_layer_set_text_alignment(bot_layer, GTextAlignmentCenter);
  text_layer_set_font(bot_layer, 
		      fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(window_layer, text_layer_get_layer(bot_layer));

}

/**
 * Destroy UI components when the window is destroyed
 */
static void window_unload(Window *window) {
  text_layer_destroy(msg_layer);
  text_layer_destroy(top_layer);
  text_layer_destroy(bot_layer);
}

/**
 * Initialise the whole watch app
 */
static void init(void) {
  // set initial target time based on default pace.
  targetTime = distm[distNo]*DEFAULT_PACE;
  // Register comms callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), 
		   app_message_outbox_size_maximum());

  // Create UI window
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  /* Subscribe to TickTimerService */
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Intialising Clock Timer....");
  tick_timer_service_subscribe(SECOND_UNIT, clock_tick_handler);

}

/**
 * De-initalise the app - destroy the window.
 */
static void deinit(void) {
  window_destroy(window);
}

/**
 * Main programme.
 */
int main(void) {
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  app_event_loop();
  deinit();
}
