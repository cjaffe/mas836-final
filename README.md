# MAS.836 Final Project - Bicycle-Mounted Road Quality Sensor

# Key Folders and Files:

Boards: Boards designed in EAGLE for the main microcontroller board and the breakout boards for the piezo films (with non-inverting amplifiers)

Code: All code for the project
- main-bike-sensor.c: This is the program that runs on the ATTiny44 chip; it uses a bit-banging implementation of I2C to communicate with the accelerometer, reads from 2 ADC registers to get data from the piezo films, and uses a software implementation of serial communication to send data over serial/ Bluetooth. This code is adapted from Neil Gershenfeld (http://academy.cba.mit.edu/classes/input_devices/accel/hello.MXD6235MP.c, http://academy.cba.mit.edu/classes/input_devices/temp/hello.temp.45.c)

- bike-sensor.make: Makefile for main-bike-sensor.c; defines settings including processor type (ATTiny44) and processor speed (8hz, though scaled in software to be 1Mhz)

- app.js: Main server-side script that scans for BLE peripherals (using noble), initializes a virtual serial port (using virtual serial-port), and sends data through a websocket to the client (using SocketIO)

Code/static: Client-side HTML, CSS & Javascript
- index.html: Main HTML page for front-end data visualization; includes a script that receives data via SocketIO and processes it (e.g. combining low and high bits, enters data into relevant data variables)

- bikeSensor.js: Main D3 visualization script

- neighborhoods.js and streets.js: GeoJSON of Cambridge, MA neighborhoods and streets, from the Cambridge City GIS page (https://github.com/cambridgegis/cambridgegis_data)

Code/node_modules: node modules and Javascript libraries used in this project, including noble, serial-to-socketio, and virtual-serialport

Data: Sample of collected data that was visualized offline
- bumpy-road and smooth-road: data files saved via the "Download Data" functionality, contain comma-separated values of each of the five measurements received over serial/ BLE 

- road-quality-vis: Excel workbook with data from bumpy-road and smooth-road that has been manually sanitized and visualized in basic scatter plots
