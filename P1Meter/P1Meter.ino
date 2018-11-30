#include <PubSubClient.h>

#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//ADJUST THIS SETTING TO 768 IN pubsubclient.h
//#define MQTT_MAX_PACKET_SIZE 768

//your settings, change accordingly
#define SERIAL_RX D5 // pin for SoftwareSerial RX

const char *ssid = "wifinetwork";
const char *password = "wifipassword";
const char *hostName = "nodemcu-p1";
const char *mqttServer = "192.168.2.35";
const char *mqttClientName = "nodemcu-p1";
const char *mqttName ="login";
const char *mqttPassword = "password";
const char *mqttP1Topic = "/energy/p1";
const char *mqttLogTopic = "/log/p1";
const int mqttPort = 1883;

//useful for debugging, outputs info to serial port
const bool outputOnSerial = false;
const bool outputMqttLog = false;

// Vars to store meter readings
long powerConsumptionLowTariff = 0;  //Meter reading Electrics - consumption low tariff in watt hours
long powerConsumptionHighTariff = 0; //Meter reading Electrics - consumption high tariff  in watt hours
long powerProductionLowTariff = 0;   //Meter reading Electrics - return low tariff  in watt hours
long powerProductionHighTariff = 0;  //Meter reading Electrics - return high tariff  in watt hours
long CurrentPowerConsumption = 0;    //Meter reading Electrics - Actual consumption in watts
long CurrentPowerProduction = 0;     //Meter reading Electrics - Actual return in watts
long GasConsumption = 0;             //Meter reading Gas in m3

long OldPowerConsumptionLowTariff = 0;
long OldPowerConsumptionHighTariff = 0;
long OldPowerProductionLowTariff = 0;
long OldPowerProductionHighTariff = 0;
long OldGasConsumption = 0;

bool firstRun = true;

//Infrastructure stuff
#define MAXLINELENGTH 64
char telegram[MAXLINELENGTH];
SoftwareSerial mySerial(SERIAL_RX, -1, true, MAXLINELENGTH); // (RX, TX. inverted, buffer)
WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  mySerial.begin(115200);
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqttServer, mqttPort);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  readTelegram();
  
   if (millis() > 600000) {
    ESP.restart();
  }
  
  ArduinoOTA.handle();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqttClientName,mqttName,mqttPassword))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishP1ToMqtt()
{
  if (CheckData())
  {
    char msgpub[256];
    char output[256];
    String msg = "{";
    msg.concat("\"powerConsumptionLowTariff\": %lu,");
    msg.concat("\"powerConsumptionHighTariff\": %lu,");
    msg.concat("\"powerProductionLowTariff\": %lu,");
    msg.concat("\"powerProductionHighTariff\": %lu,");
    msg.concat("\"CurrentPowerConsumption\": %lu,");
    msg.concat("\"CurrentPowerProduction\": %lu,");
    msg.concat("\"GasConsumption\": %lu");
    msg.concat("}");
    msg.toCharArray(msgpub, 256);

    sprintf(output, msgpub, powerConsumptionLowTariff, powerConsumptionHighTariff, powerProductionLowTariff, powerProductionHighTariff, CurrentPowerConsumption, CurrentPowerProduction, GasConsumption);
    client.publish(mqttP1Topic, output);
  }
}

void readTelegram()
{
  if (mySerial.available())
  {
    if (outputOnSerial)
    {
      Serial.println("\nTrying to read");
    }
    memset(telegram, 0, sizeof(telegram));
    while (mySerial.available())
    {
      int len = mySerial.readBytesUntil('\n', telegram, MAXLINELENGTH);
      telegram[len] = '\n';
      telegram[len + 1] = 0;
      yield();
      if (decodeTelegram(len + 1))
      {
        publishP1ToMqtt();
      }
    }
  }
}

