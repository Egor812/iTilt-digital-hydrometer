#ifndef MAIN_H
#define MAIN_H

#define DEBUG

#include <Arduino.h>


#ifdef ARDUINO_ESP32C3_DEV
  #define DS18B20_DATA_PIN 10 //D10
  //#define DS18B20_POWER_PIN "9" 
  #define SDA_PIN 6 //D4
  #define SCL_PIN 7 //D5
  #define PERIPH_POWER_PIN 5 //D3
  #define VBAT_PIN 2 //D0 A0
#else
  #define DS18B20_DATA_PIN 16
  //#define DS18B20_POWER_PIN "17"
  #define SDA_PIN 15
  #define SCL_PIN 21
  #define PERIPH_POWER_PIN 22
  #define VBAT_PIN 36
#endif

#ifdef DEBUG
 #define DEBUG_PRINTLN(x) Serial.println(x)
 #define DEBUG_PRINT(x) Serial.print(x)
 #define DEBUG_DELAY(x) delay(x);
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
 #define DEBUG_DELAY(x)
#endif



extern float tilt;
extern float tilt_ema;
extern float temperature;
extern uint8_t acc_status; // 0 - power off; 1 - not init;  2-ready

struct Settings{
  uint16_t itiltnum = 0;
  double coefficientx3 = 0.0000010000000; //model to convert tilt to gravity
  double coefficientx2 = -0.000131373000;
  double coefficientx1 = 0.0069679520000;
  double constantterm  = 0.8923835598800;
  float tiltOffset = 0;
  float batconvfact=0.685; //  k=R2/(R1+R2) = 10k / (10k+4.7k) = 0.68
  float originalgravity = 1.05;
  uint32_t pubint = 600; // publication interval in seconds
  uint32_t offlinepubint = 3600; // publication try / data store when offline 
  uint16_t portalTimeOut = 9000;
  char cloud_host[32] = "cog.arkhipy1.beget.tech";
  char cloud_username[37];
  char cloud_password[41];
  uint8_t language = 1;
  bool new_calibration = 1; // 1 - когда задан новый id устройства или новые коэффициенты калибровки. После отправки на сервер сбрасывается в 0.
};
extern Settings settings;

struct Hardware{
  uint8_t onewire_pin = DS18B20_DATA_PIN;
  uint8_t i2c_sda_pin = SDA_PIN;
  uint8_t i2c_scl_pin = SCL_PIN;  
  uint8_t power_pin = PERIPH_POWER_PIN;
  uint8_t batvolt_pin = VBAT_PIN;
  uint8_t mpu_orientation = 1; // If Vcc is on Top when iTilt is in wort, this should be 1, else 0. The value can be overwritten in 192.168.4.1/pinconfinput?
  uint8_t i2c_address = 0x68;
};
extern Hardware hardware;


void disableGPIOHold();
void powerUpSensors();
void powerDownSensors();
void infiniteSleep();
bool startDeepSleep(double interval);


//const char* getStringFromPROGMEM(StringIndex index);


bool storeData( float batvolt,  float grav, float temp,  float signal_strength, uint32_t seconds );
uint32_t getFreeSpace();



void readConfiguration();
void saveCalibrationUpdated(void);

//Sensors and conversations
float calcOffset();
float calcBatCap(float volts);
float calcBatVolt(int sample_size);
float calcBatThresholdAnalyze(float threshold, int minReadings, int maxReadings);
float calcTemp();
float calcGyroTemp();
float calcTilt(int samplesize);
float calcRoll(int samplesize);
float calcGrav(float tilt);
float calcABV(float gravity, float og);
void readTilt(void);
void initMPU(uint8_t filter);
void finishMPUReadings(void);


#endif