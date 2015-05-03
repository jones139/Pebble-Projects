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
var Pacer = {};

Pacer.id = null;
Pacer.prefPos = null;

/**
 * Options for location determination
 */
Pacer.locationOptions = {
    enableHighAccuracy: true,
    maximumAge:0,
    timeout: 50
};

Pacer.radians = function(deg) {
    var pi = 3.14159265;
    return 2*pi*deg/360;
};

/**
 * Return distance in metres between two lat,lon points.
 * Based no http://www.movable-type.co.uk/scripts/latlong.html.
 * Expects pos to include {coords.latitude, coords.longitude}.
 */
Pacer.dist = function(pos1,pos2) {
    var lat1 = Pacer.radians(pos1.coords.latitude);
    var lon1 = Pacer.radians(pos1.coords.longitude);
    var lat2 = Pacer.radians(pos2.coords.latitude);
    var lon2 = Pacer.radians(pos2.coords.longitude);

    var R = 6371000; // metres
    var φ1 = lat1;
    var φ2 = lat2;
    var Δφ = (lat2-lat1);
    var Δλ = (lon2-lon1);
    
    var a = Math.sin(Δφ/2) * Math.sin(Δφ/2) +
        Math.cos(φ1) * Math.cos(φ2) *
        Math.sin(Δλ/2) * Math.sin(Δλ/2);
    var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));

    var d = R * c;
    return d;
};




/**
 * called when we successfuly determine our location
 */
Pacer.locationSuccess = function(pos) {
    //console.log('Location Changed!');
    //console.log(JSON.stringify(pos));
    console.log("Pacer.locationSuccess: dist="+Pacer.dist({coords:{latitude:50,longitude:0}},
			     {coords:{latitude:50,longitude:1}}));
    Pacer.sendLoc(pos);
};

/**
 * called on failure to determine location.
 */
Pacer.locationError = function(err) {
    console.log('location error: '+err.message);
};

/**
 * called on successful transmission of mession to pebble.
 */
Pacer.msgSendOK = function(e) {
    //console.log('Delivered Message');
};

/**
 * called on failure to send message to pebble
 */
Pacer.msgSendFailed = function(e) {
    console.log('Unable to Deliver Message');
};

/**
 * Send location information to pebble
 */
Pacer.sendLoc = function(pos) {
    //console.log("sendLoc - pos = "+ JSON.stringify(pos));
    // Send the location as strings (because of limitation of appmessage types)

    var dnow = new Date(pos.timestamp);

    var locObj = {"timestamp":pos.timestamp,
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

    if (locObj.heading == null) { 
	//console.log("zeroing heading"); 
	locObj.heading = 0; 
    }
    if (locObj.alt_acc == null) { 
	//console.log("zeroing alt_acc"); 
	locObj.alt_acc = 0; 
    }


    //console.log('Pacer.sendLoc() - Sending Message:' 
    //		+ JSON.stringify(locObj));
    

    var transactionId = Pebble.sendAppMessage(locObj,
					      Pacer.msgSendOK,
					      Pacer.msgSendFailed
					     );
};


Pacer.msgHandler = function(e) {
    if ("command" in e.payload) {
	console.log("Pacer.msgHandler: Command = "+e.payload.command);
    }
};

/**
 * This is main program - called when the watch app first starts.
 */
Pebble.addEventListener("ready",
    function(e) {
        console.log("Pacer JS Component Started.");
	console.log("Subscribing to Location Updates");
	id = navigator.geolocation.watchPosition(
	    Pacer.locationSuccess,
	    Pacer.locationError,
	    Pacer.locationOptions);


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
	Pacer.sendLoc(samplePos);
		    
    }
);



Pebble.addEventListener('appmessage',
			function(e) {
			    console.log('Received Message:' 
					+ JSON.stringify(e.payload));
			    Pacer.msgHandler(e);
			    }
		       );
