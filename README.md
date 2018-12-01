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

### Powering NodeMCU from P1 port (ESMR 5.0)
pin 1 = VCC  
pin 6 = GND  

connect pin 1 -> Vin on NodeMCU (bottom left)  
connect pin 6 -> GND on NodeMCU (bottom left)  

### Connecting to HomeAssistant (HASS)
In sensors.yaml
<pre>
- name: Tariff Low
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ (value_json.powerConsumptionLowTariff|float/1000) }}"
  unit_of_measurement: kWh

- name: Tariff High
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ (value_json.powerConsumptionHighTariff|float/1000) }}"
  unit_of_measurement: kWh
  
- name: Total
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ (((value_json.powerConsumptionLowTariff|float)+(value_json.powerConsumptionHighTariff|float))/1000) }}"
  unit_of_measurement: kWh

- name: Total2
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ (((value_json.powerConsumptionLowTariff|float)+(value_json.powerConsumptionHighTariff|float))/1000)|round(0) }}"
  unit_of_measurement: kWh

- name: Power Usage
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ ((value_json.CurrentPowerConsumption|float)|round(0)) }}"
  unit_of_measurement: W

- name: Gas
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ (value_json.GasConsumption|float/1000) }}"
  unit_of_measurement: m³
  </pre>

Some examples making sensors to calculate values:
<pre>
- name: Total Power Consumption
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ ((((value_json.powerConsumptionLowTariff|float)+(value_json.powerConsumptionHighTariff|float))/1000)|round(0)) + 849 }}"
  unit_of_measurement: kWh

- name: Total Gas Consumption
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ ((value_json.GasConsumption|float/1000)|round(0)) + 121 }}"
  unit_of_measurement: m³
  
 - name: Power Usage
  platform: mqtt
  state_topic: energy/p1
  value_template: "{{ (value_json.CurrentPowerConsumption|int) }}"
  unit_of_measurement: W
 </pre>
