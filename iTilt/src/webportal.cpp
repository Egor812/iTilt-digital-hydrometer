#include "webportal.h"
#include "main.h"
#include "tpl.h"
#include <pgmspace.h>
#include <WiFi.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <LittleFS.h>             //https://github.com/littlefs-project/littlefs/blob/master/README.md
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <curveFitting.h>  //used for polynomial calibration v1.06 from Arduino IDE
//#include "esp32-hal-timer.h"


// наш режим - configportal

WiFiManager wm;

bool portalRunning      = false;


int16_t dataSubscribesCount = 0;
//hw_timer_t* timer = nullptr;

/*void IRAM_ATTR onTimer() {
    dataSubscribesCount--;
    if( dataSubscribesCount == 0 ) {
        timerStop(timer); //!!! просто timerAlarmDisable не работает
        Serial.println("****"); // Для отладки
        //finishTiltReadings(); // завешивает
        powerDownSensors();
    }
    //get tilt
    if(acc_status==2){
        readTilt(1);
    }
    Serial.println(dataSubscribesCount);
}*/

/*class Webportal {
    private:
        uint8_t language;
        WiFiManager wm;
    
    public:
        Webportal(uint8_t lang);
        const char* getStringFromPROGMEM(StringIndex index) const;
        void bindServerCallback();
        size_t getHtmlMenu( char* buffer, size_t bufferSize );
        void handleRoute(void);
        void handleNotFound(void);
        void handleFavicon();
        //int GetHours() const;

        #define LT(x) getStringFromPROGMEM(x)

};*/

/*Webportal::Webportal(uint8_t lang) 
{
    language = lang;
}*/

// Функция для чтения строки из PROGMEM
const char* getStringFromPROGMEM(StringIndex index)
{
    return (const char*)pgm_read_ptr(&languages[settings.language][index]);
}
#define LT(x) getStringFromPROGMEM(x)


void bindServerCallback() 
{
  wm.server->on("/readings", handleReadings);
  wm.server->on("/offsetcalibration", handleOffsetCalibration);
  wm.server->on("/polynomialcalibrationstart", handlePolynomialCalibrationStart);
  wm.server->on("/polynomialcalibrationinput",handlePolynomialCalibrationInput);
  wm.server->on("/polynomialcalibrationresults",handlePolynomialCalibrationResults);
  wm.server->on("/pinconfinput",handlePinConfInput);
  wm.server->on("/updatepinconfresults",handlePinConfResults);
  wm.server->on("/deviceconfinput",handleDeviceConfInput);
  wm.server->on("/updatedeviceconfresults",handleDeviceConfResults);
  //wm.server->on("/", [this]() { this->handleRoute();} ); // you can override wm! main page
  wm.server->on("/", handleRoute ); // you can override wm! main page
  wm.server->on("/exit",handleExit);
  wm.server->on("/info",handleNotFound);
  wm.server->on("/update",handleNotFound);
  wm.server->on("/erase",handleNotFound);
  wm.server->on("/favicon.ico", handleFavicon); // Регистрируем обработчик*/
  // Обработчик для данных
  wm.server->on("/data", []() {
     if(acc_status==2){
        dataSubscribesCount = 2000; // обновляем подписку
        static unsigned long startTime = millis();
        String json = "{\"t\":" + String((millis() - startTime) / 1000.0, 1) + ",\"v\":" + String(tilt) + ",\"f\":" + String(tilt_ema) + "}";
        wm.server->send(200, "application/json", json);
     }
     else{
        if(acc_status==0) powerUpSensors();
        if(acc_status==1) initMPU(5);
      }

  });
    // Обработчик для uPlot.iife.min.js
  wm.server->on("/uPlot.iife.min.js", []() {
      File file = LittleFS.open("/uPlot.iife.min.js", "r");
      if (!file) {
        wm.server->send(500, "text/plain", "Ошибка загрузки uPlot.min.js");
        return;
      }
      wm.server->streamFile(file, "application/javascript");
      file.close();
  });

  // Обработчик для uPlot.min.css
  wm.server->on("/uPlot.min.css", []() {
      File file = LittleFS.open("/uPlot.min.css", "r");
      if (!file) {
        wm.server->send(500, "text/plain", "Ошибка загрузки uPlot.min.css");
        return;
      }
      wm.server->streamFile(file, "text/css");
      file.close();
  });

  wm.server->onNotFound(handleNotFound); //не работает

}


size_t getHtmlMenu( char* buffer, size_t bufferSize )
{
   return snprintf_P(buffer, bufferSize, 
    PSTR("<a href='/wifi?' class='button'>%s</a>\
    <br><a href='/deviceconfinput?' class='button'>%s</a>\
    <br><a href='/readings?' class='button'>%s</a>\
    <br><a href='/offsetcalibration?' class='button'>%s</a>\
    <br><a href='/polynomialcalibrationstart?' class='button'>%s</a>\
    <br><a href='/exit?' class='button'>%s</a>\
    <br><a href='/pinconfinput?' class='button'>%s</a>"),
    LT(CONFIGURE_WIFI), LT(SETTINGS), LT(SENSOR_READINGS), LT(OFFSET_CALIBRATION), LT(POLYNOMIAL_CALIBRATION), LT(EXIT), LT(PIN_CONFIGURATION) );
    //<br><a href='/info?' class='button'>%s</a>
    //    <br><a href='/update?' class='button'>%s</a>
}

