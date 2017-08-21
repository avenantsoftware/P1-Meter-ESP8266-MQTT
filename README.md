# P1-Meter-ESP8266
Software for the ESP8266 that sends P1 smart meter data to to MQTT on a configurable topic with json. Easy to consume with Pimatic, Telegraf or other software that supports MQTT. I use a nodemcu for this sketch.

### Installation instrucions
- Make sure that your ESP8266 can be flashed from the Arduino environnment: https://github.com/esp8266/Arduino
- Install the SoftSerial library from: https://github.com/plerup/espsoftwareserial
- Install the pubsub library from: https://github.com/knolleary/pubsubclient
- Edit pubsubclient.h to set the MQTT MAX MESSAGE SIZE to 512
- Place all files from this repository in a directory. Open the .ino file.
- Adjust WIFI, MQTT and debug settings at the top of the file
- Compile and flash as usual

### Connection of the P1 meter to the ESP8266
You need to connect the smart meter with a RJ12 connector. This is the pinout to use
![RJ11 P1 connetor](http://gejanssen.com/howto/Slimme-meter-uitlezen/RJ11-pinout.png)

Connect GND->GND on ESP, RTS->3.3V on ESP and RxD->any digital pin on ESP with a 10k ohm pull up resistor placed between the RTS/3.3v connection and the data connection. In this sketch I use D5 for the serial communication with the smartmeter.
