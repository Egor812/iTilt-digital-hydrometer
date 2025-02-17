//Project Notes:https://github.com/JJSlabbert/iTilt-digital-hydrometer
//Board Seeed studio XIAO-ESP32-C3
//Using #ifdef ARDUINO_ESP32C3_DEV for specifically code
//How to turn on Serial monitor on ESP32-C3-Zero Arduino IDE > Tools > USB CDC On Boot > Enabled. Flash.


//TODO
// - обновление параметров без перезагрузки
// + калибровка - фильтр
// + калибровка - сохранить коэффициенты
// - !ресет таймера в режиме сохранения данных. Сохранение данных до ресета
// + зависание при ресете. fixed?
// - portaltimeout
// - оптимизация (типа atof в цикле)
// - все настройки, которые надо переводить atof много раз - в глобальные переменные
// - тексты в файлах для локализации на разные языки
// - 21700 и ЛоРа
// - сглаживание показаний
// / mpu ultra low  power - https://stackoverflow.com/questions/54450757/how-use-the-mpu-6050-in-ultra-low-power-mode



//fork updates:
// ESP32-C3 board
// save data to flash when wi-fi not present
// own server
// remove MQTT services. No time to modify.
// SPIFFS -> LittleFS
// No ESP8266 support. No time to modify


//c3 feautures
// it's better don't use gpio9. It's can make startup in the boot mode. See Strapping Pins

//Питание DS и MPU от одного GPIO:
//Ток на GPIO 40мА. Рекомендуется 20мА
//MPU6050 4mA + LED 0.1mA + I2C 0.7mA
//DS18D20 1.5mA
//==OK

float firmwareversion=1.10;


#define DEBUG

#ifdef ESP8266
  #define DS18B20_DATA_PIN "12"
  #define SDA_PIN "21"
  #define SCL_PIN "22"
#endif

#ifdef ESP32

#ifdef ARDUINO_ESP32C3_DEV
  #define DS18B20_DATA_PIN "10" //D10
  //#define DS18B20_POWER_PIN "9" 
  #define SDA_PIN "6" //D4
  #define SCL_PIN "7" //D5
  #define PERIPH_POWER_PIN "5"
  #define VBAT_PIN "2" //D0 A0
#else
  #define DS18B20_DATA_PIN "16"
  //#define DS18B20_POWER_PIN "17"
  #define SDA_PIN "15"
  #define SCL_PIN "21"
  #define PERIPH_POWER_PIN "22"
  #define VBAT_PIN "36"
#endif

#endif

#ifdef DEBUG
 #define DEBUG_PRINTLN(x) Serial.println(x)
#else
 #define DEBUG_PRINTLN(x)
#endif

#include "tpl.h"
#define LANG 1 //todo пункт меню

//WiFiManager global declerations
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFi.h>
#include <WiFiClientSecure.h>     //for COG server

#include <ESP32Time.h>

#include <LittleFS.h>             //https://github.com/littlefs-project/littlefs/blob/master/README.md
//waiting for update https://github.com/espressif/arduino-esp32/issues/9912

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


float tilt;
float tilt_ema=0;
float tilt_lpf=0;



//define your default values here, if there are different values in config.json, they are overwritten.
char portalTimeOut[5] = "9000";
char cloud_username[37];
char cloud_password[41];
char cloud_clientid[37];
//char cloud_service[20]="COG";
char cloud_host[32]="cog.arkhipy1.beget.tech";
//char coefficientx3[16] = "0.0000010000000"; //model to convert tilt to gravity
//char coefficientx2[16] = "-0.000131373000";
//char coefficientx1[16] = "0.0069679520000";
//char constantterm[16] =  "0.8923835598800";
/*#ifdef ESP8266
  char batconvfact[8] = "193.00"; //converting reading to battery voltage 196.25
#endif
#ifdef ESP32
  char batconvfact[8]="0.68"; //  k=R2/(R1+R2) = 10k / (10k+4.7k) = 0.68
#endif*/
char pubint[6] = "600"; // publication interval in seconds
char offlinepubint[6] = "3600"; // publication try / data store when offline 
//char originalgravity[6] = "1.05";
char tiltOffset[5] = "0";
char itiltnum[4]="0"; // iTilt ID
char dummy[3];

struct Settings{
  float coefficientx3 = 0.0000010000000; //model to convert tilt to gravity
  float coefficientx2 = -0.000131373000;
  float coefficientx1 = 0.0069679520000;
  float constantterm = 0.8923835598800;
  #ifdef ESP8266
    float batconvfact=0.68; //  k=R2/(R1+R2) = 10k / (10k+4.7k) = 0.68
  #endif
  #ifdef ESP32
    float batconvfact=193;
  #endif
  float originalgravity = 1.05;
  uint32_t pubint = 600; // publication interval in seconds
  uint32_t offlinepubint = 3600; // publication try / data store when offline 
};
Settings settings;


#pragma pack(1)
struct SensorData {
  uint32_t seconds; // as is
  int16_t temp; // -320.00 .. +320.00 (*100)
  uint16_t batvolt; // 0..4.20 (*100)
  uint16_t gravity; // 0 .. 2.0000 (*10k)
  uint8_t signal_strength; // 0..100
};
#pragma pack()

WiFiManager wm;

ESP32Time rtc;


//flag for saving data (Custom params for WiFiManager
//bool shouldSaveConfig = false;


//MQTT global declerations
//https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>

//const char broker[] = "mqtt.mydevices.com";
//int        port     = 1883;


//DS18B20 global decleration
#include <OneWire.h>  //Instal from Arduino IDE
#include <DallasTemperature.h> //Instal from Arduino IDE
char onewire_pin[3]=DS18B20_DATA_PIN;


//MPU6050 global decleration 
//https://www.i2cdevlib.com/docs/html/class_m_p_u6050.html#a196404ef04b959083d4bf5e6f1cd8b98
#include <MPU6050.h> //Instal from Arduino IDE
#include <Wire.h>
char i2c_sda_pin[3]=SDA_PIN;
char i2c_scl_pin[3]=SCL_PIN;
int i2c_address=0x68;
MPU6050 accelgyro(i2c_address);
int16_t ax, ay, az;

//ESP32 POWER PIN
#ifdef ESP32
  char power_pin[3]= PERIPH_POWER_PIN;
 // char power_pin2[3]= PERIPH_POWER_PIN;
  //#include "esp_sleep.h"  //https://github.com/espressif/arduino-esp32/issues/2712 To keep GPIO power pin LOW during Deep Sleep
  //#include "driver/gpio.h"
  //#include "esp_err.h"

/*You mentioned that the data just needs to survive deep sleep. If that's the case, your best option (if it's large enough) is to use the ESP32's RTC static RAM. 
This chunk of memory will survive restarts and deep sleep mode, but will lose its state if power is interrupted. It's real RAM so you won't wear it out by writing 
to it frequently, and it doesn't cost a lot of energy to write to. The catch is there's only 8KB of it.*/

  RTC_DATA_ATTR static int connection_missing_count = 0;
#endif
#ifdef ESP32
  char mpu_orientation[2]="1"; // If Vcc is on Top when iTilt is in wort, this should be 1, else 0. The value can be overwritten in 192.168.4.1/pinconfinput?
#endif

#ifdef ESP32
  char batvolt_pin[3]=VBAT_PIN;
#endif
//GLOBAL HTML STRINGS
String htmlMenueText="<a href='/wifi?' class='button'>CONFIGURE WIFI</a>\
<br><a href='/deviceconfinput?' class='button'>CONFIGURE DEVICE</a>\
<br><a href='/readings?' class='button'>SENSOR READINGS</a>\
<br><a href='/offsetcalibration?' class='button'>OFFSET CALIBRATION</a>\    
<br><a href='/polynomialcalibrationstart?' class='button'>POLYNOMIAL CALIBRATION</a>\
<br><a href='/info?' class='button'>INFO</a>\ 
<br><a href='/exit?' class='button'>EXIT</a>\  
<br><a href='/update?' class='button'>FIRMWARE UPDATE</a>\
<br><a href='/pinconfinput?' class='button'>PIN AND SENSOR CONFIGURATIONS</a>";  

String htmlStyleText= "<style>  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;  max-width:900px; align-content:center; margin: auto; font-size: 30px; } p {font-size: 30px;} h1 {text-align: center;}\
.button {  background-color: blue;  border: none;  color: white;  padding: 30px 15px;  text-align: center;  text-decoration: none;  display: inline-block;  font-size: 30px;  margin: 4px 2px;  cursor: pointer; width: 900px; border-radius: 20px;} </style>";

String htmlWiFiConfStyleText= "<style>  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;  max-width:450px; align-content:center; margin: auto; } p {font-size: 30px;} h1 {text-align: center;}\
.button {  background-color: blue;  border: none;  color: white;  padding: 30px 15px;  text-align: center;  text-decoration: none;  display: inline-block;  font-size: 30px;  margin: 4px 2px;  cursor: pointer; width: 450px; border-radius: 20px;} </style>";

//Other global declerations
#include <curveFitting.h>  //used for polynomial calibration v1.06 from Arduino IDE


// Функция для чтения строки из PROGMEM
const char* getStringFromPROGMEM(StringIndex index) {
  return (const char*)pgm_read_ptr(&languages[LANG][index]);
}
#define LT(x) getStringFromPROGMEM(x)



