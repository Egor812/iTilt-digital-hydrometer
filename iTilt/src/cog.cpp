#include "cog.h"
#include "main.h"
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>

float getRSSI()
{
    float signalstrength=(WiFi.RSSI()+90)*1.80;
    if (signalstrength>100) signalstrength=100;
    return signalstrength;
}

bool connectToWiFi () 
{
  Serial.print("\nConnecting to WiFi ..");
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  
  for (int i=0;i<60;i++)  //Check if WiFi is connected
  {
    Serial.print(".");
    if (WiFi.status()==WL_CONNECTED) {
      break;
    }
    if (i==44) {
      Serial.println("\nThe iTilt Could not connect to your WiFi Access Point. Deep Sleep will start...");
      return false;
    }
    delay(200);
  }

  Serial.println("The iTilt connected to your WiFi access point");
  Serial.print("Local ip: ");
  Serial.println(WiFi.localIP());  
  return true;
}


bool connectCOG( WiFiClient& client, uint16_t tilt_id, const char cloud_username[], const char cloud_password[], uint16_t qty )
{
  //const char*  server = "cog.arkhipy1.beget.tech";  // Server URL
  int port = 80;
  // WiFiClientSecure client; //TODO change to secure when https
  //client.setInsecure();
  //int port = 443;

  Serial.print("\nStarting connection to server....");
  if (!client.connect(settings.cloud_host, port)){
    Serial.println("failed");
    return false;
  }
  Serial.println("done");

  String authData = String(tilt_id) + "," + String(cloud_username) + "," + String(cloud_password) + "\n";
  uint8_t authDataSize = authData.length();
  uint8_t payloadSize = authDataSize + (qty * sizeof(SensorData));

  // Make a HTTP request:
  client.println("POST /rx HTTP/1.1");
  client.println("Host: " + String(settings.cloud_host) );
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

  Serial.print("Размер файла: ");
  Serial.print(file.size());
  Serial.print(" Записей: ");
  Serial.println(rows);

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

  DEBUG_PRINTLN( "We've got:"+String(temp)+", "+String(batvolt)+", "+String(grav)+", "+String(signal_strength)+", "+String(seconds));
  SensorData data = prepareSensorDataForUpload( batvolt, grav, temp, signal_strength, seconds );
  DEBUG_PRINTLN( "We're sending:"+String(data.temp)+", "+String(data.batvolt)+", "+String(data.gravity)+", "+String(data.signal_strength)+", "+String(data.seconds));

  client.write((uint8_t*)&data, 1 * sizeof(SensorData));

  if( !getResponseCodeCOG( client ) ) return false;

  getResponseCOG( client );
  closeConnectionCOG( client );
  return true;
}

SensorData prepareSensorDataForUpload( float batvolt,  float grav, float temp,  float signal_strength, uint32_t seconds )
{
  SensorData data;
  data.temp = temp*100;
  data.gravity = grav*10000;
  data.batvolt = batvolt*100;
  data.signal_strength = signal_strength;
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