bool decodeTelegram(int len)
{
  //need to check for start
  int startChar = FindCharInArrayRev(telegram, '/', len);
  int endChar = FindCharInArrayRev(telegram, '!', len);
  bool endOfMessage = false;
  if (startChar >= 0)
  {
    if (outputOnSerial)
    {
      for (int cnt = startChar; cnt < len - startChar; cnt++)
        Serial.print(telegram[cnt]);
    }
  }
  else if (endChar >= 0)
  {
    endOfMessage = true;
    if (outputOnSerial)
    {
      for (int cnt = 0; cnt < len; cnt++)
        Serial.print(telegram[cnt]);
    }
  }
  else
  {
    if (outputOnSerial)
    {
      for (int cnt = 0; cnt < len; cnt++)
        Serial.print(telegram[cnt]);
    }
  }

  long val = 0;
  long val2 = 0;
  // 1-0:1.8.1(000992.992*kWh)
  // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v4.0)
  if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
    powerConsumptionLowTariff = getValue(telegram, len);

  // 1-0:1.8.2(000560.157*kWh)
  // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v4.0)
  if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0)
    powerConsumptionHighTariff = getValue(telegram, len);

  // 1-0:2.8.1(000348.890*kWh)
  // 1-0:2.8.1 = Elektra opbrengst laag tarief (DSMR v4.0)
  if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0)
    powerProductionLowTariff = getValue(telegram, len);

  // 1-0:2.8.2(000859.885*kWh)
  // 1-0:2.8.2 = Elektra opbrengst hoog tarief (DSMR v4.0)
  if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0)
    powerProductionHighTariff = getValue(telegram, len);

  // 1-0:1.7.0(00.424*kW) Actueel verbruik
  // 1-0:2.7.0(00.000*kW) Actuele teruglevering
  // 1-0:1.7.x = Electricity consumption actual usage (DSMR v4.0)
  if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0)
    CurrentPowerConsumption = getValue(telegram, len);

  if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
    CurrentPowerProduction = getValue(telegram, len);

  // 0-1:24.2.1(150531200000S)(00811.923*m3)
  // 0-1:24.2.1 = Gas (DSMR v4.0) on Kaifa MA105 meter
  if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0)
    GasConsumption = getValue(telegram, len);

  return endOfMessage;
}

long getValue(char *buffer, int maxlen)
{
  int s = FindCharInArrayRev(buffer, '(', maxlen - 2);
  if (s < 8)
    return 0;
  if (s > 32)
    s = 32;
  int l = FindCharInArrayRev(buffer, '*', maxlen - 2) - s - 1;
  if (l < 4)
    return 0;
  if (l > 12)
    return 0;
  char res[16];
  memset(res, 0, sizeof(res));

  if (strncpy(res, buffer + s + 1, l))
  {
    if (isNumber(res, l))
    {
      return (1000 * atof(res));
    }
  }
  return 0;
}

int FindCharInArrayRev(char array[], char c, int len)
{
  for (int i = len - 1; i >= 0; i--)
  {
    if (array[i] == c)
    {
      return i;
    }
  }
  return -1;
}

bool CheckData()
{
  if (firstRun)
  {
    SetOldValues();
    firstRun = false;
    return true;
  }

  if (outputMqttLog)
  {
    char msgpub[768];
    char output[768];
    String msg = "{";
    msg.concat("\"powerConsumptionLowTariff\": %lu,");
    msg.concat("\"powerConsumptionHighTariff\": %lu,");
    msg.concat("\"powerProductionLowTariff\": %lu,");
    msg.concat("\"powerProductionHighTariff\": %lu,");
    msg.concat("\"CurrentPowerConsumption\": %lu,");
    msg.concat("\"CurrentPowerProduction\": %lu,");
    msg.concat("\"GasConsumption\": %lu,");
    msg.concat("\"OldPowerConsumptionLowTariff\": %lu,");
    msg.concat("\"OldPowerConsumptionHighTariff\": %lu,");
    msg.concat("\"OldPowerProductionLowTariff\": %lu,");
    msg.concat("\"OldPowerProductionHighTariff\": %lu,");
    msg.concat("\"OldGasConsumption\": %lu");
    msg.concat("}");
    msg.toCharArray(msgpub, 768);
    sprintf(output, msgpub, powerConsumptionLowTariff, powerConsumptionHighTariff, powerProductionLowTariff, powerProductionHighTariff, CurrentPowerConsumption, CurrentPowerProduction, GasConsumption, OldPowerConsumptionLowTariff, OldPowerConsumptionHighTariff, OldPowerProductionLowTariff, OldPowerProductionHighTariff, OldGasConsumption);
    client.publish(mqttLogTopic, output);
  }
  if ((powerConsumptionLowTariff - OldPowerConsumptionLowTariff > 70) || powerConsumptionLowTariff < 0)
  {
    return false;
  }
  if ((powerConsumptionHighTariff - OldPowerConsumptionHighTariff > 70) || powerConsumptionHighTariff < 0)
  {
    return false;
  }
  if ((powerProductionLowTariff - OldPowerProductionLowTariff > 70) || powerProductionLowTariff < 0)
  {
    return false;
  }
  if ((powerProductionHighTariff - OldPowerProductionHighTariff > 70) || powerProductionHighTariff < 0)
  {
    return false;
  }
  if ((GasConsumption - OldGasConsumption > 1) || GasConsumption < 0)
  {
    return false;
  }

  SetOldValues();
  return true;
}

void SetOldValues()
{
  OldPowerConsumptionLowTariff = powerConsumptionLowTariff;
  OldPowerConsumptionHighTariff = powerConsumptionHighTariff;
  OldPowerProductionLowTariff = powerProductionLowTariff;
  OldPowerProductionHighTariff = powerProductionHighTariff;
  OldGasConsumption = GasConsumption;
}

bool isNumber(char *res, int len)
{
  for (int i = 0; i < len; i++)
  {
    if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0))
    {
      return false;
    }
  }
  return true;
}