size_t getHtmlMenu( char* buffer, size_t bufferSize )
{
  //const char* settingsText = getStringFromPROGMEM(CONFIGURE_WIFI);

   return snprintf_P(buffer, bufferSize, 
    PSTR("<a href='/wifi?' class='button'>%s</a>\
    <br><a href='/deviceconfinput?' class='button'>%s</a>\
    <br><a href='/readings?' class='button'>%s</a>\
    <br><a href='/offsetcalibration?' class='button'>%s</a>\    
    <br><a href='/polynomialcalibrationstart?' class='button'>%s</a>\
    <br><a href='/info?' class='button'>%s</a>\ 
    <br><a href='/exit?' class='button'>%s</a>\  
    <br><a href='/update?' class='button'>%s</a>\
    <br><a href='/pinconfinput?' class='button'>%s</a>"),
    LT(CONFIGURE_WIFI), LT(SETTINGS), LT(SENSOR_READINGS), LT(OFFSET_CALIBRATION), LT(POLYNOMIAL_CALIBRATION), LT(INFO), LT(EXIT), LT(FIRMWARE_UPDATE), LT(PIN_CONFIGURATION) );
}



/*
void pubToAdafruit(String batcap,String tempgyro,String temp,String tilt,String batvolt,String grav,String abv, String  signalstrength, String cloud_username, String cloud_password, String mqtt_client_id)
{
   WiFiClient wifiClient;
   PubSubClient client("io.adafruit.com", 1883,wifiClient);
   if (client.connect(mqtt_client_id.c_str(), cloud_username.c_str(), cloud_password.c_str())) 
   {
    Serial.println("Connected to Adafruit IO");
   }
   else
   {
    Serial.println("Failed to connect to Adafruit IO");
  }

  String topic_batcap=cloud_username+"/feeds/battery-capacity"; 
  String topic_tempgyro=cloud_username+"/feeds/gyro-temperature";     
  String topic_temp=cloud_username+"/feeds/beer-temperature";
  String topic_tilt=cloud_username+"/feeds/tilt";
  String topic_batvolt=cloud_username+"/feeds/battery-voltage";
  String topic_grav=cloud_username+"/feeds/gravity";
  String topic_abv=cloud_username+"/feeds/alcohol-by-volume";
  String topic_signalstrength=cloud_username+"/feeds/wifi-signalstrength";
  String topic_pubint=cloud_username+"/feeds/publication-interval";
  String topic_offlinepubint=cloud_username+"/feeds/offline-publication-interval";
  String topic_originalgravity=cloud_username+"/feeds/original-gravity";
  String topic_firmwareversion=cloud_username+"/feeds/firmware-version";

  client.publish(topic_batcap.c_str(),batcap.c_str());
  client.publish(topic_tempgyro.c_str(),tempgyro.c_str()); 
  client.publish(topic_temp.c_str(),temp.c_str());
  client.publish(topic_tilt.c_str(),tilt.c_str());
  client.publish(topic_batvolt.c_str(),batvolt.c_str());
  client.publish(topic_grav.c_str(),grav.c_str());
  client.publish(topic_abv.c_str(),abv.c_str());
  client.publish(topic_signalstrength.c_str(),signalstrength.c_str());

  client.publish(topic_pubint.c_str(),pubint);
  client.publish(topic_offlinepubint.c_str(),offlinepubint);
  client.publish(topic_originalgravity.c_str(),originalgravity);
  client.publish(topic_firmwareversion.c_str(),String(firmwareversion).c_str());
  delay(500);

}
*/

bool connectCOG( WiFiClient& client, uint16_t tilt_id, const char cloud_username[], const char cloud_password[], uint16_t qty )
{
  //const char*  server = "cog.arkhipy1.beget.tech";  // Server URL
  int port = 80;
  // WiFiClientSecure client; //TODO change to secure when https
  //client.setInsecure();
  //int port = 443;

  Serial.println("\nStarting connection to server...");
  if (!client.connect(cloud_host, port)){
    Serial.println("Connection failed!");
    return false;
  }
  Serial.println("Connected to server!");


  String authData = String(tilt_id) + "," + String(cloud_username) + "," + String(cloud_password) + "\n";
  uint8_t authDataSize = authData.length();
  uint8_t payloadSize = authDataSize + (qty * sizeof(SensorData));

  // Make a HTTP request:
  client.println("POST /rx HTTP/1.1");
  client.println("Host: " + String(cloud_host) );
  client.println("User-Agent: ESP");
  client.println("Content-Type: application/octet-stream;"); //  
  client.println("Connection: close");
  //client.println(F("Content-Type: application/x-www-form-urlencoded;")); // important!
  client.println("content-length:"+ String(payloadSize) );
  client.println();
  client.print(authData);
  //client.println(data);
  return true;
}

bool getResponseCodeCOG( WiFiClient& client )
{
  // Получение и обработка ответа сервера
  String statusLine = client.readStringUntil('\n'); // Читаем первую строку ответа (статус ответа)
  Serial.println("Status line: " + statusLine);

  // Извлечение кода ответа (второе слово в строке)
  int statusCode = statusLine.substring(9, 12).toInt();
  Serial.print("HTTP response code: ");
  Serial.println(statusCode);

  if (statusCode == 201) {
    Serial.println("Request was successful!");
    return true;
  } else {
    Serial.println("Request failed!");
    return false;
  }  
}

void getResponseCOG( WiFiClient& client )
{
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  // if there are incoming bytes available
  // from the server, read them and print them:
  Serial.println("Response body:");  
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }


}

void closeConnectionCOG( WiFiClient& client )
{
  client.stop();
  Serial.println("\nConnection closed");
}

bool pubFileToCOG( uint16_t tilt_id, const char cloud_username[], const char cloud_password[] )
{
  WiFiClient client;
  SensorData data;
  uint32_t rows;
  
  File file = LittleFS.open("/sensor_data.bin", "r");
  if (!file) {
    Serial.println("Ошибка открытия файла данных для чтения");
    return false;
  }

  size_t structSize = sizeof(SensorData);
  rows = file.size()/structSize;

  Serial.println("Размер файла:"+String(file.size())+" Записей:"+String(rows));

  if( !connectCOG( client, tilt_id, cloud_username, cloud_password, rows) ) return false;

  while (file.available() >= structSize) { // Проверяем, что остались данные размером не менее одной структуры
    // Читаем одну структуру из файла
    file.read((uint8_t*)&data, structSize);
    client.write((uint8_t*)&data, structSize );
  }  

  if( !getResponseCodeCOG( client ) ) return false;

  getResponseCOG( client );
  closeConnectionCOG( client );
  file.close();
  if (LittleFS.remove("/sensor_data.bin")) return true;
  return false;
}

// COG is non MQTT
bool pubReadingToCOG( uint16_t tilt_id, const char cloud_username[], const char cloud_password[], float batvolt,  float grav, float temp,  float signal_strength, uint32_t seconds )
{

  WiFiClient client;
  
  if( !connectCOG( client, tilt_id, cloud_username, cloud_password, 1) ) return false;

/*struct SensorData {
  uint32_t seconds; // as is
  int16_t temp; // -320.00 .. +320.00 (*100)
  uint16_t batvolt; // 0..4.20 (*100)
  uint16_t gravity; // 0 .. 2.0000 (*10k)
  uint8_t signal_strength; // 0..100
};*/

  Serial.println( "We've got:"+String(temp)+", "+String(batvolt)+", "+String(grav)+", "+String(signal_strength)+", "+String(seconds));
  SensorData data = prepareSensorDataForUpload( batvolt, grav, temp, signal_strength, seconds );
  Serial.println( "We're sending:"+String(data.temp)+", "+String(data.batvolt)+", "+String(data.gravity)+", "+String(data.signal_strength)+", "+String(data.seconds));

  client.write((uint8_t*)&data, 1 * sizeof(SensorData));

  if( !getResponseCodeCOG( client ) ) return false;

  getResponseCOG( client );
  closeConnectionCOG( client );
  return true;
}

/*
void pubToCayenne(String batcap,String tempgyro,String temp,String tilt,String batvolt,String grav,String abv, String  signalstrength, String cloud_username, String cloud_password, String mqtt_client_id)
{
  WiFiClient wifiClient;
 PubSubClient client("mqtt.mydevices.com", 1883,wifiClient);

 if (client.connect(mqtt_client_id.c_str(), cloud_username.c_str(), cloud_password.c_str())) 
 {
  Serial.println("Connected to Cayenne");
 }
 else
 {
  Serial.println("Failed to connect to Cayenne");
 }
 //delay(1000);
 String topic="v1/" +cloud_username +"/things/" +mqtt_client_id+"/data/json";        
 String payload1; //Cayenne payload is limited to 5 channels / varabls in the json string
 String payload2;
 String payload3; 
 payload1="[";
 
 payload1+="{\"channel\": 0,\"value\": "+String(001)+"}";  //iTilt id number
 payload1+=",{\"channel\": 1,\"value\": "+batcap+"}";
 payload1+=",{\"channel\": 2,\"value\": "+tempgyro+"}";
 payload1+=",{\"channel\": 3,\"value\": "+String(firmwareversion)+"}";  //Firmware Version
 payload1+=",{\"channel\": 4,\"value\": "+temp+"}";
 payload1+="]";


 payload2="[";
 payload2+="{\"channel\": 5,\"value\": "+tilt+"}";
 payload2+=",{\"channel\": 6,\"value\": "+batvolt+"}";
 payload2+=",{\"channel\": 7,\"value\": "+grav+"}";
 payload2+=",{\"channel\": 8,\"value\": "+String(pubint)+"}"; //pub interval
 payload2+=",{\"channel\": 9,\"value\": "+String(originalgravity)+"}"; //original gravity
 payload2+="]";


 payload3="["; 
 payload3+="{\"channel\": 10,\"value\": "+abv+"}";
 payload3+=",{\"channel\": 11,\"value\": "+signalstrength+"}"; 
 payload3+="]";

 Serial.println("Topic: "+topic);
 Serial.println("Payload1: "+payload1);
 Serial.println("Payload2: "+payload2);
 Serial.println("Payload3: "+payload3);  
 client.publish(topic.c_str(),payload1.c_str());
 client.publish(topic.c_str(),payload2.c_str());
 client.publish(topic.c_str(),payload3.c_str());
 delay(700);
}
*/

/*
void pubToUbidots(String batcap,String tempgyro,String temp,String tilt,String batvolt,String grav,String abv, String  signalstrength, String cloud_username, String cloud_password, String mqtt_device_name)
{
  WiFiClient wifiClient;
 PubSubClient client("industrial.api.ubidots.com", 1883,wifiClient);

 if (client.connect(mqtt_device_name.c_str(), cloud_username.c_str(), cloud_password.c_str())) 
 {
  Serial.println("Connected to Ubidots");
 }
 else
 {
  Serial.println("Failed to connect to Ubidots");
 }
 
 String topic="/v1.6/devices/"+mqtt_device_name;
 String payload;
 payload="{";
 
 payload+="\"Battery Capacity\":";
 payload+=batcap;

 payload+=",\"Temperature in Gyro\":";
 payload+=tempgyro;
 
 payload+=",\"Temperature from DS18B20\":";
 payload+=temp;

 payload+=",\"Tilt\":";
 payload+=tilt;

 payload+=",\"Battery Voltage\":";
 payload+=batvolt;

 payload+=",\"Gravity\":";
 payload+=grav;

 payload+=",\"ABV\":";
 payload+=abv;

 payload+=",\"WiFi Signalstrength\":";
 payload+=signalstrength;

 // payload+=",\"Publication Interval\":";
 // payload+=String(pubint);

 // payload+=",\"Original Gravity\":";
 // payload+=String(originalgravity);

 // payload+=",\"Firmware Version\":";
 // payload+=String(firmwareversion);

 payload+="}";

 Serial.println("Topic: "+topic);
 Serial.println("Payload: "+payload);

 client.publish(topic.c_str(), payload.c_str());
 delay(500);
}
*/

void inviniteSleep()  //(Hybernation Mode) This is used when battery is charged or iTilt is stored
{ 
  Serial.println("Roll is <20 or >160. Device will enter Invinite deep sleep.");
  #ifdef ESP32
  powerDownSensors();
  #ifndef ARDUINO_ESP32C3_DEV
    digitalWrite(LED_BUILTIN,LOW);
  #endif

  //https://github.com/espressif/arduino-esp32/issues/2712 To keep GPIO power pin LOW during Deep Sleep 
  gpio_hold_en(GPIO_NUM_2); 
  gpio_hold_en(GPIO_NUM_0);
  //GPIO 1 and 3 ist TXD and RXD
  gpio_hold_en(GPIO_NUM_4);
  gpio_hold_en(GPIO_NUM_5);
  gpio_hold_en (GPIO_NUM_18);
  gpio_hold_en (GPIO_NUM_19);
  gpio_hold_en (GPIO_NUM_21);
  #ifndef ARDUINO_ESP32C3_DEV
    gpio_hold_en (GPIO_NUM_22);
    gpio_hold_en (GPIO_NUM_23);
    gpio_hold_en (GPIO_NUM_25);
    gpio_hold_en (GPIO_NUM_26);
    gpio_hold_en (GPIO_NUM_27);
    gpio_hold_en (GPIO_NUM_32);
    gpio_hold_en (GPIO_NUM_33);
    //ESP32-C3: SPI0/1: GPIO12 ~ GPIO17 are usually used for SPI flash and PSRAM and are not recommended for other uses. 
    gpio_hold_en(GPIO_NUM_12);
    gpio_hold_en(GPIO_NUM_13);
    gpio_hold_en (GPIO_NUM_14);
    gpio_hold_en (GPIO_NUM_15);
    gpio_hold_en (GPIO_NUM_16); 
    gpio_hold_en (GPIO_NUM_17);    
  #endif
  gpio_deep_sleep_hold_en();      
  #endif

  #ifdef ESP8266
  ESP.deepSleep(999999999999999999);
  delay(1000);
  #endif
  #ifdef ESP32
  esp_sleep_enable_timer_wakeup(999999999999999999);  //How long can esp32 hibernate
  esp_deep_sleep_start();
  delay(1000);
  #endif
  
}

