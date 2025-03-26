#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include "main.h"
#include "webportal.h"
#include "cog.h"
#include <unordered_map>


//Не забыть загрузить файлы из папки /data : pio run --target uploadfs

//Project Notes:https://github.com/JJSlabbert/iTilt-digital-hydrometer
//Board Seeed studio XIAO-ESP32-C3
//Using #ifdef ARDUINO_ESP32C3_DEV for specifically code
//How to turn on Serial monitor on ESP32-C3-Zero Arduino IDE > Tools > USB CDC On Boot > Enabled. Flash.


//TODO
// - Portal - нажать save не вводя пароля - что будет?
// - !ресет таймера в режиме сохранения данных. Сохранение данных до ресета (на сервере)
// - portaltimeout
// - 21700 и ЛоРа
// - сделать калибровку на лету. То есть добавляем в полином точку. Смотрим соседние. Удаляем их или нет. Вводим новые коэффициенты
// - запрос сервером коэффициентов
// ? mpu ultra low  power - https://stackoverflow.com/questions/54450757/how-use-the-mpu-6050-in-ultra-low-power-mode
// - [WebServer.cpp:638] _handleRequest(): request handler not found 
// - переход на ООП
// - переход на ESP-IDF
// + обновление параметров без перезагрузки
// + калибровка - фильтр
// + калибровка - сохранить коэффициенты
// + зависание при ресете. fixed?
// + оптимизация (типа atof в цикле)
// + все настройки, которые надо переводить atof много раз - в глобальные переменные
// + тексты в файлах для локализации на разные языки
// + сглаживание показаний калибровки
// + сглаживание показаний измерений


//c3 feautures
// it's better don't use gpio9. It's can make startup in the boot mode. See Strapping Pins

//Питание DS и MPU от одного GPIO:
//Ток на GPIO 40мА. Рекомендуется 20мА
//MPU6050 4mA + LED 0.1mA + I2C 0.7mA
//DS18D20 1.5mA
//==OK


#ifdef ESP8266
#error "This code is not compatible with ESP8266. Please use ESP32."
#endif


#include <ESP32Time.h>
//waiting for update https://github.com/espressif/arduino-esp32/issues/9912
#include <LittleFS.h>             //https://github.com/littlefs-project/littlefs/blob/master/README.md
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


//flag for saving data (Custom params for WiFiManager
//bool shouldSaveConfig = false;


//DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

//MPU6050
#include <MPU6050.h> //https://www.i2cdevlib.com/docs/html/class_m_p_u6050.html#a196404ef04b959083d4bf5e6f1cd8b98
#include <Wire.h>

ESP32Time rtc;
MPU6050 accelgyro(hardware.i2c_address);
int16_t ax, ay, az;
float tilt;
float tilt_ema;
float temperature;
uint8_t acc_status; // 0 - power off; 1 - not init;  2-ready
Settings settings;
Hardware hardware;

//#include "esp_sleep.h"  //https://github.com/espressif/arduino-esp32/issues/2712 To keep GPIO power pin LOW during Deep Sleep
//#include "driver/gpio.h"
//#include "esp_err.h"

/*You mentioned that the data just needs to survive deep sleep. If that's the case, your best option (if it's large enough) is to use the ESP32's RTC static RAM. 
This chunk of memory will survive restarts and deep sleep mode, but will lose its state if power is interrupted. It's real RAM so you won't wear it out by writing 
to it frequently, and it doesn't cost a lot of energy to write to. The catch is there's only 8KB of it.*/
RTC_DATA_ATTR static int connection_missing_count = 0;


void infiniteSleep()  //(Hybernation Mode) This is used when battery is charged or iTilt is stored
{ 
  Serial.println("Device will enter Infinite deep sleep.");
  #ifdef ESP32
  powerDownSensors();
  #ifdef LED_BUILTIN
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

  esp_sleep_enable_timer_wakeup(999999999999999999);  //How long can esp32 hibernate
  esp_deep_sleep_start();
  //delay(1000); //???
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
    #ifdef LED_BUILTIN
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
  Serial.print("Entering deep sleep for ");
  Serial.print(interval);
  Serial.println(" seconds");

  esp_sleep_enable_timer_wakeup(interval *  1000000);
  esp_deep_sleep_start();
  //delay(1000); //????

  return true;
}

