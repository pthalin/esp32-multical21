#ifndef CREDENTIALS_H
#define CREDENTIALS_H
#include <Arduino.h>

//Wifi settings: SSID, PW, MQTT broker
struct
{
  const char* ssid;
  const char* password;
  const char* hostName;
  const char* mqttAddress;
  int   mqttPort;
  const char* mqttUser;
  const char* mqttPassword;
} static const credentials[] = 
{
  // SSID,          PW,               host name          MQTT-address      MQTT-port MQTT-User     MQTT-password               
  {"mySsid",        "myPassword",     "esp-water-meter", "192.168.1.xxx",  1883,     "myMqttUser", "myMqttPassword" },
};

const uint8_t meterId[4] = { 0x00, 0x00, 0x00, 0x00 }; // Multical21 serial. Printed as hex on meter.
const uint8_t key[16] = { 0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 }; // AES-128 key. Ask your service provider.

#endif