bool startDeepSleep(double interval)
{
  if (interval <60)
  {
    Serial.println("Data Publication Interval is less than 60 seconds, iTilt will not go to deep sleep but to setup()");
    delay(interval*1000);  
    return false;
  } 

  #ifdef ESP32
    powerDownSensors();
    #ifndef ARDUINO_ESP32C3_DEV
      digitalWrite(LED_BUILTIN,LOW);
    #endif

    //https://github.com/espressif/arduino-esp32/issues/2712 To keep GPIO power pin LOW during Deep Sleep 
    gpio_hold_en(GPIO_NUM_2); 
    gpio_hold_en(GPIO_NUM_0);
    //GPIO 1 and 3 ist TXD and RXD

    gpio_hold_en(GPIO_NUM_4);
    gpio_hold_en(GPIO_NUM_5);
    gpio_hold_en (GPIO_NUM_18);
    gpio_hold_en (GPIO_NUM_19);
    gpio_hold_en (GPIO_NUM_21);
    #ifndef ARDUINO_ESP32C3_DEV  
      gpio_hold_en (GPIO_NUM_22);
      gpio_hold_en (GPIO_NUM_23);
      gpio_hold_en (GPIO_NUM_25);
      gpio_hold_en (GPIO_NUM_26);
      gpio_hold_en (GPIO_NUM_27);
      gpio_hold_en (GPIO_NUM_32);
      gpio_hold_en (GPIO_NUM_33);
      //ESP32-C3: SPI0/1: GPIO12 ~ GPIO17 are usually used for SPI flash and PSRAM and are not recommended for other uses.      
      gpio_hold_en(GPIO_NUM_12);
      gpio_hold_en(GPIO_NUM_13);
      gpio_hold_en (GPIO_NUM_14);
      gpio_hold_en (GPIO_NUM_15);
      gpio_hold_en (GPIO_NUM_16); 
      gpio_hold_en (GPIO_NUM_17);
    #endif

  /* gpio_hold_en : However, on ESP32/S2/C3/S3/C2, this function cannot be used to hold the state of a digital GPIO during Deep-sleep.
  * Even if this function is enabled, the digital GPIO will be reset to its default state when the chip wakes up from
  * Deep-sleep. If you want to hold the state of a digital GPIO during Deep-sleep, please call `gpio_deep_sleep_hold_en`.
  */    
  gpio_deep_sleep_hold_en();

  #endif
  
  #ifndef ARDUINO_ESP32C3_DEV
    digitalWrite(LED_BUILTIN,LOW);
  #endif
  Serial.println("Entering deep sleep for " + String(interval) + " seconds");
  #ifdef ESP8266
    ESP.deepSleep(interval * 1000000);
    delay(1000);
  #endif
  #ifdef ESP32
    esp_sleep_enable_timer_wakeup(interval *  1000000);
    esp_deep_sleep_start();
    delay(1000);
  #endif
  return true;
}