float calcOffset()
{ 
  float reading;
  float areading=0;
  uint8_t n=100;
  //pinMode(hardware.i2c_sda_pin, OUTPUT);//This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  //pinMode(hardware.i2c_scl_pin, OUTPUT); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(hardware.i2c_sda_pin, LOW); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(hardware.i2c_scl_pin, LOW);  //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  delay(100);
  Wire.begin(hardware.i2c_sda_pin, hardware.i2c_scl_pin);
  Wire.beginTransmission(hardware.i2c_address);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  accelgyro.initialize();
  Serial.println("You are in calcOffset(). Calibrating: ");
  for (int i=0; i<100; i++) {
    accelgyro.getAcceleration(&ax, &az, &ay);
    if( ax+ay+az==0 ) {
      Serial.println("Reading=0");
      n--;
    }
    else {    
      reading=acos(az / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
      areading+=reading/n;
      Serial.println(reading);
    }
    if (i==99 && areading==0) {
      Serial.println("The MPU6050 could not provide numerical readings for 100 times. Check your soldering and test the MPU6050.");
    }
  }
  Serial.println("Avarage Reading: "+String(areading));
  float offset;
  offset=89.0-areading;
  if( hardware.mpu_orientation==0 ) {
    offset=91-areading;
  } //This must be tested?
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
  }           
  return capacity;    
}

float calcBatVolt(int sample_size)
{
  // лучше измерять до работы wifi
  float reading=0;
  float n=sample_size;

  for (int i=0;i<n;i++) {
    reading+=analogReadMilliVolts(hardware.batvolt_pin);
    //reading=analogReadMilliVolts(hardware.batvolt_pin);
    //Serial.println(reading);
  }
  reading = reading/(1000*n*settings.batconvfact);
  return reading;
}

float calcTemp() //DS19B20 Sensor Texas Instruments
{ 
    float reading;
    OneWire oneWire(hardware.onewire_pin);
    DallasTemperature sensors(&oneWire);
    sensors.begin();
    sensors.requestTemperatures();
    reading = sensors.getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.println(reading);
    return reading;
}

float calcGyroTemp()
{
  if( acc_status != 2) {
    Serial.println("MPU not init");
    return 0;
  }
  float temp=accelgyro.getTemperature()/340 +36.53;
  return temp;
}

float calcTilt(int samplesize)
{ 
  if( acc_status != 2) {
    Serial.println("MPU not init");
    return 0;
  }

  float reading;
  float areading=0;
  float n=samplesize;
  
  for (int i=0; i<n; i++) {
    for (int j=0; j<50; j++) {
      if( !accelgyro.getIntDataReadyStatus() ) delay(1); // Rate=10 => delay=8
      else break;
    }
    accelgyro.getAcceleration(&ax, &az, &ay);
    reading=acos(az / (sqrt(ax * ax + ay * ay + az * az))) ;
    areading+=reading;
  }
  areading=areading * 180.0 / M_PI / n;
  areading+=settings.tiltOffset;

  if( hardware.mpu_orientation==0 ) { //This is when Vcc on MPU is vacing down
    areading=90-(areading-90);
  }
  return areading;

}

float calcRoll(int samplesize)
{ 
  if( acc_status != 2) {
    Serial.println("MPU not init");
    return 0;
  }  

  float reading;
  float areading=0;
  float n=samplesize;

  for (int i=0; i<n; i++)  {
    for (int j=0; j<50; j++) {
      if( !accelgyro.getIntDataReadyStatus() ) delay(1);
      else break;
    }
    accelgyro.getAcceleration(&ax, &az, &ay);
    reading=acos(ax / (sqrt(ax * ax + ay * ay + az * az))) ;  //reading=acos(az / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
    areading+=reading;
    //Serial.println(ax);
  }
  areading = areading * 180.0 / M_PI / n;
  return areading;
}