void handleRoute(void)
{
  size_t offset = 0; // Текущая позиция в буфере
  u_int16_t bufferSize = 2048;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  }  

  Serial.println("[HTTP] handle Route");
  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(MAIN_PAGE_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<BODY><h1>%s</h1>"), LT(MAIN_PAGE_H1));
  offset += getHtmlMenu( html+offset, bufferSize-offset);
  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset);
  wm.server->send(200, "text/html", html);
  free(html);
}

void handleNotFound(void)
{
    Serial.println("[HTTP] 404");
    Serial.println("Received request:");
    Serial.println("URI: " + wm.server->uri());
    Serial.println("Method: " + String(wm.server->method() == HTTP_GET ? "GET" : "POST"));
    for (uint8_t i = 0; i < wm.server->args(); i++) {
        Serial.println(" " + wm.server->argName(i) + ": " + wm.server->arg(i));
    }
    wm.server->send(404, "text/plain", "Not Found");
}
  
void handleFavicon() {
    wm.server->send(204, "image/x-icon", "");
}

void handleDeviceConfResults()
{
  bool error = false;

  Serial.println("[HTTP] handle DeviceConfResults");
  Serial.println("saving device config");  
  DynamicJsonDocument json(1024);
  json["cloud_host"]   = wm.server->arg(0);
  json["cloud_username"]   = wm.server->arg(1);
  json["cloud_password"]   = wm.server->arg(2);    
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
  json["language"]=wm.server->arg(14);

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
  if( strlen(json["language"].as<const char*>())==0 ) error=true;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(json, Serial);
  serializeJson(json, configFile);

  configFile.close(); 

  //update settings vars
  strcpy(settings.cloud_host, json["cloud_host"]);
  strcpy(settings.cloud_username, json["cloud_username"]);
  strcpy(settings.cloud_password, json["cloud_password"]);
  settings.coefficientx1 = json["coefficientx1"].as<float>();
  settings.coefficientx2 = json["coefficientx2"].as<float>();
  settings.coefficientx3 = json["coefficientx3"].as<float>();
  settings.constantterm = json["constantterm"].as<float>();
  settings.batconvfact = json["batconvfact"].as<float>();
  settings.pubint = json["pubint"].as<uint32_t>();
  settings.offlinepubint = json["offlinepubint"].as<uint32_t>();
  settings.itiltnum = json["itiltnum"].as<uint16_t>();
  settings.tiltOffset = json["tiltOffset"].as<float>();
  settings.originalgravity = json["originalgravity"].as<float>();
  settings.portalTimeOut = json["portalTimeOut"].as<uint16_t>();
  settings.language = json["language"].as<uint8_t>();

  size_t offset = 0; // Текущая позиция в буфере
  u_int16_t bufferSize = 3072;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  }  

  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(SETTINGS_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<body><h1>%s</h1>"), LT(SETTINGS_UPDATED));
  if( error ) offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<p style='color:red'>%s</p>"), LT(SETTINGS_ERROR));
  offset += snprintf_P(html + offset, bufferSize - offset, 
    PSTR("<ul>\
      <li>%s: %s</li>\
      <li>%s: %s</li>\
      <li>%s: %s</li>\
      <li>%s: %.15f</li>\
      <li>%s: %.15f</li>\
      <li>%s: %.15f</li>\
      <li>%s: %.15f</li>\
      <li>%s: %.3f</li>\
      <li>%s: %i</li>\
      <li>%s: %i</li>\
      <li>%s: %.3f</li>\
      <li>%s: %.2f</li>\
      <li>%s: %i</li>\
      <li>%s: %i</li>\
      <li>%s: %i (0-English, 1-Русский)</li>\
    </ul>\
    <a class='back' href='/'>%s</a>"), 
  LT(SERVER), settings.cloud_host,
  LT(USERNAME), settings.cloud_username,
  LT(PASSWORD), settings.cloud_password,
  LT(K_T3), settings.coefficientx3,
  LT(K_T2), settings.coefficientx2,
  LT(K_T1), settings.coefficientx1,
  LT(CONSTANT_TERM), settings.constantterm,
  LT(K_BATTERY), settings.batconvfact,
  LT(DATA_PUBLICATION), settings.pubint,
  LT(DATA_PUBLICATION_OFFLINE), settings.offlinepubint,
  LT(OG), settings.originalgravity,
  LT(TILT_OFFSET), settings.tiltOffset,
  LT(ITILT_ID), settings.itiltnum,
  LT(CONFIGURATION_PORTAL_TIMEOUT), settings.portalTimeOut,  
  LT(LANGUAGE), settings.language,
  LT(BACK) 
  );
  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); 
  wm.server->send(200, "text/html", html);
  free(html);
}

