/*
 Copyright (C) 2020 chester4444@wolke7.net
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #ifdef DEBUG
  #include <SoftwareSerial.h>
  #endif
#elif defined(ESP32)
  #include <WiFi.h>
  #include <ESPmDNS.h>
#endif
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include "credentials.h"
#include "WaterMeter.h"
#include "hwconfig.h"

#define ESP_NAME "WaterMeter"

#include "debug.h"
 
WaterMeter waterMeter;

WiFiClient espMqttClient;
PubSubClient mqttClient(espMqttClient);

char MyIp[16];
int cred = -1;

void blink(unsigned int t) {

    for(int i=0; i < 20; i++)
    {
      digitalWrite(LED_BUILTIN, LOW); //off
      delay(t);
      DEBUG_PRINT(".");
      digitalWrite(LED_BUILTIN, HIGH); //on
      delay(t);
    }
}
int getWifiToConnect(int numSsid)
{
  for (int i = 0; i < NUM_SSID_CREDENTIALS; i++)
  {
    //DEBUG_PRINTLN(WiFi.SSID(i));
    
    for (int j = 0; j < numSsid; ++j)
    {
      /*DEBUG_PRINT(j);
      DEBUG_PRINT(": ");
      DEBUG_PRINT(WiFi.SSID(i).c_str());
      DEBUG_PRINT(" = ");
      DEBUG_PRINTLN(credentials[j][0]);*/
      if (strcmp(WiFi.SSID(j).c_str(), credentials[i][0]) == 0)
      {
        DEBUG_PRINTLN("Credentials found for: ");
        DEBUG_PRINTLN(credentials[i][0]);
        return i;
      }
    }
  }
  return -1;
}

// connect to wifi – returns true if successful or false if not
bool ConnectWifi(void)
{
  int i = 0;

  DEBUG_PRINTLN("starting scan");
  // scan for nearby networks:
  int numSsid = WiFi.scanNetworks();

  DEBUG_PRINT("scanning WIFI, found ");
  DEBUG_PRINT(numSsid);
  DEBUG_PRINTLN(" available access points:");

  if (numSsid == -1)
  {
    DEBUG_PRINTLN("Couldn't get a wifi connection");
    return false;
  }
  
  for (int i = 0; i < numSsid; i++)
  {
    DEBUG_PRINT(i+1);
    DEBUG_PRINT(") ");
    DEBUG_PRINTLN(WiFi.SSID(i));
  }

  // search for given credentials
  cred = getWifiToConnect(numSsid);
  if (cred == -1)
  {
    DEBUG_PRINTLN("No Wifi!");
    return false;
  }

  // try to connect
  WiFi.begin(credentials[cred][0], credentials[cred][1]);
  DEBUG_PRINTLN("");
  DEBUG_PRINT("Connecting to WiFi ");
  DEBUG_PRINTLN(credentials[cred][0]);

  i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    DEBUG_PRINT(".");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    if (i++ > 30)
    {
      // giving up
      return false;
    }
  }
  return true;
}

void mqttDebug(const char* debug_str)
{
    String s="watermeter/0/debug";
    mqttClient.publish(s.c_str(), debug_str);
}

void mqttCallback(char* topic, byte* payload, unsigned int len)
{
  // create a local copies of topic and payload
  // PubSubClient overwrites it internally
  String t(topic);
  byte *p = new byte[len];
  memcpy(p, payload, len);
  
/*  DEBUG_PRINT("MQTT-RECV: ");
  DEBUG_PRINT(topic);
  DEBUG_PRINT(" ");
  DEBUG_PRINTLN((char)payload[0]); // FIXME LEN
*/
  if (strstr(topic, "/smarthomeNG/start"))
  {
    if (len == 4) // True
    {
      // maybe to something
    }
  }
  else if (strstr(topic, "/espmeter/reset"))
  {
    if (len == 4) // True
    {
      // maybe to something
      const char *topic = "/espmeter/reset/status";
      const char *msg = "False";
      mqttClient.publish(topic, msg);
      mqttClient.loop();
      delay(200);

      // reboot
      ESP.restart();
    }
  }
  // and of course, free it
  delete[] p;
}

bool mqttConnect()
{
  mqttClient.setServer(credentials[cred][2], 1883);
  mqttClient.setCallback(mqttCallback);

  // connect client to retainable last will message
  return mqttClient.connect(ESP_NAME, mqtt_user, mqtt_pass, "watermeter/0/online", 0, true, "False");
}

void  mqttMyData(const char* debug_str)
{
    String s="watermeter/0/sensor/mydata";
    mqttClient.publish(s.c_str(), debug_str, true);
}

void  mqttMyDataJson(const char* debug_str)
{
    String s="watermeter/0/sensor/mydatajson";
    mqttClient.publish(s.c_str(), debug_str, true);
}