std::pair<float, float> calcTiltAndRoll(int samplesize)
{
  if( acc_status != 2) {
    Serial.println("MPU not init");
    return {0,0};
  }

  float readingT;
  float readingR;
  float areadingT=0;
  float areadingR=0;
  float n=samplesize;
  
  for (int i=0; i<n; i++) {
    for (int j=0; j<50; j++) {
      if( !accelgyro.getIntDataReadyStatus() ) delay(1);
      else break;
    }

    accelgyro.getAcceleration(&ax, &az, &ay);
    readingT=acos(az / (sqrt(ax * ax + ay * ay + az * az)));
    readingR=acos(ax / (sqrt(ax * ax + ay * ay + az * az)));
    areadingT+=readingT;
    areadingR+=readingR;
  }
  areadingT=areadingT * 180.0 / M_PI / n;
  areadingT+=settings.tiltOffset;
  areadingR = areadingT * 180.0 / M_PI / n;

  if( hardware.mpu_orientation==0 ) { //This is when Vcc on MPU is vacing down
    areadingT=90-(areadingT-90);
  }
  return {areadingT, areadingR}; 
}



float calcGrav(float tilt)
{
  return settings.coefficientx3 * (tilt * tilt * tilt) + settings.coefficientx2 * tilt * tilt + settings.coefficientx1 * tilt + settings.constantterm;
}

float calcABV(float gravity, float og)
{
  float abv = 131.258 * (og - gravity);
  return abv;
}

// фильтр6 чтение1 +- 0.07
// фильтр5 чтение1 +- 0.11
// фильтр4 чтение1 +- 0.14 
// фильтр3 чтение1 +- 0.18 
void initMPU(uint8_t filter)
{ 
  digitalWrite(hardware.i2c_sda_pin, LOW); //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  digitalWrite(hardware.i2c_scl_pin, LOW);  //This need to be done, possable conflict between WiFimanager and Wire Library (i2c). It must be before Wire.begin()
  //delay(100); //??? 
  //Let's check this:
  while (digitalRead(hardware.i2c_sda_pin) == HIGH || digitalRead(hardware.i2c_scl_pin) == HIGH) {
    //Serial.println("*!*");
  }
  Wire.begin(hardware.i2c_sda_pin, hardware.i2c_scl_pin);
  //Wire.setClock(100000); it's by default
  Wire.beginTransmission(hardware.i2c_address);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  accelgyro.initialize();
  // initialize() has made setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  // initialize() has made setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  
/** digital low-pass filter configuration.
 * The DLPF_CFG parameter sets the digital low pass filter configuration. It
 * also determines the internal sampling rate used by the device as shown in
 * the table below.
 *
 * Note: The accelerometer output rate is 1kHz. This means that for a Sample
 * Rate greater than 1kHz, the same accelerometer sample may be output to the
 * FIFO, DMP, and sensor registers more than once.
 *
 * <pre>
 *          |   ACCELEROMETER    |           GYROSCOPE
 * DLPF_CFG | Bandwidth | Delay  | Bandwidth | Delay  | Sample Rate
 * ---------+-----------+--------+-----------+--------+-------------
 * 0        | 260Hz     | 0ms    | 256Hz     | 0.98ms | 8kHz
 * 1        | 184Hz     | 2.0ms  | 188Hz     | 1.9ms  | 1kHz
 * 2        | 94Hz      | 3.0ms  | 98Hz      | 2.8ms  | 1kHz
 * 3        | 44Hz      | 4.9ms  | 42Hz      | 4.8ms  | 1kHz
 * 4        | 21Hz      | 8.5ms  | 20Hz      | 8.3ms  | 1kHz
 * 5        | 10Hz      | 13.8ms | 10Hz      | 13.4ms | 1kHz
 * 6        | 5Hz       | 19.0ms | 5Hz       | 18.6ms | 1kHz
 * 7        |   -- Reserved --   |   -- Reserved --   | Reserved
 * </pre>
*/  
  accelgyro.setDLPFMode(filter);
  // нужна пауза, так как фильтр будет догонять от 0
  // паузы нужно перепроверить
  switch(filter){
    case 0: // MPU6050_DLPF_BW_256
      break;
    case 1: 
      delay(10); // MPU6050_DLPF_BW_188
      break;
    case 2: 
      delay(25); // MPU6050_DLPF_BW_98
      break;
    case 3: // MPU6050_DLPF_BW_42
      delay(50);
      break;
    case 4: // MPU6050_DLPF_BW_20
      delay(75);
      break;
    case 5: // MPU6050_DLPF_BW_10
      delay(100);
      break;
    case 6:  // MPU6050_DLPF_BW_5
      delay(250);
      break;
  }

  accelgyro.setRate(10); // Низкая частота дискретизации уменьшает уровень шума
  accelgyro.setIntDataReadyEnabled(true); // accelgyro.getIntDataReadyStatus() == 1 когда есть новые данные

  //Get Gyroscope readings untill it stoped reading nan (Not a Number)
  for (int i=0;i<200;i++)  {
    accelgyro.getAcceleration(&ax, &az, &ay);
    if( ax+ay+az==0 ) {
      Serial.println("You are in initMPU(). Readings=0");
    }
    else {
      Serial.println("MPU: ready");
      acc_status = 2;
      break;
    }
    if (i==199) {
      Serial.println("You are in initMPU(). The MPU6050 could not provide numerical readings for 200 times. Check your soldering and Test the MPU6050.");
    }
  }
}

