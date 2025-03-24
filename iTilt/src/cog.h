// подключение к wifi
// публикация данных
// хранение данных

#ifndef COG_H
#define COG_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

#pragma pack(1)
struct SensorData {
  uint32_t seconds; // as is
  float tilt; // as is
  int16_t temp; // -320.00 .. +320.00 (*100)
  uint16_t batvolt; // 0..4.20 (*100)
  //uint16_t gravity; // 0 .. 2.0000 (*10k)
  uint8_t signal_strength; // 0..100
};
#pragma pack()

float getRSSI();
bool connectToWiFi();
bool connectCOG( WiFiClient& client, uint16_t tilt_id, const char cloud_username[], const char cloud_password[], uint16_t qty );
bool getResponseCodeCOG( WiFiClient& client );
void getResponseCOG( WiFiClient& client );
bool pubFileToCOG( uint16_t tilt_id, const char cloud_username[], const char cloud_password[] );
bool pubReadingToCOG( uint16_t tilt_id, const char cloud_username[], const char cloud_password[], float batvolt, float grav, float temp,  float signal_strength, uint32_t seconds );
//SensorData prepareSensorDataForUpload( float batvolt,  float grav, float temp,  float signal_strength, uint32_t seconds );
SensorData prepareSensorDataForUpload( float batvolt, float temp,  float signal_strength, uint32_t seconds, float tilt );
bool storeData( float batvolt,  float grav, float temp,  float signal_strength, uint32_t seconds );

#endif