void handlePinConfResults()
{
  Serial.println("[HTTP] handle PinConfResults");
  Serial.println("saving pinconfig");  
  DynamicJsonDocument json(1024);
 
  json["power_pin"]   = wm.server->arg(0);
  //json["power_pin2"]   = wm.server->arg(1);
  json["batvolt_pin"]   = wm.server->arg(1);    
  json["onewire_pin"] =wm.server->arg(2);
  json["i2c_sda_pin"]   =wm.server->arg(3);
  json["i2c_scl_pin"]   = wm.server->arg(4);
  json["mpu_orientation"]=wm.server->arg(5);

  //save to file
  File pinConfigFile = LittleFS.open("/pinconfig.json", "w");
  if (!pinConfigFile) {
    Serial.println("failed to open pinconfig file for writing");
  }
  serializeJson(json, Serial);
  serializeJson(json, pinConfigFile);
  pinConfigFile.close(); 

  size_t offset = 0; // Текущая позиция в буфере
  u_int16_t bufferSize = 2048;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  }  

  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(PIN_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<body><h1>%s</h1>"), LT(PIN_UPDATED));
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<ul>") );
  for( uint8_t c=0;c<7;c++)  {
    offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<li>>%s %s</li>"), wm.server->argName(c), wm.server->arg(c) );
  }
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("</ul><p>%s</p>"), LT(RESTART30) );

  //offset += getHtmlMenu( html+offset, bufferSize-offset);
  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); //?
  wm.server->send(200, "text/html", html);
  free(html);
  delay(2000);
  ESP.restart();
}

void handleDeviceConfInput()
{
  Serial.println("[HTTP] handle DeviceConfInput");

  u_int16_t bufferSize = 4096;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  }  

  size_t offset = 0; // Текущая позиция в буфере
  //Serial.println(ESP.getFreeHeap());

  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(SETTINGS_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<body><h1>%s</h1>"), LT(SETTINGS_H1));
  offset += snprintf_P(html + offset, bufferSize - offset, 
  PSTR("<p><form action='/updatedeviceconfresults?' method ='POST'>\
  <table>\
  <tr><td>%s</td><td><input type='text' name='service' size='32' value='%s'></td></tr>\
  <tr><td>%s</td><td><input type='text' name='usernname' size='32' value='%s'></td></tr>\
  <tr><td>%s</td><td><input type='text' name='password' size='32' value='%s'></td></tr></table>\
  <p><font size='4'><b>%s</b></font></p>\
  <table>\
  <tr><td>%s</td><td><input type='text' inputmode='decimal' name='coefficientx3' pattern='^[-+]?[0-9]*[.,]?[0-9]*' size='18' value='%.15f'></td></tr>\
  <tr><td>%s</td><td><input type='text' inputmode='decimal' name='coefficientx2' pattern='^[-+]?[0-9]*[.,]?[0-9]*' size='18' value='%.15f'></td></tr>\
  <tr><td>%s</td><td><input type='text' inputmode='decimal' name='coefficientx1' pattern='^[-+]?[0-9]*[.,]?[0-9]*' size='18' value='%.15f'></td></tr>\
  <tr><td>%s</td><td><input type='text' inputmode='decimal' name='constantterm' pattern='^[-+]?[0-9]*[.,]?[0-9]*' size='18' value='%.15f'></td></tr></table>"),
  LT(SERVER), settings.cloud_host,
  LT(USERNAME), settings.cloud_username,
  LT(PASSWORD), settings.cloud_password,
  LT(POLINOMIAL_PARAMETERS),
  LT(K_T3), settings.coefficientx3,
  LT(K_T2), settings.coefficientx2,
  LT(K_T1), settings.coefficientx1,
  LT(CONSTANT_TERM), settings.constantterm);

  offset += snprintf_P(html + offset, bufferSize - offset, 
  PSTR("<p><font size='4'><b>%s</b></font></p>\
  <table>\
  <tr><td>%s</td><td><input type='text' inputmode='decimal' pattern='^[-+]?[0-9]*[.,]?[0-9]*' name='batconvfact' size='8' value='%.3f'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='pubint' size='6' value='%i'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='offlinepubint' size='6' value='%i'></td></tr>\
  <tr><td>%s</td><td><input type='text' inputmode='decimal' pattern='^[-+]?[0-9]*[.,]?[0-9]*' name='originalgravity' size='6' value='%.3f'></td></tr>\
  <tr><td>%s</td><td><input type='text' inputmode='decimal' pattern='^[-+]?[0-9]*[.,]?[0-9]*' name='tiltOffset' size='5' value='%.2f'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='itiltnum' size='4' value='%i'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='portalTimeOut' size='5' value='%i'></td></tr>\
  <tr><td>%s (0-Eng, 1-Рус)</td><td><input type='number' name='language' size='1' value='%i'></td></tr>\
  </table><br><input type='submit' value='%s' class='save'></form></p>\
  <a class='back' href='/'>%s</a>"),
  LT(OTHER_PARAMETERS),
  LT(K_BATTERY), settings.batconvfact,
  LT(DATA_PUBLICATION), settings.pubint,
  LT(DATA_PUBLICATION_OFFLINE), settings.offlinepubint,
  LT(OG), settings.originalgravity,
  LT(TILT_OFFSET), settings.tiltOffset,
  LT(ITILT_ID), settings.itiltnum,
  LT(CONFIGURATION_PORTAL_TIMEOUT), settings.portalTimeOut,
  LT(LANGUAGE), settings.language,
  LT(SUBMIT), LT(BACK) );
  //offset += getHtmlMenu( html+offset, bufferSize-offset);
  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght: ");
  Serial.println(offset); //3830
  wm.server->send(200, "text/html", html);  
  free(html); // Освобождаем память
}