void finishMPUReadings(void)
{
  accelgyro.setSleepEnabled(true);
  if( acc_status!=0 ) acc_status = 1;
}


void readTilt(void) // для частых вызовов. (калибровка) 
{
  if( accelgyro.getIntDataReadyStatus() ){
    accelgyro.getAcceleration(&ax, &az, &ay);
    tilt = acos(az / (sqrt(ax * ax + ay * ay + az * az))) * 180.0 / M_PI;
    tilt += settings.tiltOffset;

    float alpha = 0.002; // Коэффициент сглаживания (чем меньше, тем сильнее сглаживание)
    //EMA Экспоненциальное скользящее среднее
    tilt_ema = alpha * tilt + (1 - alpha) * tilt_ema;
  }

}

void disableGPIOHold() 
{
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
}

void powerUpSensors()
{
  //uint8_t pp = 5;
  //uint8_t pp2 = atoi(power_pin2);
  //Serial.println("Power Pin: "+String(pp) +" must go high");
  //Serial.println("Power Pin: "+String(pp2) +" must go high");    
  pinMode(hardware.power_pin,OUTPUT);
  digitalWrite(hardware.power_pin,HIGH);
  //pinMode(pp2,OUTPUT);
  //digitalWrite(pp2,HIGH);
  acc_status = 1;
}

void powerDownSensors()
{
  acc_status = 0;
  //uint8_t pp = 5;
  //uint8_t pp2 = atoi(power_pin2);
  pinMode(hardware.power_pin,OUTPUT);
  digitalWrite(hardware.power_pin,LOW);
  //pinMode(pp2,OUTPUT);
  //digitalWrite(pp2,LOW);
}

void listFiles() {
  Serial.println("Список файлов в LittleFS:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
      Serial.print("Файл: ");
      Serial.println(file.name());
      file = root.openNextFile();
  }
}

/**
 * Преобразует JSON-поле (число или строку) в целочисленную переменную.
 * Поддерживает: uint8_t, uint16_t, uint32_t, int, short и другие целочисленные типы.
 * 
 * @param target     Ссылка на переменную, в которую записываем значение.
 * @param jsonValue  JSON-поле (JsonVariant).
 * @return           true, если преобразование успешно, false при ошибке.
 */
template <typename T>
bool jsonToSettingsInt(T &target, const JsonVariant &jsonValue) 
{
    // Если JSON-поле - целое число
    if (jsonValue.is<int>()) {
        int val = jsonValue.as<int>();
        target = static_cast<T>(val);
        return true;
    }
    
    // Если JSON-поле - строка (например, "123")
    if (jsonValue.is<const char*>()) {
        target = atoi(jsonValue.as<const char*>());
        return true;
    }
    
    // Если тип не подходит (например, bool, float, массив)
    return false;
}

