var id;

/**
 * Options for location determination
 */
var locationOptions = {
    enableHighAccuracy: true,
    maximumAge:0,
    timeout: 50
};

/**
 * called when we successfuly determine our location
 */
function locationSuccess(pos) {
    console.log('Location Changed!');
    console.log(JSON.stringify(pos));
    sendLoc(pos);
};

/**
 * called on failure to determine location.
 */
function locationError(err) {
    console.log('location error: '+err.message);
};

/**
 * called on successful transmission of mession to pebble.
 */
var msgSendOK = function(e) {
    console.log('Delivered Message');
};

/**
 * called on failure to send message to pebble
 */
var msgSendFailed = function(e) {
    console.log('Unable to Deliver Message');
};

/**
 * Send location information to pebble
 */
function sendLoc(pos) {
    console.log("sendLoc - pos = "+ JSON.stringify(pos));
    // Send the location as strings (because of limitation of appmessage types)

    var dnow = new Date(pos.timestamp);

    locObj = {"timestamp":pos.timestamp,
              "day": dnow.getDate(),
              "mon": dnow.getMonth()+1,
              "year": dnow.getYear()+1900,
              "hour": dnow.getHours(),
              "min": dnow.getMinutes(),
	      "sec": dnow.getSeconds(),
	      "lat":parseInt(1000*pos.coords.latitude),
	      "lon":parseInt(1000*pos.coords.longitude),
	      "speed":parseInt(1000*pos.coords.speed),
	      "heading":pos.coords.heading,
	      "alt":pos.coords.altitude,
	      "acc":pos.coords.accuracy,
	      "alt_acc":pos.coords.altitudeAccuracy
	     };

    if (locObj.heading == null) { console.log("zeroing heading"); locObj.heading = 0; }
    if (locObj.alt_acc == null) { console.log("zeroing alt_acc"); locObj.alt_acc = 0; }


    console.log('Sending Message:' 
		+ JSON.stringify(locObj));
    

    var transactionId = Pebble.sendAppMessage(locObj,
					      msgSendOK,
					      msgSendFailed
					     );
}


/**
 * This is main program - called when the watch app first starts.
 */
Pebble.addEventListener("ready",
    function(e) {
        console.log("Pacer JS Component Started.");
	console.log("Subscribing to Location Updates");
	id = navigator.geolocation.watchPosition(
	    locationSuccess,
	    locationError,
	    locationOptions);


	var samplePos = {};
	var dnow = new Date();
	samplePos.timestamp = dnow;
	samplePos.coords={};
	samplePos.coords.latitude = 54.4;
	samplePos.coords.longitude = -1.23;
	samplePos.coords.speed = 0;
	samplePos.coords.heading = 0;
	samplePos.alt = 0;
	samplePos.acc = 999;
	samplePos.altitudeAccuracy = 999;

	console.log("samplePos = "+JSON.stringify(samplePos));
	sendLoc(samplePos);
		    
    }
);


Pebble.addEventListener('appmessage',
			function(e) {
			    console.log('Received Message:' 
					+ JSON.stringify(e.payload));
			}
		       );