void handlePinConfInput()
{
  Serial.println("[HTTP] handle PinConfInput");
  size_t offset = 0; // Текущая позиция в буфере
  u_int16_t bufferSize = 4096;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  }  

  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(PIN_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<body><h1>%s</h1>"), LT(PIN_H1));
  offset += snprintf_P(html + offset, bufferSize - offset, 
  PSTR("<p>%s</p>\
  <p>%s:</p>\
  <ul>\
  <li>%s: %i</li>\
  <li>%s: %i</li>\
  <li>%s: %i</li>\
  <li>%s: %i</li>\
  <li>%s: %i</li>\
  <li>%s: %i</li>\
  </ul>"),
  LT(PIN_P), LT(PIN_CURRENT), 
  LT(PIN_PERIPH), hardware.power_pin,
  LT(PIN_VCC), hardware.batvolt_pin,
  LT(PIN_ONEWIRE), hardware.onewire_pin,
  LT(PIN_SDA), hardware.i2c_sda_pin,
  LT(PIN_SCL), hardware.i2c_scl_pin,
  LT(PIN_GYRO_ORIENTATION), hardware.mpu_orientation
  );

  offset += snprintf_P(html + offset, bufferSize - offset,
  PSTR("<p><form action='/updatepinconfresults?' method ='POST'>\
  <table>\
  <tr><td>%s</td><td><input type='number' name='Power Pin' size='5' min='0' max='40' step='1' value='%i'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='Battery Voltage Pin' size='5' min='0' max='40' step='1' value='%i'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='One Wire Pin' size='5' min='0' max='40' step='1' value='%i'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='i2c SDA Pin' size='5' min='0' max='40' step='1' value='%i'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='i2c SCL' size='5' min='0' max='40' step='1' value='%i'></td></tr>\
  <tr><td>%s</td><td><input type='number' name='mpu orientation' size='5' min='0' max='40' step='1' value='%i'></td></tr>\
  </table>\
  <br><input type='submit' value='%s' class='save'>\
  </form>\
  <a class='back' href='/'>%s</a></p>"),
  LT(PIN_PERIPH), hardware.power_pin,
  LT(PIN_VCC), hardware.batvolt_pin,
  LT(PIN_ONEWIRE), hardware.onewire_pin,
  LT(PIN_SDA), hardware.i2c_sda_pin,
  LT(PIN_SCL), hardware.i2c_scl_pin,
  LT(PIN_GYRO_ORIENTATION), hardware.mpu_orientation,
  LT(UPDATE_RESTART),
  LT(BACK)
  );
  //offset += getHtmlMenu( html+offset, bufferSize-offset);
  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); //ru 2717
  wm.server->send(200, "text/html", html);  
  free(html); // Освобождаем память
}
  
void handleOffsetCalibration()
{ 
  size_t offset = 0; // Текущая позиция в буфере
  u_int16_t bufferSize = 2048;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  }  
  powerUpSensors();
  float coffset=calcOffset(); 
  powerDownSensors();

  Serial.println("[HTTP] handle Offset Calibration");
  Serial.print("OFFSET Calculated, Save in /deviceconfinput?: ");
  Serial.println(coffset);

  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(OFFSET_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<body><h1>%s</h1>"), LT(OFFSET_H1));
  offset += snprintf_P(html + offset, bufferSize - offset, 
  PSTR("<p>%s: %.4f</p>\
  <p>%s <a href='/deviceconfinput?' target='_blank'>%s</a></p>\
  <a class='back' href='/'>%s</a>"),
  LT(OFFSET_CALCULATED), coffset, LT(OFFSET_COPY), LT(SETTINGS), LT(BACK));
  //offset += getHtmlMenu( html+offset, bufferSize-offset);
  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); // ru 1324
  wm.server->send(200, "text/html", html);
  free(html);
}