void mqttSubscribe()
{
  String s;
  // publish online status
  s = "watermeter/0/online";
  mqttClient.publish(s.c_str(), "True", true);
  DEBUG_PRINT("MQTT-SEND: ");
  DEBUG_PRINT(s);
  DEBUG_PRINTLN(" True");
  
  // publish ip address
  s="watermeter/0/ipaddr";
  IPAddress MyIP = WiFi.localIP();
  snprintf(MyIp, 16, "%d.%d.%d.%d", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
  mqttClient.publish(s.c_str(), MyIp, true);
  DEBUG_PRINT("MQTT-SEND: ");
  DEBUG_PRINT(s);
  DEBUG_PRINT(" ");
  DEBUG_PRINTLN(MyIp);

  // if smarthome.py restarts -> publish init values
  s = "/smarthomeNG/start";
  mqttClient.subscribe(s.c_str());

  // if True; meter data are published every 5 seconds
  // if False: meter data are published once a minute
  s = "watermeter/0/liveData";
  mqttClient.subscribe(s.c_str());

  // if True -> perform an reset
  s = "espmeter/reset";
  mqttClient.subscribe(s.c_str());
}

void setupOTA()
{
    // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ESP_NAME);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    #ifdef DEBUG
    Serial.println("Start updating " + type);
    #endif
  });
  ArduinoOTA.onEnd([]() {
    #ifdef DEBUG
    Serial.println("\nEnd");
    #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef DEBUG
    DEBUG_PRINTF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    #endif
  });
  ArduinoOTA.begin();
}

// receive encrypted packets -> send it via MQTT to decrypter
void waterMeterLoop()
{
  if (waterMeter.isFrameAvailable())
  {
    // publish meter info via MQTT

  }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // on 

    DEBUG_BEGIN(115200);

    blink(100);
    waterMeter.begin();
    digitalWrite(LED_BUILTIN, LOW); // off 
    DEBUG_PRINTLN("Setup done...");
}

enum ControlStateType
  { StateInit
  , StateNotConnected
  , StateWifiConnect
  , StateMqttConnect
  , StateConnected
  , StateOperating
  };
ControlStateType ControlState = StateInit;

void loop()
{
  switch (ControlState)
  {
    case StateInit:
      //DEBUG_PRINTLN("StateInit:");
      WiFi.mode(WIFI_STA);

      ControlState = StateNotConnected;
      break;

    case StateNotConnected:
      //DEBUG_PRINTLN("StateNotConnected:");

      ControlState = StateWifiConnect;
      break;
      
    case StateWifiConnect:
      //DEBUG_PRINTLN("StateWifiConnect:");
      // station mode
      blink(200);
      ConnectWifi();

      delay(500);
      
      if (WiFi.status() == WL_CONNECTED)
      {
        DEBUG_PRINTLN("");
        DEBUG_PRINT("Connected to ");
        DEBUG_PRINTLN(credentials[cred][0]); // FIXME
        DEBUG_PRINT("IP address: ");
        DEBUG_PRINTLN(WiFi.localIP());

        setupOTA();
        
        ControlState = StateMqttConnect;
      }
      else
      {
        DEBUG_PRINTLN("");
        DEBUG_PRINTLN("Connection failed.");

        // try again
        ControlState = StateNotConnected;

        // reboot 
        ESP.restart();
      }
      break;

    case StateMqttConnect:
      DEBUG_PRINTLN("StateMqttConnect:");
      digitalWrite(LED_BUILTIN, HIGH); // off

      if (WiFi.status() != WL_CONNECTED)
      {
        ControlState = StateNotConnected;
        break; // exit (hopefully) switch statement
      }
      
      DEBUG_PRINT("try to connect to MQTT server ");
      DEBUG_PRINTLN(credentials[cred][2]); // FIXME

      if (mqttConnect())
      {
        ControlState = StateConnected;
      }
      else
      {
        DEBUG_PRINTLN("MQTT connect failed");

        delay(1000);
        // try again
      }
      ArduinoOTA.handle();
      
      break;

    case StateConnected:
      DEBUG_PRINTLN("StateConnected:");

      if (!mqttClient.connected())
      {
        ControlState = StateMqttConnect;
        delay(1000);
      }
      else
      {
        // subscribe to given topics
        mqttSubscribe();
        
        ControlState = StateOperating;
        digitalWrite(LED_BUILTIN, LOW); // off
        DEBUG_PRINTLN("StateOperating:");
        //mqttDebug("up and running");
      }
      ArduinoOTA.handle();
      
      break;
    
    case StateOperating:
      //DEBUG_PRINTLN("StateOperating:");

      if (WiFi.status() != WL_CONNECTED)
      {
        ControlState = StateWifiConnect;
        break; // exit (hopefully switch statement)
      }

      if (!mqttClient.connected())
      {
        DEBUG_PRINTLN("not connected to MQTT server");
        ControlState = StateMqttConnect;
      }

      // here we go
      waterMeterLoop();

      mqttClient.loop();

      ArduinoOTA.handle();

      break;

    default:
      DEBUG_PRINTLN("Error: invalid ControlState");  
  }
}