float calcOffset()
{ 
  float reading;
  float areading=0;
  float n=100;
  pinMode(atoi(i2c_sda_pin), OUTPUT);//This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  pinMode(atoi(i2c_scl_pin), OUTPUT); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(atoi(i2c_sda_pin), LOW); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(atoi(i2c_scl_pin), LOW);  //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  delay(100);
  Wire.begin(atoi(i2c_sda_pin), atoi(i2c_scl_pin));
  Wire.beginTransmission(i2c_address);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  accelgyro.initialize();
  for (int i=0; i<n; i++)
  {
    accelgyro.getAcceleration(&ax, &az, &ay);
    reading=acos(az / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
    areading+=(1/n)*reading;
    Serial.println("Calibrating: "+String(reading));
    Serial.println("Avarage Reading: "+String(areading));
  }
  float offset;
  offset=89.0-areading;
  #ifdef ESP32
    if (atoi(mpu_orientation)==0)
      {offset=91-areading;} //This must be tested?
  #endif
  return offset;
}

float calcBatCap(float volts)  //linear interpolation used, data from http://www.benzoenergy.com/blog/post/what-is-the-relationship-between-voltage-and-capacity-of-18650-li-ion-battery.html
{
  float capacity;
  Serial.println("You are in calcBatCap(): volts is: "+String(volts));
  if (volts<=3)
  {
    capacity=0;
    return capacity;
  }
  if (volts>3 and volts<=3.45)
  {
    capacity=0+((volts-3)/(3.45-3))*5;
    return capacity;    
  }
  if (volts>3.45 and volts<=3.68)
  {
    capacity=5+((volts-3.45)/(3.68-3.45))*5;
    return capacity;    
  }
  if (volts>3.68 and volts<=3.74)
  {
   capacity=10+((volts-3.68)/(3.74-3.68))*10;
   return capacity;   
  }
  if (volts>3.74 and volts<=3.77)
  {
    capacity=20+((volts-3.74)/(3.77-3.74))*10;
    return capacity;    
  }
  if (volts>3.77 and volts<=3.79)
  {
    capacity=30+((volts-3.77)/(3.79-3.77))*10;
    return capacity;    
  }
    
  if (volts>3.79 and volts<=3.82)
  {
    capacity=40+((volts-3.79)/(3.82-3.79))*10;
    return capacity;    
  }
  if (volts>3.82 and volts<=3.87)
  {
    capacity=50+((volts-3.82)/(3.87-3.82))*10;
    return capacity;    
  }                
  if (volts>3.87 and volts<=3.92)
  {
    capacity=60+((volts-3.87)/(3.92-3.87))*10;
    return capacity;    
  }       
  if (volts>3.92 and volts<=3.98)
  {
    capacity=70+((volts-3.92)/(3.98-3.92))*10;
    return capacity;    
  }       
  if (volts>3.98 and volts<=4.06)
  {
    capacity=80+((volts-3.98)/(4.06-3.98))*10;
    return capacity;    
  }      
  if (volts>4.06)
  {
    capacity=90+((volts-4.06)/(4.16-4.06))*10;
    if (capacity>100)
    {capacity=100;}
    return capacity;    
  }           
}

float calcBatVolt(int sample_size)
{
  float reading=0;
  float n=sample_size;
  for (int i=0;i<n;i++)
  { 
    #ifdef ESP8266
    reading+=(1/n)*analogRead(A0)/settings.batconvfact;
    #endif
    #ifdef ESP32
    //reading+=(1/n)*analogRead(atoi(batvolt_pin))/atof(batconvfact);
    //reading+= analogRead(atoi(batvolt_pin))/n;
    reading+=analogReadMilliVolts(atoi(batvolt_pin));
    #endif
  }
  reading = reading/(1000*n*atof(settings.batconvfact));
  return reading;
}

float calcTemp()//DS19B20 Sensor Texas Instruments
{ 
    float reading;
    OneWire oneWire(atoi(onewire_pin));
    DallasTemperature sensors(&oneWire);
    sensors.begin();
    sensors.requestTemperatures();
    reading = sensors.getTempCByIndex(0);
    Serial.println("Temperature:"+String(reading));
    return reading;
}

float calcGyroTemp()
{
  pinMode(atoi(i2c_sda_pin), OUTPUT);//This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  pinMode(atoi(i2c_scl_pin), OUTPUT); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(atoi(i2c_sda_pin), LOW); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(atoi(i2c_scl_pin), LOW);  //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  delay(100);
  Wire.begin(atoi(i2c_sda_pin), atoi(i2c_scl_pin));
  Wire.beginTransmission(i2c_address);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
 
  accelgyro.initialize();
  float temp=accelgyro.getTemperature()/340 +36.53;
  accelgyro.setSleepEnabled(true);
  return temp;
}

float calcTilt(int samplesize)
{ 
  float reading;
  float areading=0;
  float n=samplesize;

  pinMode(atoi(i2c_sda_pin), OUTPUT);//This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  pinMode(atoi(i2c_scl_pin), OUTPUT); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(atoi(i2c_sda_pin), LOW); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(atoi(i2c_scl_pin), LOW);  //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  delay(100);
  Wire.begin(atoi(i2c_sda_pin), atoi(i2c_scl_pin));
  Wire.beginTransmission(i2c_address);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
 
  accelgyro.initialize();
  //Get Gyroscope readings untill it stoped reading nan (Not a Number)
  for (int i=0;i<200;i++)
  {
    accelgyro.getAcceleration(&ax, &az, &ay);
    if( ax+ay+az==0 ) {
      Serial.println("You are in calcTilt(). Reading=0");
    }
    else {
      reading=acos(az / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
      Serial.println("You are in calcTilt(). reading="+String(reading));
      break;
    }
    if (i==199)
    {
      Serial.println("You are in calcTilt(). The MPU6050 could not provide numerical readings for 200 times. Check your soldering and Test the MPU6050.");
    }
  }

  
  for (int i=0; i<n; i++)
  {
    accelgyro.getAcceleration(&ax, &az, &ay);
    reading=acos(az / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
    areading+=reading;
    //    Serial.println("Calibrating: "+String(reading));
    //    Serial.println("Avarage Reading: "+String(areading));
  }
  areading=areading/n;
  areading+=atof(tiltOffset);
  //  Serial.println("Gyro Readings: "+String(ax)+","+String(ay)+","+String(az));
  //  Serial.println("Calculated Tilt: "+String(reading));
  accelgyro.setSleepEnabled(true);//-----------------------------------------------------------------------------------------------
  #ifdef ESP32
    Serial.println("You are in calcTilt() mpu_orientation is: "+String(mpu_orientation));
    if (atoi(mpu_orientation)==0) //This is when Vcc on MPU is vacing down
    {
      areading=90-(areading-90);
    }
  #endif
  return areading;

}

float calcRoll(int samplesize)
{ 
  float reading;
  float areading=0;
  float n=samplesize;

  pinMode(atoi(i2c_sda_pin), OUTPUT);//This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  pinMode(atoi(i2c_scl_pin), OUTPUT); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(atoi(i2c_sda_pin), LOW); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(atoi(i2c_scl_pin), LOW);  //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  delay(100);
  Wire.begin(atoi(i2c_sda_pin), atoi(i2c_scl_pin));
  Wire.beginTransmission(i2c_address);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
 
  accelgyro.initialize();
  //Get Gyroscope readings untill it stoped reading nan (Not a Number)
  for (int i=0;i<1000;i++)
  {
    accelgyro.getAcceleration(&ax, &az, &ay);
    reading=acos(ax / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;  //reading=acos(az / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
    Serial.println("You are in calcRoll(). reading="+String(reading));
    if (String(reading)!="nan")
    {
      break;
    }
    if (i==999)
    {
      Serial.println("You are in calcRoll(). The MPU6050 could not provide numerical readings for 1000 times. Check Soldering or MPU6050");
    }
  }
  
  for (int i=0; i<n; i++)
  {
    accelgyro.getAcceleration(&ax, &az, &ay);
    reading=acos(ax / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;  //reading=acos(az / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
    areading+=reading;
    //    Serial.println("Calibrating: "+String(reading));
    //    Serial.println("Avarage Reading: "+String(areading));
  }
  areading = areading / n;
  
  //  Serial.println("Gyro Readings: "+String(ax)+","+String(ay)+","+String(az));
  //  Serial.println("Calculated Roll: "+String(areading));
  accelgyro.setSleepEnabled(true);//-----------------------------------------------------------------------------------------------
  return areading;

}

float calcGrav(float tilt)
{
  /*float fcoefficientx3 = atof(coefficientx3);
  float fcoefficientx2 = atof(coefficientx2);
  float fcoefficientx1 = atof(coefficientx1);
  float fconstantterm = atof(constantterm);*/
  return settings.coefficientx3 * (tilt * tilt * tilt) + settings.coefficientx2 * tilt * tilt + settings.coefficientx1 * tilt + settings.constantterm;
}

float calcABV(float gravity)
{
  float abv = 131.258 * (settings.originalgravity - gravity);
  return abv;
}

void bindServerCallback() {

  wm.server->on("/readings", handleReadings);
  wm.server->on("/offsetcalibration", handleOffsetCalibration);
  wm.server->on("/polynomialcalibrationstart", handlePolynomialCalibrationStart);
  wm.server->on("/polynomialcalibrationinput",handlePolynomialCalibrationInput);
  wm.server->on("/polynomialcalibrationresults",handlePolynomialCalibrationResults);
  wm.server->on("/pinconfinput",handlePinConfInput);
  wm.server->on("/updatepinconfresults",handlePinConfResults);
  wm.server->on("/deviceconfinput",handleDeviceConfInput);
  wm.server->on("/updatedeviceconfresults",handleDeviceConfResults);
  wm.server->on("/",handleRoute); // you can override wm! main page
}

void handleRoute()
{
  char html[1024];  // Буфер для хранения HTML-кода

  Serial.println("[HTTP] handle Route");
  String htmlText = "<HTML><HEAD><meta charset=\"UTF-8\"><TITLE>iTilt Main Page</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText+= "<BODY><h1>CONFIGURATION PORTAL for iTilt</h1>";
  getHtmlMenu(html, sizeof(html));
  htmlText+=html;
  htmlText += "</BODY></HTML>";  
  wm.server->send(200, "text/html", htmlText);
}

void handleDeviceConfResults()
{
  bool error = false;

  Serial.println("[HTTP] handle DeviceConfResults");
  Serial.println("saving device config");  
  DynamicJsonDocument json(1024);
  //json["cloud_service"]   = wm.server->arg(0);
  json["cloud_host"]   = wm.server->arg(0);
  json["cloud_username"]   = wm.server->arg(1);
  json["cloud_password"]   = wm.server->arg(2);    
  //json["cloud_clientid"] =wm.server->arg(3);
  json["coefficientx3"]   =wm.server->arg(3);
  json["coefficientx2"]   = wm.server->arg(4);
  json["coefficientx1"]=wm.server->arg(5);
  json["constantterm"]=wm.server->arg(6);
  json["batconvfact"]=wm.server->arg(7);
  json["pubint"]=wm.server->arg(8);
  json["offlinepubint"]=wm.server->arg(9);
  json["originalgravity"]=wm.server->arg(10);
  json["tiltOffset"]=wm.server->arg(11);
  json["itiltnum"]=wm.server->arg(12);
  json["portalTimeOut"]=wm.server->arg(13);


  if( strlen(json["coefficientx3"].as<const char*>())==0 ) error=true;
  if( strlen(json["coefficientx2"].as<const char*>())==0 ) error=true;
  if( strlen(json["coefficientx1"].as<const char*>())==0 ) error=true;
  if( strlen(json["constantterm"].as<const char*>())==0 ) error=true;
  if( strlen(json["batconvfact"].as<const char*>())==0 ) error=true;
  if( strlen(json["pubint"].as<const char*>())==0 ) error=true;
  if( strlen(json["offlinepubint"].as<const char*>())==0 ) error=true;
  if( strlen(json["originalgravity"].as<const char*>())==0 ) error=true;
  if( strlen(json["tiltOffset"].as<const char*>())==0 ) error=true;
  if( strlen(json["itiltnum"].as<const char*>())==0 ) error=true;
  if( strlen(json["portalTimeOut"].as<const char*>())==0 ) error=true;

  File pinConfigFile = LittleFS.open("/config.json", "w");
  if (!pinConfigFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(json, Serial);
  serializeJson(json, pinConfigFile);

  pinConfigFile.close(); 
   
  String htmlText="<HTML><HEAD><TITLE>iTilt Config</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText+= "<BODY><h1>Configuration is updated:</h1>";
  if( error ) htmlText+= "<p style='color:red'>But has errors</p>";
  htmlText+="<ul><li>"+wm.server->argName(0)+" "+String(wm.server->arg(0))+"</li>";
  htmlText+="<li>"+wm.server->argName(1)+" "+String(wm.server->arg(1))+"</li>";
  htmlText+="<li>"+wm.server->argName(2)+" "+String(wm.server->arg(2))+"</li>";
  htmlText+="<li>"+wm.server->argName(3)+" "+String(wm.server->arg(3))+"</li>";
  htmlText+="<li>"+wm.server->argName(4)+" "+String(wm.server->arg(4))+"</li>";
  htmlText+="<li>"+wm.server->argName(5)+" "+String(wm.server->arg(5))+"</li>"; 
  htmlText+="<li>"+wm.server->argName(6)+" "+String(wm.server->arg(6))+"</li>";       
  htmlText+="<li>"+wm.server->argName(7)+" "+String(wm.server->arg(7))+"</li>";       
  htmlText+="<li>"+wm.server->argName(8)+" "+String(wm.server->arg(8))+"</li>";       
  htmlText+="<li>"+wm.server->argName(9)+" "+String(wm.server->arg(9))+"</li>";       
  htmlText+="<li>"+wm.server->argName(10)+" "+String(wm.server->arg(10))+"</li>";       
  htmlText+="<li>"+wm.server->argName(11)+" "+String(wm.server->arg(11))+"</li>";       
  htmlText+="<li>"+wm.server->argName(12)+" "+String(wm.server->arg(12))+"</li>";       
  htmlText+="<li>"+wm.server->argName(13)+" "+String(wm.server->arg(13))+"</li></ul>";
  htmlText+="Перезагрузите устройство";
  htmlText+="<a href='/'>Back</a>";

  settings.coefficientx1 = json["coefficientx1"].as<float>();
  settings.coefficientx2 = json["coefficientx2"].as<float>();
  settings.coefficientx3 = json["coefficientx3"].as<float>();
  settings.constantterm = json["constantterm"].as<float>();
  settings.batconvfact = json["batconvfact"].as<float>();
 
  wm.server->send(200, "text/html", htmlText);
     
}

void handlePinConfResults()
{
  Serial.println("[HTTP] handle PinConfResults");
  Serial.println("saving pinconfig");  
  DynamicJsonDocument json(1024);
  #ifdef ESP32  
  json["power_pin"]   = wm.server->arg(0);
  //json["power_pin2"]   = wm.server->arg(1);
  json["batvolt_pin"]   = wm.server->arg(1);    
  json["onewire_pin"] =wm.server->arg(2);
  json["i2c_sda_pin"]   =wm.server->arg(3);
  json["i2c_scl_pin"]   = wm.server->arg(4);
  json["mpu_orientation"]=wm.server->arg(5);

  
  #endif
  #ifdef ESP8266
  json["onewire_pin"] =wm.server->arg(0);
  json["i2c_sda_pin"]   =wm.server->arg(1);
  json["i2c_scl_pin"]   = wm.server->arg(2);  
  #endif


  File pinConfigFile = LittleFS.open("/pinconfig.json", "w");
  if (!pinConfigFile) {
    Serial.println("failed to open pinconfig file for writing");
  }

  serializeJson(json, Serial);
  serializeJson(json, pinConfigFile);

  pinConfigFile.close(); 
   
  String htmlText="<HTML><HEAD><TITLE>iTilt Custom Pin Configuration</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText+= "<BODY><h1>CUSTOM PIN CONFIGURATION IS UPDATED AS FOLLOW</h1>";
  #ifdef ESP32  
  htmlText+="<p>"+wm.server->argName(0)+" "+String(wm.server->arg(0))+"<br>";
  htmlText+=wm.server->argName(1)+" "+String(wm.server->arg(1))+"<br>";
  htmlText+=wm.server->argName(2)+" "+String(wm.server->arg(2))+"<br>";
  htmlText+=wm.server->argName(3)+" "+String(wm.server->arg(3))+"</p>";
  htmlText+=wm.server->argName(4)+" "+String(wm.server->arg(4))+"</p>";
  htmlText+=wm.server->argName(5)+" "+String(wm.server->arg(5))+"</p>"; 
  htmlText+=wm.server->argName(6)+" "+String(wm.server->arg(6))+"</p>";       
  #endif
  #ifdef ESP8266
  htmlText+=wm.server->argName(0)+" "+String(wm.server->arg(0))+"<br>";
  htmlText+=wm.server->argName(1)+" "+String(wm.server->arg(1))+"<br>";
  htmlText+=wm.server->argName(2)+" "+String(wm.server->arg(2))+"</p>";    
  #endif

  htmlText+="<p> The iTilt will restart in less than 30 seconds. Updates will take effect after restart.</p>";
 
  wm.server->send(200, "text/html", htmlText);
  delay(2000);
  #ifdef ESP32
  ESP.restart();
  delay(3000); 
  #endif
  #ifdef ESP8266  //For some reason the ESP8266 AP Config Portal does not terminate on reset, restart, WiFi.disconnect()?
  ESP.deepSleep(20 * 1000000);
  delay(3000);
  #endif
     
}

void handleDeviceConfInput()
{
  Serial.println("[HTTP] handle DeviceConfInput");
  String htmlText = "<HTML><HEAD><TITLE>iTilt Device Configuration</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText+= "<BODY><h1>Device settings</h1>";
  htmlText+="<p><form action='/updatedeviceconfresults?' method ='POST'>";
  htmlText+="<table border='1' style='font-size:30pt;'>";
  //htmlText+="<tr><td>COG / MQTT SERVICE (CAYENNE, UBIDOTS, ADAFRUIT)</td><td><input type='text' name='service' size='5' value="+String(cloud_service)+" style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>COG Server</td><td><input type='text' name='service' size='32' value='"+String(cloud_host)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Cloud USERNAME</td><td><input type='text' name='usernname' size='32' value='"+String(cloud_username)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Cloud PASSWORD</td><td><input type='text' name='password' size='32' value='"+String(cloud_password)+"' style='height:80px;font-size:30pt;'></td></tr></table>"; 
  //htmlText+="<tr><td>Cloud CLIENT ID</td><td><input type='text' name='clientid' size='32' value='"+String(cloud_clientid)+"' style='height:80px;font-size:30pt;' disabled></td></tr>"; 
  htmlText+="<p><font size='4'><b>Provide the Parameters of you Polynomial</b></font></p>";
  htmlText+="<table border='1' style='font-size:30pt;'>";
  htmlText+="<tr><td>Coefficient of Tilt^3</td><td><input type='text' inputmode='decimal' name='coefficientx3' pattern='^[-+]?[0-9]*[.,]?[0-9]*' size='18' value='"+String(settings.coefficientx3,15)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Coefficient of Tilt^2</td><td><input type='text' inputmode='decimal' name='coefficientx2' pattern='^[-+]?[0-9]*[.,]?[0-9]*' size='18' value='"+String(settings.coefficientx2,15)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Coefficient of Tilt^1</td><td><input type='text' inputmode='decimal' name='coefficientx1' pattern='^[-+]?[0-9]*[.,]?[0-9]*' size='18' value='"+String(settings.coefficientx1,15)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Constant Term (Regression Model Polynomial)</td><td><input type='text' inputmode='decimal' name='constantterm' pattern='^[-+]?[0-9]*[.,]?[0-9]*' size='18' value='"+String(settings.constantterm,15)+"' style='height:80px;font-size:30pt;'></td></tr></table>"; 
  htmlText+="<p><font size='4'><b>Other Parameters for the iTilt</b></font></p>";
  htmlText+="<table border='1' style='font-size:30pt;'>";
  htmlText+="<tr><td>Battery Conversion Factor</td><td><input type='text' inputmode='decimal' pattern='^[-+]?[0-9]*[.,]?[0-9]*' name='batconvfact' size='8' value='"+String(settings.batconvfact)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Data Publication Interval (s)</td><td><input type='number' name='pubint' size='6' value='"+String(settings.pubint)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Data Publication Interval when offline (s)</td><td><input type='number' name='offlinepubint' size='6' value='"+String(settings.offlinepubint)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Original Gravity</td><td><input type='text' inputmode='decimal' pattern='^[-+]?[0-9]*[.,]?[0-9]*' name='originalgravity' size='6' value='"+String(settings.originalgravity)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Calibrated Tilt Offset</td><td><input type='text' inputmode='decimal' pattern='^[-+]?[0-9]*[.,]?[0-9]*' name='tiltOffset' size='5' value='"+String(tiltOffset)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>iTilt ID Number</td><td><input type='number' name='itiltnum' size='4' value='"+String(itiltnum)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>WiFi Manager Configuration Portal Time Out</td><td><input type='number' name='portalTimeOut' size='5' value='"+String(portalTimeOut)+"' style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="</table><br><input type='submit' value='Save' style='height:80px;font-size:30pt;'></form></p>";         
  htmlText+=htmlMenueText;
  htmlText += "</BODY></HTML>";
  wm.server->send(200, "text/html", htmlText);

}


void handlePinConfInput()
{
  Serial.println("[HTTP] handle PinConfInput");
  String htmlText = "<HTML><HEAD><TITLE>iTilt Custom Pin Configuration</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText+= "<BODY><h1>CUSTOM PIN CONFIGURATION for iTilt</h1>";
  htmlText+="<p>Thi iTilt uses default pin configurations";
  htmlText+="<br>You may re congigure these pins for your custom design";
  htmlText+="<br>Use only numerical values for GPIO numbers.";

  #ifdef ESP32
  htmlText+="<b><br>ESP32 (Works very well with ESP32-E Firebeetle:</b><br> Power=25,25, Battery Voltage=36, One Wire(DS18B20 Data)=26,<br> GYRO MPU6050_SDA=17, GYRO MPU6050_SCL=16 <br>";
  htmlText+="<br>To see what pins suitable for Temperature and Gyroscope sensors, <a href='https://randomnerdtutorials.com/esp32-pinout-reference-gpios/'>https://randomnerdtutorials.com/esp32-pinout-reference-gpios/</a>";
  htmlText+="<br>The ESP32 has 2 ADC chips. ADC 2 is used by WiFi. You should only use pins GPIO 39, 36, 34, 35, 32, 33 to measure battery voltage"; 
  #endif
  #ifdef ESP8266
  htmlText+="<b>ESP8266 (iSpindel):</b><br> One Wire(DS18B20 TEMP Data)=12,<br> GYRO MPU6050_SDA=0, MPU6050_SCL=2<br>";
  #endif
  htmlText+="<p>Current Pin Configuration<br>";
  #ifdef ESP32
  htmlText+="Periph Power Pin: "+String(power_pin)+"<br>";
  //htmlText+="Power Pin2 (DS18B20): "+String(power_pin2)+"<br>";
  htmlText+="Battery Voltage Pin: "+String(batvolt_pin)+"</p>";
  htmlText+="Gyro Orientation: Vcc up=1, Vcc down=0"+String(mpu_orientation)+"</p>";     
  #endif
  htmlText+="One Wire Pin: "+String(onewire_pin)+"<br>";
  htmlText+="i2c SDA Pin: "+String(i2c_sda_pin)+"<br>"; 
  htmlText+="i2C SCL Pin: "+String(i2c_scl_pin)+"</p>";
  
  htmlText+="<p><form action='/updatepinconfresults?' method ='POST'>";
  htmlText+="<table border='1' style='font-size:30pt;'>";
  #ifdef ESP32
  htmlText+="<tr><td>POWER PIN (ESP32 ONLY)</td><td><input type='number' name='Power Pin' size='5' min='0' max='40' step='1' value="+String(power_pin)+" style='height:80px;font-size:30pt;'></td></tr>";
  //htmlText+="<tr><td>POWER PIN 2 (ESP32 ONLY)</td><td><input type='number' name='Power Pin2' size='5' min='0' max='40' step='1' value="+String(power_pin2)+" style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>Battery Voltage Measure Pin (ESP32 ONLY)</td><td><input type='number' name='Battery Voltage Pin' size='5' min='0' max='40' step='1' value="+String(batvolt_pin)+" style='height:80px;font-size:30pt;'></td></tr>";
  htmlText+="<tr><td>ONE WIRE PIN (DS18B20) Temperature Sensor</td><td><input type='number' name='One Wire Pin' size='5' min='0' max='40' step='1' value="+String(onewire_pin)+" style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>i2c SDA MPU6050 GYRO</td><td><input type='number' name='i2c SDA Pin' size='5' min='0' max='40' step='1' value="+String(i2c_sda_pin)+" style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>i2c SCL MPU6050 GYRO</td><td><input type='number' name='i2c SCL' size='5' min='0' max='40' step='1' value="+String(i2c_scl_pin)+" style='height:80px;font-size:30pt;'></td></tr>";
  htmlText+="<tr><td>Gyro Orientation</td><td><input type='number' name='mpu orientation' size='5' min='0' max='40' step='1' value="+String(mpu_orientation)+" style='height:80px;font-size:30pt;'></td></tr></table>";  
  #endif
  #ifdef ESP8266
  htmlText+="<tr><td>ONE WIRE PIN (DS18B20) Temperature Sensor</td><td><input type='number' name='One Wire Pin' size='5' min='0' max='40' step='1' value="+String(onewire_pin)+" style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>i2c SDA MPU6050 GYRO</td><td><input type='number' name='i2c SDA Pin' size='5' min='0' max='40' step='1' value="+String(i2c_sda_pin)+" style='height:80px;font-size:30pt;'></td></tr>"; 
  htmlText+="<tr><td>i2c SCL MPU6050 GYRO</td><td><input type='number' name='i2c SCL' size='5' min='0' max='40' step='1' value="+String(i2c_scl_pin)+" style='height:80px;font-size:30pt;'></td></tr></table>";
  #endif
  
  htmlText+="<br><input type='submit' value='UPDATE PINS AND RESTART' style='height:80px;font-size:30pt;'>";
  htmlText+="</form></p>";         
  htmlText+=htmlMenueText;
  wm.server->send(200, "text/html", htmlText);
}
  
void handleOffsetCalibration()
{ 
  float offset=calcOffset(); 
  Serial.println("[HTTP] handle Offset Calibration");
  Serial.println("OFFSET Calculated, Send to /offsetcalibration?: "+String(offset));
  String htmlText = "<HTML><HEAD><TITLE>iTilt Offset Calibration</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText += "<BODY><h1>OFFSET CALIBRATION</h1>";
  htmlText+="<p> Calculated Offset: "+String(offset)+"</p>";
  htmlText+="<p> Insert this value, with iets sign in the <a href='/wifi?' target='_blank'>Configure WiFi</a> page.</p>";
  htmlText+=htmlMenueText;
  htmlText += "</BODY></HTML>";
  wm.server->send(200, "text/html", htmlText);
}

void handlePolynomialCalibrationResults()
{
  Serial.println("[HTTP] handle Polynomial Calibration Results");
  Serial.println("Number of server arguments: "+String(wm.server->args()));
  int n=wm.server->args()/2;
  int numargs=wm.server->args();
  Serial.println("Sample Size in handlePolynomialCalibrationResults() : "+String(n));

  String htmlText = "<HTML><HEAD><TITLE>iTilt POLYNOMIAL RESULTS</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText += "<BODY><h1>POLYNOMIAL CALIBRATION WIZARD: RESULTS</h1>";

  //  for (int i=0; i<wm.server->args()-1;i++)
  //      {
  //        text+= "Argument "+String(i)+", Argument Name: "+wm.server->argName(i)+", Argument Value: "+String(wm.server->arg(i));
  //        text+="<br>";        
  //      }

  for (int i=0; i<numargs;i++)
  {
    Serial.println("Argument number: "+String(i)+", Argument Name: "+wm.server->argName(i)+", Argument Value: "+String(wm.server->arg(i)));
  }

  
  double sampledtilt[n];
  double sampledgrav[n];
  int recordnumber=0;
  htmlText+="<table border='1'><align='right'><tr><td>TILT VALUES</td><td>GRAVITY VALUES</td></tr>";
  for (int i=0; i<numargs; i=i+2)
  {
    sampledtilt[recordnumber]=atof(wm.server->arg(i).c_str());
    sampledgrav[recordnumber]=atof(wm.server->arg(i+1).c_str());
    htmlText+="<tr><td>"+String(sampledtilt[recordnumber])+"</td><td>"+String(sampledgrav[recordnumber],3)+"</td></tr>";
    recordnumber++;
  }
  //  {
  //    sampledtilt[i]=atof(wm.server->arg(i).c_str());
  //    Serial.println("sampled tilt"+String(i)+": "+String(sampledtilt[i]));
  //    sampledgrav[i]=atof(wm.server->arg(i+n).c_str());
  //    Serial.println("sampled gravity"+String(i)+": "+String(sampledgrav[i],3)); 
  //    htmlText+="<tr><td>"+String(sampledtilt[i])+"</td><td>"+String(sampledgrav[i],3)+"</td></tr>";
  //    
  //    //htmlText+= "Sampled_Tilt"+String(sampledtilt[i]) +", Sampled_Gravity"+String(sampledgrav[i],3)+"<br>";  
  //  }
  htmlText+="<align></table>";
  int order = 3;
  double coeffs[order+1];   
  int ret = fitCurve(order, sizeof(sampledgrav)/sizeof(double), sampledtilt, sampledgrav, sizeof(coeffs)/sizeof(double), coeffs);
  if (ret==0)
  {
    Serial.println("Coefficiant of tilt^3: "+String(coeffs[0],15));
    Serial.println("Coefficiant of tilt^2: "+String(coeffs[1],15));
    Serial.println("Coefficiant of tilt^1: "+String(coeffs[2],15));
    Serial.println("Constant Term: "+String(coeffs[3],15));
    settings.coefficientx3=coeffs[0];
    settings.coefficientx2=coeffs[1];
    settings.coefficientx1=coeffs[2];
    settings.constantterm=coeffs[3];
    htmlText+="<br>Values copied to <a href='/deviceconfinput?' target='_blank'>Settings</a>. Go and SAVE it.</br>";
    htmlText+="<br>Coefficiant of tilt^3: "+String(coeffs[0],15)+"<br> Coefficiant of tilt^2: "+String(coeffs[1],15)+"<br> Coefficiant of tilt^1: "+String(coeffs[2],15)+"<br> Constant Term: "+String(coeffs[3],15);
  }
  else
  {
    Serial.println("Failed to calculate Coefficients.");
  }
  //Calculating R^2 Rsquare
  float agrav=0;  //average of gravity
  for (int i=0 ; i<n; i++)
  {
    agrav+=(1/float(n))*sampledgrav[i];
    Serial.println("Calculating agrav: "+String(agrav,5));
  }
  Serial.println("Average Gravity: "+String(agrav,8));
  float SSres=0;
  for (int i=0 ; i<n; i++)
  {
    SSres+=pow((sampledgrav[i]-(coeffs[0]*sampledtilt[i]*sampledtilt[i]*sampledtilt[i]+coeffs[1]*sampledtilt[i]*sampledtilt[i]+coeffs[2]*sampledtilt[i]+coeffs[3])),2);
  }
  Serial.println("SSres: "+String(SSres,8));
  float SStot=0;
  for (int i=0 ; i<n; i++)
  {
    SStot+=pow((sampledgrav[i]-agrav),2);
  }
  Serial.println("SStot: "+String(SStot,8));
  float Rsquare=1-SSres/SStot;
  htmlText+="<br> Coefficient of determination: "+String(Rsquare,15)+" (This measures the strenght of the statistical fit. You should get something in the range of 0.98-0.99"; 
  htmlText+=htmlMenueText; 
  htmlText+="</BODY></HTML>";
  wm.server->send(200, "text/html", htmlText);
  
}

void handlePolynomialCalibrationInput()
{   
  String htmlText="<HTML><HEAD><TITLE>iTilt Polynomial Calibration</TITLE></HEAD><style>  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; } </style>";
  htmlText+= htmlStyleText;
  htmlText += "<BODY><h1>POLYNOMIAL (MODEL) CALIBRATION WIZARD: SAMPLED DATA</h1>";
  htmlText+="<p>This wizard should assist you in calibrating the iTilt. If you are here, you may already have a calibration data sample set (ordered pairs of Tilt(Measured in <a href='/readings?'>SENSOR READINGS</a>) ";
  htmlText+="and Gravity (Measured with a Hydrometer). If not, you can do it now. Make sure your publication interval is set to 0, and portal time out is 9999. </p>";
  htmlText+="<p>We suggest you use a 3L measuring jar, boil 0.55kg sugar in 1.5L clean water. Let the mix cool down to about 20 degrees Celsius. Add your mix to the jar. ";
  htmlText+="Full the jar with extra water until it reaches 2.5L. Stir the content properly (before each measurement). Measure your Tilt, Measure you Gravity (If you have a hydro meter). If not, use the theoretical values.";
  htmlText+="<p>The instructions in the 3rd column is a guide only, you may ignore them</p>";
  htmlText+="<p>ALL RECORDS MUST BE COMLETED. The polynomial will be WRONG otherwise</p>";
  htmlText+="<p>IMPORTANT: Make sure your iTilt is free floating. It must not touch the bottom or two sides of the jar.</p>";
  htmlText+= "<p><table style='height:80px;font-size:20pt;' border='1'><tr><td><br><form action='/polynomialcalibrationresults?' method ='POST'><b> TILT:</b></td><td><b>GRAVITY:</b></td><td><b>WIZARD INSTRUCTIONS AND THEORETICAL GRAVITY</b></td></tr>";
  int n=atoi(wm.server->arg(0).c_str());
  Serial.println("Sample Size in handlePolynomialCalibrationInput() : "+String(n));
  for (int i=0; i<n; i++)
  {
    htmlText+= "<tr><td><input type='number' name=+""tilt"+String(i) +" size='5' min='12.60' max='80.00' step='0.01' style='height:80px;font-size:30pt;'></td>";
    htmlText+= "<td><input type='number' name=+""gravity"+String(i) +" size='5' min='1' max='1.12' step='0.001' style='height:80px;font-size:30pt;'></td>";
    if (i==0)
    {
     htmlText+="<td>";
     htmlText+="Your sugar content in the jar of 2.5L is 500g. The Theoretical Gravity is 1.084 SG";
     htmlText+="</td></tr>";     
    }
    else if (i==1)
    {
     htmlText+="<td>";
     htmlText+="Before any measurement, remove 600 ml of mix in jar and replace it with 600 ml clean water. ";
     htmlText+="Your sugar content in the jar of 2.5L is 410 g. The Theoretical Gravity is 1.064 SG";           
     htmlText+="</td></tr>";          
    }
    else if (i==2)
    {
     htmlText+="<td>";
     htmlText+="Before any measurement, remove 700 ml of mix in jar and replace it with 700 ml clean water. ";
     htmlText+="Your sugar content in the jar of 2.5L is 301 g. The Theoretical Gravity is 1.046 SG";       
     htmlText+="</td></tr>";          
    }
    else if (i==3)
    {
     htmlText+="<td>";
     htmlText+="Before any measurement, remove 800 ml of mix in jar and replace it with 800 ml clean water. ";
     htmlText+="Your sugar content in the jar of 2.5L is 205 g. The Theoretical Gravity is 1.032 SG";              
     htmlText+="</td></tr>";        
    } 
    else if (i==4)
    {
     htmlText+="<td>";
     htmlText+="Before any measurement, remove 900 ml of mix in jar and replace it with 900 ml clean water. ";
     htmlText+="Your sugar content in the jar of 2.5L is 131 g. The Theoretical Gravity is 1.020 SG";              
     htmlText+="</td></tr>";        
    } 
    else if (i==5)
    {
     htmlText+="<td>";
     htmlText+="Before any measurement, remove 1100 ml of mix in jar and replace it with 1100 ml clean water. ";
     htmlText+="Your sugar content in the jar of 2.5L is 73 g. The Theoretical Gravity is 1.011 SG";              
     htmlText+="</td></tr>";          
    } 
    else if (i==6)
    {
     htmlText+="<td>"; 
     htmlText+="Before any measurement, remove 1300 ml of mix in jar and replace it with 1300 ml clean water. ";
     htmlText+="Your sugar content in the jar of 2.5L is 35 g. The Theoretical Gravity is 1.005";             
     htmlText+="</td></tr>";           
    }
    else
     htmlText+="<td></td></tr>";     
  }
  htmlText += "</table><br><input type='submit' value='Submit' style='height:80px;font-size:30pt;'>  </form></p><br>";
  htmlText += "<br></BODY></HTML>";  
  wm.server->send(200, "text/html", htmlText);
}

void handlePolynomialCalibrationStart()
{
  String htmlText = "<HTML><HEAD><TITLE>iTilt Calibration Page</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText+= "<BODY><h1>POLYNOMIAL CALIBRATION WIZARD</h1>";
  htmlText+="<p> This links will not work if you are connected to the iTilt Access point.</p>";
  htmlText+="<p>Sugar Wash calculators will indicate how much sugar you need to add into a specific amount of water to reach certain gravity</p>";
  htmlText+="<p>Sugar wash Calculator at https://chasethecraft.com/calculators</p>";
  htmlText+="<p>Sugar wash Calculator at https://www.hillbillystills.com/distilling-calculator</p>";  

  htmlText+="<p>It is recomended to use a sample size of 7 and the and water volumes and sugar weights provided in the next step.</p>";  
  htmlText+="<p><form action='/polynomialcalibrationinput?' method ='POST'> SAMPLE SIZE (6-30): <input type='number' name='sample_size' size='5' min='6' max='30' step='1' style='height:80px;font-size:30pt;'><input type='submit' value='Submit' style='height:80px;font-size:30pt;'>  </form></p><br>";
  //htmlText += "<p><a href='/wifi?'>--Configure Wifi--</a><a href='/readings?'>--Sensor Readings--</a> <a href='/offsetcalibration?'>--Offset Calibration--</a><a href='/polynomialcalibrationstart?'>--Polynomial Calibration--</a><a href='/'>--Main Page--</a></p>";
  htmlText+=htmlMenueText;   
  
  htmlText += "</BODY></HTML>";  
  wm.server->send(200, "text/html", htmlText);    
}

/*void getHtmlMenu( char* buffer, size_t bufferSize )
{
  //const char* settingsText = getStringFromPROGMEM(CONFIGURE_WIFI);

  snprintf_P(buffer, bufferSize, 
    F("<a href='/wifi?' class='button'>%s</a>\
    <br><a href='/deviceconfinput?' class='button'>%s</a>\
    <br><a href='/readings?' class='button'>%s</a>\
    <br><a href='/offsetcalibration?' class='button'>%s</a>\    
    <br><a href='/polynomialcalibrationstart?' class='button'>%s</a>\
    <br><a href='/info?' class='button'>%s</a>\ 
    <br><a href='/exit?' class='button'>%s</a>\  
    <br><a href='/update?' class='button'>%s</a>\
    <br><a href='/pinconfinput?' class='button'>%s</a>"),
    LT(CONFIGURE_WIFI), LT(SETTINGS), LT(SENSOR_READINGS), LT(OFFSET_CALIBRATION), LT(POLYNOMIAL_CALIBRATION), LT(INFO), LT(EXIT), LT(FIRMWARE_UPDATE), LT(PIN_CONFIGURATION) );
}*/

void getReadingsHtml(char* buffer, size_t bufferSize, float batvolt, float batp, float tilt, float roll, float grav, float temp, float gyro_temp, float abv, float ema, float lpf)
{

  size_t offset = 0; // Текущая позиция в буфере

  offset += snprintf_P(buffer + offset, bufferSize - offset, PSTR("<HTML><HEAD><meta charset='UTF-8' http-equiv='refresh' content='1'><TITLE>%s</TITLE></HEAD>"), LT(READINGS_TITLE));
  offset += snprintf_P(buffer + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(buffer + offset, bufferSize - offset,
    PSTR("<BODY><h1>%s</h1>\
    <p>%s</p>\
    <p>%s: %.2f V</p>\
    <p>%s: %.0f %%</p>\
    <p>%s: %.2f &deg; (EMA %.2f. LPF %.2f) %s</p>\
    <p>%s: %.2f &deg;. %s</p>\
    <p>%s: %.5f SG</p>\
    <p>%s: %.2f &deg;C</p>\
    <p>%s: %.2f &deg;C</p>\
    <p>%s: %.2f %%</p>"),
   LT(READINGS_H1), LT(READINGS_UPDATE), LT(BAT_VOLTAGE), batvolt, LT(BAT_PERCENT), batp, LT(TILT), tilt, ema, lpf, LT(TILT_INFO), LT(ROLL), roll, LT(ROLL_INFO), 
   LT(GRAVITY), grav, LT(TEMPERATURE_MPU), gyro_temp, LT(TEMPERATURE_DS), temp, LT(ABV), abv);
  offset += getHtmlMenu( buffer+offset, bufferSize-offset);
  offset += snprintf_P( buffer+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); //2271
}


void handleReadings() 
{
  char html[4096];
  powerUpSensors();
  float batvolt=calcBatVolt(200);
  tilt= calcTilt(40);
  float roll= calcRoll(40);
  float grav=calcGrav(tilt);
  float abv=calcABV(grav);

  float alpha = 0.1; // Коэффициент сглаживания (чем меньше, тем сильнее сглаживание)
  //EMA Экспоненциальное скользящее среднее
  if(tilt_ema==0) tilt_ema=tilt;
  tilt_ema = alpha * tilt + (1 - alpha) * tilt_ema;
  //LPF Фильтр низких частот
  if(tilt_lpf==0) tilt_lpf=tilt;
  tilt_lpf = tilt_lpf + alpha * (tilt - tilt_lpf);


  Serial.println("[HTTP] handle Readings");
  getReadingsHtml(html, sizeof(html), batvolt, calcBatCap(batvolt), tilt, roll, grav, calcTemp(), calcGyroTemp(), abv, tilt_ema, tilt_lpf );
  /*String htmlText = "<HTML><HEAD><meta charset='UTF-8' http-equiv='refresh' content='1'><TITLE>iTilt Sensor Readings</TITLE></HEAD>";
  htmlText+= htmlStyleText;
  htmlText+= "<BODY><h1>iTilt Sensor Readings</h1>";
  htmlText+= "<p>Sensor Readings will update every 2 seconds.</p>";
  htmlText+= "<p>Battery Voltage: " + String(batvolt,2) + "</p>";
  htmlText+= "<p>Battery Remaining Capacity: " + String(calcBatCap(batvolt),0) + " %</p>";  
  htmlText+= "<p>Tilt: " + String(tilt) + " °. This value should be about 89 degrees if the iTilt is on a horizontal surface. If not, do a Tilt Ofset calibration or resolder your MPU6050 Gyroscope</p>";
  htmlText+="<p>Roll: "+String(roll)+" °. This value should be close to 90 when iTilt is free flowing in liquid. . </p>";
  htmlText+= "<p>Gravity: " + String(grav, 5) + " SG</p>";
  htmlText+= "<p>Temperature according to DS18B20 (Very accurate): " + String(calcTemp()) + " °C</p>";
  htmlText+="<p>Gyro Internal Temperature (1 degree resolution): "+String(calcGyroTemp())+" °C</p>";
  htmlText+= "<p>Alcohol by Volume (ABV): " + String(abv) + " %</p>";
  htmlText+=htmlMenueText; 
  htmlText += "</BODY></HTML>";*/
  powerDownSensors();
  wm.server->send(200, "text/html", html);
}

//callback notifying us of the need to save config
/*void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}*/

void runConfigurationPortal () {
  #ifndef ARDUINO_ESP32C3_DEV
      pinMode(LED_BUILTIN,OUTPUT);  //It seems like WiFimanager or WiFi frequently overwright this statement
    
    for (int i=0;i<10;i++)
      {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      }
 
    digitalWrite(LED_BUILTIN, HIGH);  
  #endif  

  Serial.println("iTilt will enter the WiFi configuration portal");
  Serial.println("Should run portal");

           
    String menue="<p>"+htmlMenueText+"</p>";
    WiFiManagerParameter custom_menue(menue.c_str());
    
  
    //wm.resetSettings();
    //Setting Callbacks
    //wm.setSaveConfigCallback(saveConfigCallback);
    wm.setWebServerCallback(bindServerCallback);  

  
    //add all your parameters here
    //wm.addParameter(&custom_portalTimeOut);
    wm.addParameter(&custom_menue);
    //wm.setConfigPortalTimeout(atoi(portalTimeOut));
    wm.setCustomHeadElement(htmlWiFiConfStyleText.c_str());

    String APName="iTilt_";
    Serial.println("WiFi Mac adress is: "+WiFi.macAddress());
    APName+=itiltnum;
    Serial.println("Default password: 12345678");
    if (!wm.startConfigPortal(APName.c_str(), "12345678"))
    {
      Serial.println("WiFi Manager Portal: User failed to connect to the portal or dit not save. Time Out");
      ESP.restart();
      #ifndef ARDUINO_ESP32C3_DEV
        digitalWrite(LED_BUILTIN,HIGH);
      #endif      
      delay(5000);
    }

    //read updated parameters
    //strcpy(portalTimeOut, custom_portalTimeOut.getValue());
  
    //save the custom parameters to FS
    /*if (shouldSaveConfig) {
      Serial.println("shouldSaveConfig is: "+String(shouldSaveConfig));
      Serial.println("saving config");  
      //DynamicJsonDocument json(1024);  
      //json["portalTimeOut"]   = portalTimeOut;
  
      File configFile = LittleFS.open("/wifi.json", "w");
      if (!configFile) {
        Serial.println("failed to open config file for writing");
      }
  
      serializeJson(json, Serial);
      serializeJson(json, configFile);
  
      configFile.close();
      //end save
    }    */

    //    mqttClient.setUsernamePassword(String(cloud_username), String(cloud_password));
    //    mqttClient.setId(String(cloud_clientid));
 
}

void disableGPIOHold () {
  #ifdef ESP32  //power the MPU6050 and DS18B20 from GPIO PINS OR SWITCH THEM WITH TRANSISTORS
    gpio_hold_dis(GPIO_NUM_2);
    gpio_hold_dis(GPIO_NUM_0);  //https://github.com/espressif/arduino-esp32/issues/2712 To Remove the lock on the GPIO
    //GPIO 1 and 3 ist TXD and RXD
  
    gpio_hold_dis(GPIO_NUM_4);  //https://github.com/espressif/arduino-esp32/issues/2712 To Remove the lock on the GPIO
    gpio_hold_dis(GPIO_NUM_5);  //https://github.com/espressif/arduino-esp32/issues/2712  To Remove the lock on the GPIO
    gpio_hold_dis(GPIO_NUM_12);  //https://github.com/espressif/arduino-esp32/issues/2712 
    gpio_hold_dis(GPIO_NUM_13);  //https://github.com/espressif/arduino-esp32/issues/2712 
    gpio_hold_dis (GPIO_NUM_14);  //https://github.com/espressif/arduino-esp32/issues/2712 
    gpio_hold_dis (GPIO_NUM_15);  //https://github.com/espressif/arduino-esp32/issues/2712 
    gpio_hold_dis (GPIO_NUM_16);  //https://github.com/espressif/arduino-esp32/issues/2712     
    gpio_hold_dis (GPIO_NUM_17);  //https://github.com/espressif/arduino-esp32/issues/2712 
    gpio_hold_dis (GPIO_NUM_18);  //https://github.com/espressif/arduino-esp32/issues/2712 
    gpio_hold_dis (GPIO_NUM_19);  //https://github.com/espressif/arduino-esp32/issues/2712 
    gpio_hold_dis (GPIO_NUM_21);  //https://github.com/espressif/arduino-esp32/issues/2712 
    #ifndef ARDUINO_ESP32C3_DEV    
      gpio_hold_dis (GPIO_NUM_22);  //https://github.com/espressif/arduino-esp32/issues/2712 
      gpio_hold_dis (GPIO_NUM_23);  //https://github.com/espressif/arduino-esp32/issues/2712 
      gpio_hold_dis (GPIO_NUM_25);  //https://github.com/espressif/arduino-esp32/issues/2712  
      gpio_hold_dis (GPIO_NUM_26);  //https://github.com/espressif/arduino-esp32/issues/2712  
      gpio_hold_dis (GPIO_NUM_27);  //https://github.com/espressif/arduino-esp32/issues/2712  
      gpio_hold_dis (GPIO_NUM_32);  //https://github.com/espressif/arduino-esp32/issues/2712 
      gpio_hold_dis (GPIO_NUM_33);  //https://github.com/espressif/arduino-esp32/issues/2712     
    #endif
   
  #endif
}

void powerUpSensors()
{
  #ifdef ESP32
    uint8_t pp = atoi(power_pin);
    //uint8_t pp = 5;
    //uint8_t pp2 = atoi(power_pin2);
    //Serial.println("Power Pin: "+String(pp) +" must go high");
    //Serial.println("Power Pin: "+String(pp2) +" must go high");    
    pinMode(pp,OUTPUT);
    digitalWrite(pp,HIGH);
    //pinMode(pp2,OUTPUT);
    //digitalWrite(pp2,HIGH);
  #endif
}

void powerDownSensors()
{
  #ifdef ESP32
    //uint8_t pp = 5;
    uint8_t pp = atoi(power_pin);
    //uint8_t pp2 = atoi(power_pin2);
    pinMode(pp,OUTPUT);
    digitalWrite(pp,LOW);
    //pinMode(pp2,OUTPUT);
    //digitalWrite(pp2,LOW);
  #endif
}

void readConfiguration () {

  //read configuration from FS json
  delay(2000);
  Serial.println("mounting FS...");

  if (LittleFS.begin()) {
    Serial.println("mounted file system");
    if (LittleFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file config.json");
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        //Serial.println("opening config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        if ( ! deserializeError ) {
          Serial.println("We've read:");
          serializeJson(json, Serial);
          Serial.println("");

/*
char coefficientx3[16] = "0.0000010000000"; //model to convert tilt to gravity
char coefficientx2[16] = "-0.000131373000";
char coefficientx1[16] = "0.0069679520000";
char constantterm[16] = "0.8923835598800";
char batconvfact[8]="872.48";
char pubint[6] = "600"; // publication interval in seconds
char offlinepubint[6] = "3600"; // publication try / data store when offline 
char originalgravity[6] = "1.05";
char tiltOffset[5] = "0";
char itiltnum[4]="0"; // iTilt ID
*/


          if (!json["portalTimeOut"].isNull() && strlen(json["portalTimeOut"].as<const char*>()) > 0) strcpy(portalTimeOut, json["portalTimeOut"]);
          strcpy(cloud_username, json["cloud_username"]);
          strcpy(cloud_password, json["cloud_password"]);
          //strcpy(cloud_clientid, json["cloud_clientid"]);
          //strcpy(cloud_service, json["cloud_service"]);    //*******************************************************************     
          strcpy(cloud_host, json["cloud_host"]);
          //if (!json["coefficientx3"].isNull() && strlen(json["coefficientx3"].as<const char*>()) > 0) strcpy(coefficientx3, json["coefficientx3"]);
          //if (!json["coefficientx2"].isNull() && strlen(json["coefficientx2"].as<const char*>()) > 0) strcpy(coefficientx2, json["coefficientx2"]);
          //if (!json["coefficientx1"].isNull() && strlen(json["coefficientx1"].as<const char*>()) > 0) strcpy(coefficientx1, json["coefficientx1"]);
          if (!json["coefficientx3"].isNull() && strlen(json["coefficientx3"].as<const char*>()) > 0) settings.coefficientx3 = json["coefficientx3"].as<float>();
          if (!json["coefficientx2"].isNull() && strlen(json["coefficientx2"].as<const char*>()) > 0) settings.coefficientx2 = json["coefficientx2"].as<float>();
          if (!json["coefficientx1"].isNull() && strlen(json["coefficientx1"].as<const char*>()) > 0) settings.coefficientx1 = json["coefficientx1"].as<float>();
          if (!json["constantterm"].isNull() && strlen(json["constantterm"].as<const char*>()) > 0) settings.constantterm = json["constantterm"].as<float>();
          if (!json["batconvfact"].isNull() && strlen(json["batconvfact"].as<const char*>()) > 0) settings.batconvfact = json["batconvfact"].as<float>();
          if (!json["pubint"].isNull() && strlen(json["pubint"].as<const char*>()) > 0) settings.pubint = json["pubint"].as<uint32_t>;
          if (!json["offlinepubint"].isNull() && strlen(json["offlinepubint"].as<const char*>()) > 0) settings.offlinepubint = json["offlinepubint"].as<uint32_t>();
          if (!json["originalgravity"].isNull() && strlen(json["originalgravity"].as<const char*>()) > 0) settings.originalgravity = json["originalgravity"].as<float>();
          if (!json["tiltOffset"].isNull() && strlen(json["tiltOffset"].as<const char*>()) > 0) strcpy(tiltOffset, json["tiltOffset"]);
          if (!json["itiltnum"].isNull() && strlen(json["itiltnum"].as<const char*>()) > 0) strcpy(itiltnum,json["itiltnum"]);
          //strcpy(dummy,json["dummy"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
    else{
      Serial.println("no config file config.json");
    }

    //add wifi.json reading.

    if (LittleFS.exists("/pinconfig.json")) {
      //file exists, reading and loading
      Serial.println("reading Pin config file");
      File pinConfigFile = LittleFS.open("/pinconfig.json", "r");
      if (pinConfigFile) {
        Serial.println("opened Pin config file");
        size_t size = pinConfigFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        pinConfigFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(1024*2);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {

          Serial.println("\nparsed json");
          #ifdef ESP32
          strcpy(power_pin, json["power_pin"]);
          //strcpy(power_pin2, json["power_pin2"]);
          strcpy(batvolt_pin, json["batvolt_pin"]);
          strcpy(onewire_pin, json["onewire_pin"]);          
          strcpy(i2c_sda_pin, json["i2c_sda_pin"]);
          strcpy(i2c_scl_pin, json["i2c_scl_pin"]);
          strcpy(mpu_orientation, json["mpu_orientation"]);                  
          #endif
          #ifdef ESP8266
          strcpy(onewire_pin, json["onewire_pin"]);          
          strcpy(i2c_sda_pin, json["i2c_sda_pin"]);
          strcpy(i2c_scl_pin, json["i2c_scl_pin"]);
          #endif

        } else {
          Serial.println("failed to load json pinconfig");
        }
        pinConfigFile.close();
      }
    }
    else{
      Serial.println("no config file pinconfig.json");
    }    

    
  } else {
    Serial.println("failed to mount FS");  //This hapen with new ESP32 KOALA WROVER-Format Spifs?
    LittleFS.format();  //May be needed to format ESP32 with MicroPython or iSpindel Firmware
  }  
}

bool connectToWiFi () {
  Serial.print("\nConnecting to WiFi ..");
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  
  for (int i=0;i<60;i++)  //Check if WiFi is connected
  {
    Serial.print(".");
    if (WiFi.status()==WL_CONNECTED)
    {
      break;
    }
    if (i==44)
    {
      Serial.println("\nThe iTilt Could not connect to your WiFi Access Point. Deep Sleep will start...");
      return false;
    }
    delay(200);
  }

  Serial.println("The iTilt connected to your WiFi access point");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());  
  return true;
}

/*void sendData ( float batcap, float batvolt, float tilt, float roll, float grav, float abv, float temp, float tempgyro, float signalstrength, uint32_t seconds) {
  Serial.println("Selected Cloud Service: "+String(cloud_service));
  if (String(cloud_service)=="CAYENNE"){
    pubToCayenne(String(batcap,4),String(tempgyro,2), String(temp,4),String(tilt,4),String(batvolt,4),String(grav,4), String(abv,4), String(signalstrength), String(cloud_username), String(cloud_password), String(cloud_clientid));
  }
  else if (String(cloud_service)=="UBIDOTS"){
    pubToUbidots(String(batcap,2),String(tempgyro,2), String(temp,2),String(tilt,1),String(batvolt,2),String(grav,4), String(abv,2), String(signalstrength), String(cloud_username), String(cloud_password), String(cloud_clientid));
  }
  else if (String(cloud_service)=="ADAFRUIT"){
    pubToAdafruit(String(batcap,4),String(tempgyro,2), String(temp,4),String(tilt,4),String(batvolt,4),String(grav,4), String(abv,4), String(signalstrength), String(cloud_username), String(cloud_password), String(cloud_clientid));
  }  
  //else if (String(cloud_service)=="COG"){
  //  //pubToCOG(String(batcap,4),String(tempgyro,2), String(temp,4),String(tilt,4),String(batvolt,4),String(grav,4), String(abv,4), String(signalstrength), String(cloud_username), String(cloud_password))
  //  pubToCOG( itiltnum, cloud_username, cloud_password, batvolt,  grav, temp, signalstrength, seconds ); 
  //}
}*/


// Функция для проверки свободного места
uint32_t getFreeSpace() {
  uint32_t totalBytes = LittleFS.totalBytes(); // Общее количество байт на файловой системе
  uint32_t usedBytes = LittleFS.usedBytes();   // Занятое количество байт
  return totalBytes - usedBytes;             // Свободное место
}

SensorData prepareSensorDataForUpload( float batvolt,  float grav, float temp,  float signal_strength, uint32_t seconds )
{
  SensorData data;
  data.temp = temp*100+0.5;
  data.gravity = grav*10000+0.5;
  data.batvolt = batvolt*100+0.5;
  data.signal_strength = signal_strength+0.5;
  data.seconds = seconds;
  return data;  
}

bool storeData( float batvolt,  float grav, float temp,  float signal_strength, uint32_t seconds )
{

  //на С3 выделено 1441792 байт. При структуре 11байт этого хватит на год записей раз в 4 минуты.

  SensorData data = prepareSensorDataForUpload( batvolt, grav, temp, signal_strength, seconds );

  // Подключаем файловую систему
  /*if (!LittleFS.begin()) {
    Serial.println("Ошибка инициализации LittleFS");
    return false;
  } */ // begin в readConfiguration 
  
  uint32_t freeBytes = getFreeSpace();

  if (freeBytes < sizeof(SensorData) ) {
    Serial.println("Недостаточно места для записи одной структуры в файл.");
    return false;
  }


  // Запись данных в файл
  File file = LittleFS.open("/sensor_data.bin", "ab");
  if (!file) {
    Serial.println("Не удалось открыть файл для записи");
    return false;
  }  

  size_t bytesWritten = file.write((uint8_t*)&data, sizeof(data));
  file.close();  

  if (bytesWritten == sizeof(SensorData)) {
    Serial.println("Данные успешно записаны в файл");
    return true;
  } else {
    Serial.println("Ошибка записи данных в файл");
    return false;
  }

}

void setup() 
{ 
  float signalstrength;
  float roll;
  float grav;
  float abv;
  float temp;
  float tempgyro; 
  float batvolt;
  float batcap;

  //double dpubint;  // saved measurement/publication interval (sec)
  uint32_t dofflineint; //saved offline measurement/publication interval (sec)
  uint32_t interval; //interval var


  bool has_queue;
  uint16_t ditiltnum; //itilt id
 

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("The iTilt is starting up....");

  accelgyro.setSleepEnabled(true);

  delay(100);  //This is for accurate battery voltage reading
  #ifndef ARDUINO_ESP32C3_DEV
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN,HIGH);
  #endif  

  //clean FS, for testing
  //LittleFS.format();
  //wm.resetSettings();
  readConfiguration();
  //dpubint = atoi(pubint);  
  dofflineint = settings.offlinepubint;

  disableGPIOHold();

//..
  while(true){ //no sleep if need readings faster than 60s

    powerUpSensors();
    signalstrength=(WiFi.RSSI()+90)*1.80;
    if (signalstrength>100) signalstrength=100;
    tilt=calcTilt(50);
    roll=calcRoll(20);
    if (roll<20 or roll>160) {
      inviniteSleep();
    }
    grav=calcGrav(tilt);
    abv=calcABV(grav);
    temp=calcTemp();
    tempgyro=calcGyroTemp();  
    powerDownSensors();

    batvolt=calcBatVolt(500);
    if (batvolt<3)  {
      Serial.println("Battery voltage is less than 3 volt. Recharge the battery or fix the battery conversion factor.");
      inviniteSleep();
    }
    batcap=calcBatCap(batvolt);
    //Serial.println("LED_BUILTIN:"+String(LED_BUILTIN));
    Serial.println("You are in setup(), Tilt is: "+String(tilt)+ ", If tilt <12 degrees, between 85 and 95 degress or nan, The Wifi Manager configuration portal will run");
    if ((tilt<95 and tilt>85) or tilt<12 or String(tilt)=="nan")  //Condition to run The Wifi Manager portal.  
    {
      runConfigurationPortal();
      powerUpSensors();
      tilt=calcTilt(50);
      grav=calcGrav(tilt);
      abv=calcABV(grav);
      temp=calcTemp();
      tempgyro=calcGyroTemp();
      batvolt=calcBatVolt(200);
      batcap=calcBatCap(batvolt); 
      powerDownSensors();
      //dpubint = atoi(pubint);  
      dofflineint = settings.offlinepubint;
    }
    else  //This is when tilt>12, nan or between 85 and 95 degrees and setup should not run
    {
      Serial.println("Should not run portal");
      #ifndef ARDUINO_ESP32C3_DEV
        digitalWrite(LED_BUILTIN,HIGH);
      #endif      
    }
    if (tilt>85)
    {
      Serial.println("Tilt is larger than 85 degrees. It seems like it is not in your brew yet. ESP will restart");
      ESP.restart();
    }


    if( dofflineint < settings.pubint ) dofflineint = settings.pubint;

    //uint32_t now = rtc.getEpoch();
    uint32_t now = time(nullptr); // faster
    //Serial.println("time:"+String(time(nullptr)) +"; Epoch:"+String(now) );
    bool sending_failed = false;
    bool data_saved = false;

    if( connection_missing_count!=0 ) has_queue=true; 
    else has_queue=false;
    if ( connectToWiFi() ){
      ditiltnum = atoi(itiltnum);
      
      if( has_queue == false ) {
        if( !pubReadingToCOG(ditiltnum, cloud_username, cloud_password, batvolt, grav, temp, signalstrength, now ) ) {
          sending_failed=true;
        }
      }
      else {
        storeData(  batvolt,  grav, temp,  signalstrength, now );
        data_saved = true;
        if( !pubFileToCOG(ditiltnum, cloud_username, cloud_password) )  {
          sending_failed=true;
        }
      }
    }
    else{
      sending_failed=true;
    }

    if( sending_failed ){
      connection_missing_count++;
      Serial.println("connection_missing_count:"+String(connection_missing_count));      
      if( connection_missing_count >= dofflineint/settings.pubint ){
        //offline mode
        interval = dofflineint;
        connection_missing_count = dofflineint; // excepts overflow
      }
      else{
        interval = settings.pubint;
        connection_missing_count = 0;
      }
      if(!data_saved) storeData(  batvolt,  grav, temp,  signalstrength, now );
    }
    else{
      interval = settings.pubint;
    }
    //Serial.println("sleep:"+String(interval));
    startDeepSleep(interval);
  }
}

void loop()
{
  //void loop() should never execute

}