void handlePolynomialCalibrationResults()
{
  Serial.println("[HTTP] handle Polynomial Calibration Results");
  size_t offset = 0; // Текущая позиция в буфере
  u_int16_t bufferSize = 4096;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  } 

  Serial.println("Number of server arguments: "+String(wm.server->args()));
  int n=wm.server->args()/2;
  int numargs=wm.server->args();
  Serial.println("Sample Size in handlePolynomialCalibrationResults() : "+String(n));

  for (int i=0; i<numargs;i++)  {
    Serial.println("Argument number: "+String(i)+", Argument Name: "+wm.server->argName(i)+", Argument Value: "+String(wm.server->arg(i)));
  }
  
  double sampledtilt[n];
  double sampledgrav[n];
  int recordnumber=0;

  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(POLYNOM_RESULTS_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<body><h1>%s</h1>"), LT(POLYNOM_RESULTS_H1));
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<table border='1'><align='right'><tr><td>%s</td><td>%s</td></tr>"), LT(TILT_VAL), LT(GRAVITY_VAL));

  for (int i=0; i<numargs; i=i+2) {
    sampledtilt[recordnumber]=atof(wm.server->arg(i).c_str());
    sampledgrav[recordnumber]=atof(wm.server->arg(i+1).c_str());
    offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<tr><td>%f</td><td>%.3f</td></tr>"), sampledtilt[recordnumber],sampledgrav[recordnumber] );
    recordnumber++;
  }
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("</align></table>") );

  int order = 3;
  double coeffs[order+1];   
  int ret = fitCurve(order, sizeof(sampledgrav)/sizeof(double), sampledtilt, sampledgrav, sizeof(coeffs)/sizeof(double), coeffs);
  if (ret==0) {
    Serial.println("Coefficiant of tilt^3: "+String(coeffs[0],15));
    Serial.println("Coefficiant of tilt^2: "+String(coeffs[1],15));
    Serial.println("Coefficiant of tilt^1: "+String(coeffs[2],15));
    Serial.println("Constant Term: "+String(coeffs[3],15));
    settings.coefficientx3=coeffs[0];
    settings.coefficientx2=coeffs[1];
    settings.coefficientx1=coeffs[2];
    settings.constantterm=coeffs[3];
    offset += snprintf_P(html + offset, bufferSize - offset, 
      PSTR("<br>%s <a href='/deviceconfinput?' target='_blank'>%s</a>. %s.</br>\
      <br>%s: %f <br> %s: %f <br> %s: %f <br> %s: %f <br>"), 
      LT(POLYNOM_VAL_COPIED), LT(SETTINGS), LT(POLYNOM_GO_AND_SAVE), 
      LT(K_T3), settings.coefficientx3, LT(K_T2), settings.coefficientx2, LT(K_T1), settings.coefficientx1, LT(CONSTANT_TERM), settings.constantterm);
  }
  else {
    Serial.println("Failed to calculate Coefficients.");
  }
  //Calculating R^2 Rsquare
  float agrav=0;  //average of gravity
  for (int i=0 ; i<n; i++) {
    agrav+=(1/float(n))*sampledgrav[i];
    Serial.println("Calculating agrav: "+String(agrav,5));
  }
  Serial.println("Average Gravity: "+String(agrav,8));
  float SSres=0;
  for (int i=0 ; i<n; i++) {
    SSres+=pow((sampledgrav[i]-(coeffs[0]*sampledtilt[i]*sampledtilt[i]*sampledtilt[i]+coeffs[1]*sampledtilt[i]*sampledtilt[i]+coeffs[2]*sampledtilt[i]+coeffs[3])),2);
  }
  Serial.println("SSres: "+String(SSres,8));
  float SStot=0;
  for (int i=0 ; i<n; i++) {
    SStot+=pow((sampledgrav[i]-agrav),2);
  }
  Serial.println("SStot: "+String(SStot,8));
  float Rsquare=1-SSres/SStot;
  offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<br>%s %f %s<br><br><a class='back' href='/'>%s</a>"), LT(K_DETERM), Rsquare, LT(K_DETERM_DESC), LT(BACK));

  //offset += getHtmlMenu( html+offset, bufferSize-offset);
  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); //?
  wm.server->send(200, "text/html", html);  
  free(html);
}