bool jsonToSettingsBool(bool &target, const JsonVariant &jsonValue)
{
  if (jsonValue.is<bool>()) {
    target = jsonValue.as<bool>();
    return true;
  } 
  else if (jsonValue.is<const char*>()) { // "new_calibration":"true" / "new_calibration":"1"
    String val = jsonValue.as<const char*>();
    val.toLowerCase();
    target = (val == "true" || val == "1");
    return true;
  }
  return false;
}

template <typename T>
bool jsonToSettingsFloating(T &target, const JsonVariant &jsonValue)
{
  if (jsonValue.is<float>()) {
    target = jsonValue.as<float>();
    return true;
  } 
  else if (jsonValue.is<double>()) {
    target = jsonValue.as<double>();
    return true;
  } 
  else if (jsonValue.is<const char*>()) { 
    //target = atof(jsonValue.as<const char*>());
    target = jsonValue.as<T>();
    return true;
  }
  return false;
}




void readConfiguration() 
{
  //read configuration from FS json
  Serial.print("mounting FS: ");
  if (LittleFS.begin()) {
    Serial.println("done");
    //listFiles();
    if (LittleFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config.json");
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

          if (!json["portalTimeOut"].isNull()) jsonToSettingsInt(settings.portalTimeOut, json["portalTimeOut"]);
          //if (!json["portalTimeOut"].isNull() && strlen(json["portalTimeOut"].as<const char*>()) > 0) settings.portalTimeOut = json["portalTimeOut"].as<uint16_t>();
          strcpy(settings.cloud_host, json["cloud_host"]);
          strcpy(settings.cloud_username, json["cloud_username"]);
          strcpy(settings.cloud_password, json["cloud_password"]);
          if (!json["coefficientx3"].isNull()) jsonToSettingsFloating(settings.coefficientx3, json["coefficientx3"]);
          if (!json["coefficientx2"].isNull()) jsonToSettingsFloating(settings.coefficientx2, json["coefficientx2"]);
          if (!json["coefficientx1"].isNull()) jsonToSettingsFloating(settings.coefficientx1, json["coefficientx1"]);
          if (!json["constantterm"].isNull()) jsonToSettingsFloating(settings.constantterm, json["constantterm"]);
          if (!json["batconvfact"].isNull()) jsonToSettingsFloating(settings.batconvfact, json["batconvfact"]);
          //if (!json["coefficientx3"].isNull() && strlen(json["coefficientx3"].as<const char*>()) > 0) settings.coefficientx3 = json["coefficientx3"].as<float>();
          //if (!json["coefficientx2"].isNull() && strlen(json["coefficientx2"].as<const char*>()) > 0) settings.coefficientx2 = json["coefficientx2"].as<float>();
          //if (!json["coefficientx1"].isNull() && strlen(json["coefficientx1"].as<const char*>()) > 0) settings.coefficientx1 = json["coefficientx1"].as<float>();
          //if (!json["constantterm"].isNull() && strlen(json["constantterm"].as<const char*>()) > 0) settings.constantterm = json["constantterm"].as<float>();
          //if (!json["batconvfact"].isNull() && strlen(json["batconvfact"].as<const char*>()) > 0) settings.batconvfact = json["batconvfact"].as<float>();
          if (!json["pubint"].isNull()) jsonToSettingsInt(settings.pubint, json["pubint"]);
          if (!json["offlinepubint"].isNull()) jsonToSettingsInt(settings.offlinepubint, json["offlinepubint"]);
          if (!json["originalgravity"].isNull()) jsonToSettingsFloating(settings.originalgravity, json["originalgravity"]);
          if (!json["tiltOffset"].isNull()) jsonToSettingsFloating(settings.tiltOffset, json["tiltOffset"]);
          if (!json["itiltnum"].isNull()) jsonToSettingsInt(settings.itiltnum, json["itiltnum"]);
          if (!json["language"].isNull()) jsonToSettingsInt(settings.language, json["language"]);

          /*if (!json["pubint"].isNull() && strlen(json["pubint"].as<const char*>()) > 0) settings.pubint = json["pubint"].as<uint32_t>();
          if (!json["offlinepubint"].isNull() && strlen(json["offlinepubint"].as<const char*>()) > 0) settings.offlinepubint = json["offlinepubint"].as<uint32_t>();
          if (!json["originalgravity"].isNull() && strlen(json["originalgravity"].as<const char*>()) > 0) settings.originalgravity = json["originalgravity"].as<float>();
          if (!json["tiltOffset"].isNull() && strlen(json["tiltOffset"].as<const char*>()) > 0) settings.tiltOffset = json["tiltOffset"].as<float>();
          if (!json["itiltnum"].isNull() && strlen(json["itiltnum"].as<const char*>()) > 0) settings.itiltnum = json["itiltnum"].as<uint16_t>();
          if (!json["language"].isNull() && strlen(json["language"].as<const char*>()) > 0) settings.language = json["language"].as<uint8_t>();*/
          /*if (json.containsKey("language")) {
            if (json["language"].is<uint8_t>()) {
                settings.language = json["language"].as<uint8_t>();
            } 
            else if (json["language"].is<const char*>()) {
                settings.language = atoi(json["language"].as<const char*>()); // "1" → 1
            }
          }*/
 
          if (!json["new_calibration"].isNull()) jsonToSettingsBool(settings.new_calibration, json["new_calibration"]);
          
          if( settings.pubint==0 ) settings.pubint=10;
          if( settings.offlinepubint < settings.pubint ) settings.offlinepubint=settings.pubint;

          Serial.println("We've set:");
          //settings.new_calibration=true;
          Serial.println( settings.new_calibration );

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
    else{
      Serial.println("no config file config.json");
    }

    if (LittleFS.exists("/pinconfig.json")) {
      //file exists, reading and loading
      Serial.println("reading pinconfig.json");
      File pinConfigFile = LittleFS.open("/pinconfig.json", "r");
      if (pinConfigFile) {
        Serial.println("opened Pin config file");
        size_t size = pinConfigFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        pinConfigFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(1024*2); // ?? много же
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {

          Serial.println("\nparsed json");

          hardware.power_pin = json["power_pin"].as<uint8_t>();
          //strcpy(power_pin2, json["power_pin2"]);
          hardware.batvolt_pin = json["batvolt_pin"].as<uint8_t>();
          hardware.onewire_pin = json["onewire_pin"].as<uint8_t>();
          hardware.i2c_sda_pin = json["i2c_sda_pin"].as<uint8_t>();
          hardware.i2c_scl_pin = json["i2c_scl_pin"].as<uint8_t>();
          hardware.mpu_orientation = json["mpu_orientation"].as<uint8_t>();
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
    Serial.println("failed");  //This hapen with new ESP32 KOALA WROVER-Format Spifs?
    LittleFS.format();  //May be needed to format ESP32 with MicroPython or iSpindel Firmware
  }  
}

void saveConfiguration(void)
{
  // Открываем файл для записи (перезаписываем, если существует)
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
      Serial.println("failed to open config file for writing");
      return;
  }

  // Создаём JSON-документ
  DynamicJsonDocument doc(1024); // Размер можно увеличить, если структура большая

  // Заполняем JSON значениями из структуры
  doc["itiltnum"] = static_cast<uint16_t>(settings.itiltnum);
  doc["coefficientx3"] = static_cast<double>(settings.coefficientx3);
  doc["coefficientx2"] = static_cast<double>(settings.coefficientx2);
  doc["coefficientx1"] = static_cast<double>(settings.coefficientx1);
  doc["constantterm"] = static_cast<double>(settings.constantterm);
  doc["tiltOffset"] = static_cast<float>(settings.tiltOffset);
  doc["batconvfact"] = static_cast<float>(settings.batconvfact);
  doc["originalgravity"] = static_cast<float>(settings.originalgravity);
  doc["pubint"] = static_cast<uint32_t>(settings.pubint);
  doc["offlinepubint"] = static_cast<uint32_t>(settings.offlinepubint);
  doc["portalTimeOut"] = static_cast<uint16_t>(settings.portalTimeOut);
  doc["cloud_host"] = settings.cloud_host;
  doc["cloud_username"] = settings.cloud_username;
  doc["cloud_password"] = settings.cloud_password;
  doc["language"] = static_cast<uint8_t>(settings.language);
  doc["new_calibration"] = static_cast<bool>(settings.new_calibration);

  // Сериализуем JSON в файл
  if (serializeJson(doc, configFile) == 0) {
      Serial.println("Ошибка записи в файл!");
  } else {
      Serial.println("Настройки сохранены в /config.json");
  }

  configFile.close();
}  

void saveCalibrationUpdated(void)
{
  settings.new_calibration = false;
  saveConfiguration();
}


uint32_t getFreeSpace() // Функция для проверки свободного места
{
  uint32_t totalBytes = LittleFS.totalBytes(); // Общее количество байт на файловой системе
  uint32_t usedBytes = LittleFS.usedBytes();   // Занятое количество байт
  return totalBytes - usedBytes;             // Свободное место
}

// пороговый метод с накоплением данных
// Функция для анализа данных в реальном времени
// выборка 500 - 220раз одно значение. Остальные не более 20 раз
// лучше измерять до работы wifi
float calcBatThresholdAnalyze(float threshold, int minReadings, int maxReadings) {
  std::unordered_map<uint32_t, int> frequencyMap; // Хранение частот значений
  int totalReadings = 0; // Общее количество измерений

  while (true) {
      // Чтение данных
      uint32_t reading = analogReadMilliVolts(hardware.batvolt_pin);
      // Обновляем частоту для текущего значения
      frequencyMap[reading]++;

      // Увеличиваем общее количество измерений
      totalReadings++;

      // Проверяем, превысило ли какое-то значение порог
      if (totalReadings >= minReadings && frequencyMap[reading] >= threshold * totalReadings) {
          return reading/(1000.0f*settings.batconvfact); // Возвращаем значение, которое превысило порог
      }

      // Если достигнуто максимальное количество измерений, останавливаемся
      if (totalReadings >= maxReadings) {
          Serial.println("calcBatThresholdAnalyze: no value reached the threshold");
          return -1; // Ни одно значение не превысило порог
      }
  }
}


void setup() 
{ 
  float signalstrength;
  float roll;
  float grav;
  float abv;
  //float temp;
  float tempgyro; 
  float batvolt;
  float batcap;

  uint32_t interval; //interval var
  bool has_queue;

  DEBUG_DELAY(2000); // debug for Serial

  tilt_ema=0;
  acc_status=0;

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("The iTilt is starting up....");

  //accelgyro.setSleepEnabled(true); // еще шина не инициализирована - убрать

  //delay(100);  //This is for accurate battery voltage reading  (???)
  #ifdef LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN,HIGH);
  #endif  

  //clean FS, for testing
  //LittleFS.format();
  //wm.resetSettings();
  readConfiguration();
  //dpubint = atoi(pubint);  
  //dofflineint = settings.offlinepubint;

  disableGPIOHold();

  pinMode(hardware.i2c_sda_pin, OUTPUT); // раньше в ArduinoIDE вызывался каждый раз перед wire.begin. А теперь это приводит к ошибке i2cRead returned Error 263
  pinMode(hardware.i2c_scl_pin, OUTPUT);

  while(true){ //no sleep if need readings faster than 60s
    powerUpSensors();
    initMPU(5);
    roll = calcRoll(10);
    //auto [tilt, roll] = calcTiltAndRoll(50); // лучше отдельно из-зи накопления ВЧ фильтра.
    if (roll<20 or roll>160) {
      Serial.println( "Roll:" + String(roll) + ". Going to sleep." );
      infiniteSleep();
    }
    tilt = calcTilt(100);
    finishMPUReadings();
    temperature = calcTemp();
    //tempgyro=calcGyroTemp();  
    powerDownSensors();

    batvolt=calcBatThresholdAnalyze(0.33, 10, 100); // 2mks !
    //batvolt=calcBatVolt(500); 
    // зачем так много? 500= 40ms
    // выборка 500 - 220раз одно значение. Остальные не более 20 раз
    //Serial.print("Has data:");
    //Serial.println(millis());
    if (batvolt<3)  {
      if (batvolt<1.0) {
        Serial.println("voltage sensor error");
        continue;
      }
      if (batvolt<2.5) {
        Serial.println("Error! Battery voltage is less than 2.5 volt. Recharge the battery or fix the battery conversion factor. Shut down.");
        delay(1000); // for serial output
        infiniteSleep();
      }
      Serial.println("Warning. Battery voltage is less than 3 volt. Recharge the battery or fix the battery conversion factor. Continue.");
    }

    Serial.println("You are in setup(), Tilt is: "+String(tilt)+ ", If tilt <12 degrees, between 85 and 95 degress or nan, The Wifi Manager configuration portal will run");
    if ((tilt<95 && tilt>85) || tilt<12 || tilt==0) { //Condition to run The Wifi Manager portal.  
      runConfigurationPortal();
      
      // Чудесный выход из портала без перезагрузки. Как?
      powerUpSensors();
      initMPU(5);
      tilt=calcTilt(100);
      temperature=calcTemp();
      //tempgyro=calcGyroTemp();
      batvolt=calcBatThresholdAnalyze(0.33, 10, 100);
      //batcap=calcBatCap(batvolt); 
      finishMPUReadings();
      powerDownSensors();
      //dpubint = atoi(pubint);  
      //dofflineint = settings.offlinepubint;
    }
    else  //This is when tilt>12, nan or between 85 and 95 degrees and setup should not run
    {
      Serial.println("Should not run portal");
      #ifdef LED_BUILTIN
        digitalWrite(LED_BUILTIN,HIGH);
      #endif      
    }
    if (tilt>85)
    {
      Serial.println("Tilt is larger than 85 degrees. It seems like it is not in your brew yet. ESP will restart");
      ESP.restart();
    }

    ////////////////
    //Штатный режим
    ////////////////
    // Изначально режим работы: Пробуждение. Получения X значений. Подглючение wi-fi. Отправка среднего. Сон
    // Мне не нравится, что, кроме шума, устройство подвержено гармоническим колебаниям. И мы выхватываем рандомную малую часть этого колебания.
    // С этим можно бороться сглаживанием на сервере. Чтобы получить сглаженный график, который охотно идет вниз и с большим трудом вверх.
    //
    // Можно увеличить длину сеанса бодрствования: Пробуждение. Сбор данных Х секунд. Подглючение wi-fi. Отправка среднего. Сон
    //
    // Можно: [ Пробуждение. Сбор Х данных. Сон. ] N раз. Подглючение wi-fi. Отправка среднего. Сон.
    //
    // Вопрос, что экономичнее по энергии?


    signalstrength=getRSSI();
    //abv=calcABV(grav, settings.originalgravity); // на сервере посчитаем
    //grav=calcGrav(tilt);

    //if( dofflineint < settings.pubint ) dofflineint = settings.pubint;

    uint32_t now = time(nullptr); // faster
    bool sending_failed = false;
    bool data_saved = false;

    if( connection_missing_count!=0 ) has_queue=true; 
    else has_queue=false;
    if ( connectToWiFi() ){
      //ditiltnum = atoi(itiltnum);
      if( has_queue == false ) {
        if( !pubReadingToCOG(settings.itiltnum, settings.cloud_username, settings.cloud_password, batvolt, tilt, temperature, signalstrength, now ) ) {
          sending_failed=true;
        }
      }
      else {
        storeData(  batvolt,  tilt, temperature,  signalstrength, now );
        data_saved = true;
        if( !pubFileToCOG(settings.itiltnum, settings.cloud_username, settings.cloud_password) )  {
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
      if( connection_missing_count >= settings.offlinepubint/settings.pubint ){
        //offline mode
        interval = settings.offlinepubint;
        connection_missing_count = settings.offlinepubint; // excepts overflow
      }
      else{
        interval = settings.pubint;
        connection_missing_count = 0;
      }
      if(!data_saved) storeData(  batvolt,  tilt, temperature,  signalstrength, now );
    }
    else{
      interval = settings.pubint;
    }
    //Serial.println("sleep:"+String(interval));
    
    /*Serial.print("Sent:"); //debug
    Serial.println(millis()); //debug
    delay(1000); //debug*/

    startDeepSleep(interval);
  }
}

void loop()
{
  //void loop() should never execute
}
