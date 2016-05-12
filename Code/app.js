/*
 *
 * Uses serial-to-socketio, virtual serialport, and noble to read data from 
 * BLE peripheral device into browser
 *
 * Adapted from examples
 */

var express = require('./node_modules/ser-to-socket/node_modules/express');
var app = express();
var http = require('http').Server(app);
var io = require('./node_modules/ser-to-socket/node_modules/socket.io')(http);
var serialPort = require('./node_modules/virtual-serialport');//require("./node_modules/ser-to-socket/node_modules/serialport");
var noble = require('./node_modules/noble/index');

var deviceUUID = 'ffe0';
var serviceUUID = 'ffe0';
var characUUID = 'ffe1';

// Prompt for virtual serial port name
if (process.argv.length != 3) {
    console.log("usage: \"node app.js <serial_port>\"");
    process.exit(0);
}

//Use noble to start scanning for BLE peripherals
noble.on('stateChange', function(state) {
  if (state === 'poweredOn') {
    //
    // Once the BLE radio has been powered on, it is possible
    // to begin scanning for services. Pass an empty array to
    // scan for all services (uses more time and power).
    //
    console.log('scanning...');
    noble.startScanning([deviceUUID], false);
  }
  else {
    noble.stopScanning();
  }
});

var readCharac = null;

// Look at the services and characteristics available in that peripheral
noble.on('discover', function(peripheral) {
  // we found a peripheral, stop scanning
  noble.stopScanning();

  //
  // The advertisment data contains a name, power level (if available),
  // certain advertised service uuids, as well as manufacturer data,
  // which could be formatted as an iBeacon.
  //
  console.log('found peripheral:', peripheral.advertisement);
  //
  // Once the peripheral has been discovered, then connect to it.
  // It can also be constructed if the uuid is already known.
  ///
  peripheral.connect(function(err) {
    //
    // Once the peripheral has been connected, then discover the
    // services and characteristics of interest.
    //
    peripheral.discoverServices([serviceUUID], function(err, services) {
      services.forEach(function(service) {
        //
        // This must be the service we were looking for.
        //
        console.log('found service:', service.uuid);

        //
        // So, discover its characteristics.
        //
        service.discoverCharacteristics([characUUID], function(err, characteristics) {
          console.log("characteristics: "+characteristics);
          characteristics.forEach(function(characteristic) {
            //
            // Loop through each characteristic and match them to the
            // UUIDs that we know about.
            //
            console.log('found characteristic:', characteristic.uuid);

            if (characUUID == characteristic.uuid) {
              readCharac = characteristic;
            }
          });

          //
          // Check to see if we found all of our characteristics.
          //
           if (readCharac) {
            // console.log("found characteristic"); //DEBUG
            readCharac.read(function(err, data) {
              console.log(data);
            }); // this first read probably not strictly necessary 

            // Subscribe to updates
            readCharac.notify(true, function(err) {
              if (err) {
                console.log('notify error');
              }
            });

            readCharac.on('read', function(data, isNotification) {
              // console.log('Just read from characteristic'); //DEBUG
              // console.log(data); // DEBUG
              var length = data.length;

              // SEND TO VIRTUAL SERIAL PORT HERE
              for (var i= 0; i < length; i++) {
                var val = data.slice(i,i+1).readUInt8();
                serialConnection.writeToComputer(val);
                // console.log("just wrote: "+ val); //DEBUG
              }
            });
          
   
          }
          else {
            console.log('missing characteristics');
          }
        });
      });
    });
  });
});

//setup expressjs to serve static files from the static directory and redirect root to index.html
app.use("/", express.static(__dirname + "/static"));
app.get("/", function (req, res) {
    res.redirect('/index.html');
});

//start express
http.listen(3000, function () {
    console.log('go to http://127.0.0.1:3000');
});

//initialize socketui connections
io.on('connection', function(socket){
    //relay socket.io writes to the serial port
    socket.on('data', function(data){
        serialConnection.write(data);
    });
});

//initialize serial connection with a single byte parser
 var serialConnection = new serialPort(process.argv[2]);//, {
//     parser: serialPort.parsers.byteLength(1),
//     baudrate: 9600
// });

//on data callback broadcast to the default socketio connection
serialConnection.on("open", function () {
    serialConnection.on('data', function (data) {
        // console.log("socket just received: "+data); // DEUB
        io.emit("data", data);
    });
});

//error handling
serialConnection.on("error", function () {
    console.error("Can't establish serial connection with " + process.argv[2]);
    process.exit(1);
});