void handlePolynomialCalibrationInput()
{
  Serial.println("[HTTP] handle Polynomial Calibration Input");
  size_t offset = 0; // Текущая позиция в буфере
  u_int16_t bufferSize = 8192;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  } 

  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(CALIBRATION_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(html + offset, bufferSize - offset,
    PSTR("<BODY><h1>%s</h1>\
      <p>%s</p>\
      <p>%s</p>\
      <p>%s</p>\
      <p>%s</p>\
      <p>%s</p>\
      <p>\
      <form action='/polynomialcalibrationresults?' method ='POST'>\
      <table>\
      <tr>\
        <td><b>%s:</b></td>\
        <td><b>%s:</b></td>\
      </tr>\
      "),
    LT(CALIBRATION_SD_H1), LT(CALIBRATION_SD_DESC), LT(CALIBRATION_SD_STEP0), LT(CALIBRATION_SD_W1), LT(CALIBRATION_SD_W2), LT(CALIBRATION_SD_W3), 
    LT(TILT), LT(GRAVITY));

  int n=atoi(wm.server->arg(0).c_str());
  Serial.println("Sample Size in handlePolynomialCalibrationInput() : "+String(n));
  for (int i=0; i<n; i++) {
    offset += snprintf_P(html + offset, bufferSize - offset,
      PSTR("<tr>\
        <td><input type='number' name='tilt-%i' size='5' min='12.60' max='80.00' step='0.01'></td>\
        <td><input type='number' name='gravity-%i' size='5' min='1' max='1.12' step='0.001'></td></tr>"), i, i);
    if (i==0) {
      offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<tr><td colspan='2' style='padding: 0 8px 16px 8px;'>%s</td></tr>"), LT(CALIBRATION_SD_500));
    }
    else if (i==1) {
      offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<tr><td colspan='2' style='padding: 0 8px 16px 8px;'>%s</td></tr>"), LT(CALIBRATION_SD_600));
    }
    else if (i==2) {
      offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<tr><td colspan='2' style='padding: 0 8px 16px 8px;'>%s</td></tr>"), LT(CALIBRATION_SD_700));
    }
    else if (i==3) {
      offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<tr><td colspan='2' style='padding: 0 8px 16px 8px;'>%s</td></tr>"), LT(CALIBRATION_SD_800));
    } 
    else if (i==4) {
      offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<tr><td colspan='2' style='padding: 0 8px 16px 8px;'>%s</td></tr>"), LT(CALIBRATION_SD_900));
    } 
    else if (i==5) {
      offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<tr><td colspan='2' style='padding: 0 8px 16px 8px;'>%s</td></tr>"), LT(CALIBRATION_SD_1100));
    } 
    else if (i==6)  {
      offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<tr><td colspan='2' style='padding: 0 8px 16px 8px;'>%s</td></tr>"), LT(CALIBRATION_SD_1300));
    }
    /*else
      offset += snprintf_P(html + offset, bufferSize - offset, PSTR("<td></td></tr>"));*/
  }
  offset += snprintf_P(html + offset, bufferSize - offset, 
    PSTR("</table><br><input type='submit' value='%s' class='save'></form></p>\
      <br><a class='back' href='/'>%s</a>"), LT(SUBMIT), LT(BACK));

  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); //~6.5k

  wm.server->send(200, "text/html", html);
  free(html);
}

void handlePolynomialCalibrationStart()
{
  Serial.println("[HTTP] handle Polynomial Calibration Start");

  size_t offset = 0; // Текущая позиция в буфере
  u_int16_t bufferSize = 4096;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  } 

  offset += snprintf_P(html + offset, bufferSize - offset, headTemplate, LT(CALIBRATION_TITLE)); 
  offset += snprintf_P(html + offset, bufferSize - offset, styleTemplate);
   offset += snprintf_P(html + offset, bufferSize - offset,
    PSTR("<BODY><h1>%s</h1>\
    <p>%s</p>\
    <p>%s</p>\
    <p>%s</p>\
    <p>%s</p>\
    <p>\
      <form action='/polynomialcalibrationinput?' method ='POST'> %s (6-30): \
        <input type='number' name='sample_size' size='5' min='6' max='30' step='1' style='margin-top:10px;'>\
        <input class='save' type='submit' value='%s' style='margin-top:10px;'>\
      </form>\
    </p>\
    <a class='back' href='/'>%s</a><br>"),
     LT(CALIBRATION_H1), LT(CALIBRATION_P1), LT(CALIBRATION_P2), LT(CALIBRATION_P3), LT(CALIBRATION_P4), LT(CALIBRATION_SAMPLE_SIZE), LT(SUBMIT), LT(BACK) );
  //offset += getHtmlMenu( html+offset, bufferSize-offset);
  offset += snprintf_P( html+offset, bufferSize-offset, PSTR("</body></html>") );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); //?

  wm.server->send(200, "text/html", html);
  free(html);
}


void getReadingsHtml(char* buffer, size_t bufferSize, float batvolt, float batp, float tilt, float roll, float grav, float temp, float gyro_temp, float abv, float ema)
{
  size_t offset = 0; // Текущая позиция в буфере

  //http-equiv='refresh' content='1'
  //
  offset += snprintf_P(buffer + offset, bufferSize - offset, 
    PSTR("<HTML><head><meta charset='UTF-8'>\
      <link rel=\"stylesheet\" href=\"/uPlot.min.css\">\
      <script src=\"/uPlot.iife.min.js\"></script>\
      <style>\
        #graph { width:100%%; height:300px; }\
        button { font-size: 16px; padding: 10px; margin: 10px; }\
      </style>\
      <title>%s</title></head>"), LT(READINGS_TITLE));
  offset += snprintf_P(buffer + offset, bufferSize - offset, styleTemplate);
  offset += snprintf_P(buffer + offset, bufferSize - offset,
    PSTR("<BODY><h1>%s</h1>\
    <p>%s</p>\
    <p>%s: %.2f</p>\
    <p>%s: %.0f</p>\
    <p>%s: <b><span id='tilt-val'>%.2f</span>&deg;</b> (EMA <span id='tilt-ema'>%.2f</span>)</p><p style='font-size:12px;'>%s</p>\
    <p>%s: %.2f &deg;</p><p style='font-size:12px;'>%s</p>\
    <p>%s: %.5f SG</p>\
    <p>%s: %.2f &deg;C</p>\
    <p>%s: %.2f &deg;C</p>\
    <p>%s: %.2f %%</p>\
    <h2>Real-Time Graph</h2>\
    <button id=\"toggle-plot\">Включить</button>\
    <div id=\"graph\"></div>\
    <div id=\"h_graph\"></div>\
    <script>\
      let ltime=0;\
      let data = [ [], [], [] ]; /*[time, values, ema, ]*/\
      let h_data = [ [], [] ];/*[values, freq]*/\
      let opts = {\
          title: \"Real-Time Data\",\
          width: 800,\
          height: 300,\
          scales: {\
                x: { time: false },\
                y: {font: \"10px Arial\"},\
          },\
          series: [\
                {},\
                { label: \"tilt\", stroke: \"red\", width: 1 },\
                { label: \"ema\", stroke: \"blue\", width: 1 },\
          ],\
      };\
      let h_opts = {\
          title: \"Histogram\",\
          width: 800,\
          height: 300,\
          scales: {\
                x: { time: false },\
                y: {font: \"10px Arial\"},\
          },\
          series: [\
                {},\
                { label: \"freq\", stroke: \"blue\", width: 1 },\
          ],\
      };\
      \
      let uplot = new uPlot(opts, data, document.getElementById(\"graph\"));\
      let h_uplot = new uPlot(h_opts, h_data, document.getElementById(\"h_graph\"));\
      let isPlotting = false;\
      let tiltValueElement = document.getElementById(\"tilt-val\");\
      let tiltEMAElement = document.getElementById(\"tilt-ema\");\
      \
      function updateTiltValue() {\
        if( isPlotting ){\
          tiltValueElement.textContent = data[1].at(-1);\
          tiltEMAElement.textContent =  data[2].at(-1);\
        }\
        else{\
          fetch(\"/data\")\
            .then(response => response.json())\
            .then(json => {\
                tiltValueElement.textContent = json.v.toFixed(2);\
                tiltEMAElement.textContent = json.f.toFixed(2);\
          });\
        }\
      }\
      \
      /* Функция для подсчета частот */\
      function countFrequencies(data) {\
        let frequencyMap = new Map();\
        \
        for (let value of data) {\
          if (frequencyMap.has(value)) {\
            frequencyMap.set(value, frequencyMap.get(value) + 1);\
          } else {\
            frequencyMap.set(value, 1);\
          }\
        }\
      \
      /* Преобразуем Map в два массива: значения и частоты */\
      let values = Array.from(frequencyMap.keys());\
      let frequencies = Array.from(frequencyMap.values());\
      \
      /* Сортируем значения и частоты */\
      let sortedIndices = values\
        .map((value, index) => ({ value, index })) /* Сохраняем индексы */\
        .sort((a, b) => a.value - b.value) /* Сортируем по значениям */\
        .map(obj => obj.index); /* Получаем отсортированные индексы */\
      \
      /* Применяем сортировку к values и frequencies*/\
      values = sortedIndices.map(i => values[i]);\
      frequencies = sortedIndices.map(i => frequencies[i]);\
      \
      return [ values, frequencies ];\
      }\
      \
      function updatePlotData() {\
        if (!isPlotting) return;\
        fetch(\"/data\")\
          .then(response => response.json())\
            .then(json => {\
              if (json.t > ltime){\
                ltime = json.t;\
                data[0].push(json.t); /*Время*/\
                data[1].push(json.v); /*tilt*/\
                data[2].push(json.f); /*tilt filtered*/\
        \
                [ h_data[0], h_data[1] ] = countFrequencies(data[1]);\
        \
                /* Ограничиваем количество точек на графике (например, 100) */\
                if (data[0].length > 100) {\
                  data[0].shift();\
                  data[1].shift();\
                  data[2].shift();\
                }\
        \
                uplot.setData(data); /* Обновляем график*/\
                h_uplot.setData(h_data);\
              }\
              /*setTimeout(fetchData, 100);*/ /* Запрос каждые 100 мс */\
            });\
      }\
      setInterval(updateTiltValue, 2000);\
      setInterval(updatePlotData, 200);\
      \
      /* Обработчик для кнопки включения/остановки прорисовки */\
      document.getElementById(\"toggle-plot\").addEventListener(\"click\", () => {\
          isPlotting = !isPlotting;\
          document.getElementById(\"toggle-plot\").textContent = isPlotting\
              ? \"Остановить\"\
              : \"Включить\";\
      });\
      /*window.onload = fetchData;*/ /* Запуск при загрузке страницы */\
    </script>"),
   LT(READINGS_H1), LT(READINGS_UPDATE), LT(BAT_VOLTAGE), batvolt, LT(BAT_PERCENT), batp, LT(TILT), tilt, ema, LT(TILT_INFO), LT(ROLL), roll, LT(ROLL_INFO), 
   LT(GRAVITY), grav, LT(TEMPERATURE_MPU), gyro_temp, LT(TEMPERATURE_DS), temp, LT(ABV), abv);
  offset += snprintf_P( buffer+offset, bufferSize-offset, PSTR("<br><br><a class='back' href='/'>%s</a></body></html>"), LT(BACK) );
  Serial.print("[HTTP] Lenght:");
  Serial.println(offset); 
}

void handleReadings() 
{
  Serial.println("[HTTP] handle Readings");

  powerUpSensors();
  initMPU(5);
  float batvolt=calcBatThresholdAnalyze(0.33, 10, 100);
  float roll= calcRoll(40);
  tilt= calcTilt(200); // чем ниже тем лучше. идет накопление ВЧ фильтра
  float grav=calcGrav(tilt);
  float abv=calcABV(grav, settings.originalgravity);
  float t = calcTemp();
  float t2 = calcGyroTemp();
  //powerDownSensors();

  float alpha = 0.1; // Коэффициент сглаживания (чем меньше, тем сильнее сглаживание)
  //EMA Экспоненциальное скользящее среднее
  if(tilt_ema==0) tilt_ema=tilt;
  tilt_ema = alpha * tilt + (1 - alpha) * tilt_ema;

  u_int16_t bufferSize = 8192;
  char* html = (char*)malloc(bufferSize); // Выделяем память в куче
  if (html == NULL) {
      Serial.println("Memory allocation failed!");
      return;
  } 

  getReadingsHtml(html, bufferSize, batvolt, calcBatCap(batvolt), tilt, roll, grav, t, t2, abv, tilt_ema );
  
  wm.server->send(200, "text/html", html);
  free(html);
}

//callback notifying us of the need to save config
/*void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}*/


void checkLanguage(void)
{
    if( settings.language >= NUM_LANGUAGES ) settings.language=0;
}


void runConfigurationPortal () 
{
  #ifdef LED_BUILTIN
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

  checkLanguage();

  //https://github.com/espressif/arduino-esp32/blob/release/v2.x/docs/source/api/timer.rst
  // The ESP32 default clock is at 80MhZ. The value "80" will divide the clock by 80, giving us 1,000,000 ticks per second.
  // ESP32C3 has 2 timers
  /*timer = timerBegin(0, 80, true); // Таймер 0, делитель 80, счет вверх
  timerAttachInterrupt(timer, &onTimer, true); // Привязываем прерывание
  timerAlarmWrite(timer, 100000, true); // Интервал 1/10 секунда, автоповтор
  timerStop(timer);
  timerAlarmEnable(timer);*/
            
    //String menue="<p>"+htmlMenueText+"</p>";
    //WiFiManagerParameter custom_menue(menue.c_str());
    
    //wm.setDebugOutput(true, WM_DEBUG_MAX );
  
    //wm.resetSettings();
    //Setting Callbacks
    //wm.setSaveConfigCallback(saveConfigCallback);


    wm.setConfigPortalBlocking(false);
    wm.setWebServerCallback(bindServerCallback);  
  
    WiFiManagerParameter back_button("<br><br><a style='border: 0;\
    border-radius: .3rem;\
    background-color: #1fa3ec;\
    color: #fff;\
    line-height: 2.4rem;\
    font-size: 1.2rem;\
    width: 100%;\
    display: block;\
    text-align: center;' class='back' href='/'><b>&#706;<b></a>");
    wm.addParameter(&back_button);   //wm.setCustomBodyFooter(&back_button); ждем нового релиза с этой функцией.
    //add all your parameters here
    //wm.addParameter(&custom_portalTimeOut);
    ////wm.addParameter(&custom_menue);
    //wm.setConfigPortalTimeout(atoi(portalTimeOut));
    //wm.setCustomHeadElement(htmlWiFiConfStyleText.c_str());
    ////wm.setCustomHeadElement(htmlStyleText.c_str());

    String APName="iTilt_";
    Serial.println("WiFi Mac adress is: "+WiFi.macAddress());
    APName+=String(settings.itiltnum);
    Serial.println("Default password: 12345678");
    /*if (!wm.startConfigPortal(APName.c_str(), "12345678"))
    {
      Serial.println("WiFi Manager Portal: User failed to connect to the portal or dit not save. Time Out");
      ESP.restart();
      #ifndef ARDUINO_ESP32C3_DEV
        digitalWrite(LED_BUILTIN,HIGH);
      #endif      
      delay(5000);
    }*/
    wm.startConfigPortal(APName.c_str(), "12345678");
    portalRunning = true;
    doWiFiManager();
    wm.stopConfigPortal();
    Serial.println("Portal stopped");
    delay(1000);
    ESP.restart();
    #ifndef ARDUINO_ESP32C3_DEV
      digitalWrite(LED_BUILTIN,HIGH);
    #endif      
 }

 void doWiFiManager()
 {
    while(true){
        if( portalRunning ) {
            wm.process(); // do processing
        } else {
            break;
        }

        //tilt
        if( dataSubscribesCount > 0 ){
            dataSubscribesCount--;
            if( dataSubscribesCount == 0 ) {
                //Serial.println("****"); // Для отладки
                finishMPUReadings();
                powerDownSensors();
            }
            //get tilt
            if(acc_status==2){
                readTilt();
            }
            //if( dataSubscribesCount % 10 == 0 ) Serial.println(dataSubscribesCount);
        }
    }
}

void handleExit(void)
{
    portalRunning = false;
}