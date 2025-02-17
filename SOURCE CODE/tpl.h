#ifndef LANGUAGES_H
#define LANGUAGES_H

#include <pgmspace.h> // Для PROGMEM

enum StringIndex {
    CONFIGURE_WIFI,
    SETTINGS,
    SENSOR_READINGS,
	OFFSET_CALIBRATION,
	POLYNOMIAL_CALIBRATION,
	INFO,
	EXIT,
	FIRMWARE_UPDATE,
	PIN_CONFIGURATION,
	MAIN_PAGE_TITLE,
	MAIN_PAGE_H1,
	SETTINGS_WIFI_TITLE,
	SETTINGS_UPDATED,
	SETTINGS_ERROR,
	RESET,
	BACK,
	PIN_TITLE,
	RESTART30,
	SETTINGS_TITLE,
	SETTINGS_H1,
	SERVER,
	USERNAME,
	PASSWORD,
	POLINOMIAL_PARAMETERS,
	K_T3,
	K_T2,
	K_T1,
	CONSTANT_TERM,
	OTHER_PARAMETERS,
	K_BATTERY,
	DATA_PUBLICATION,
	DATA_PUBLICATION_OFFLINE,
	OG,
	TILT_OFFSET,
	ITILT_ID,
	CONFIGURATION_PORTAL_TIMEOUT,
	READINGS_TITLE,
	READINGS_H1,
	READINGS_UPDATE,
	BAT_VOLTAGE,
	BAT_PERCENT,
	TILT,
	TILT_INFO,
	ROLL,
	ROLL_INFO,
	GRAVITY,
	TEMPERATURE_DS,
	TEMPERATURE_MPU,
	ABV
};




// Массив указателей на строки для каждого языка
const char* const english[] PROGMEM = 
	{"CONFIGURE WIFI",	"CONFIGURE DEVICE", "SENSOR READINGS","OFFSET CALIBRATION","POLYNOMIAL CALIBRATION","INFO","EXIT","FIRMWARE_UPDATE","PIN AND SENSOR CONFIGURATIONS",
	"iTilt Main Page","CONFIGURATION PORTAL for iTilt",
	"iTilt Config", "Configuration is updated:","But has errors", "Reset device","Back",
	"iTilt Custom Pin Configuration", "The iTilt will restart in less than 30 seconds. Updates will take effect after restart.",
	"iTilt Device Configuration", "Device settings","COG Server", "Cloud USERNAME", "Cloud PASSWORD", "Provide the Parameters of you Polynomial", "Coefficient of Tilt^3", "Coefficient of Tilt^2", "Coefficient of Tilt^1",
	"Constant Term (Regression Model Polynomial)", "Other Parameters for the iTilt", "Battery Conversion Factor", "Data Publication Interval (s)", "Data Publication Interval when offline (s)", "Original Gravity", 
	"Calibrated Tilt Offset", "iTilt ID Number", "WiFi Manager Configuration Portal Time Out",
	"iTilt Sensor Readings", "iTilt Sensor Readings", "Sensor Readings will update every 2 seconds.", "Battery Voltage", "Battery Remaining Capacity", 
	"Tilt", "This value should be about 89 degrees if the iTilt is on a horizontal surface. If not, do a Tilt Ofset calibration or resolder your MPU6050 Gyroscope",
	"Roll", "This value should be close to 90 when iTilt is free flowing in liquid.",
	"Gravity", "Temperature according to DS18B20 (Very accurate)", "Gyro Internal Temperature (1 degree resolution)", "Alcohol by Volume (ABV)" 	};

const char* const russian[] PROGMEM = 
	{"Настройка WiFi", "Настройки устройства", "Данные датчиков","Настройка гироскопа","Калибровка плотности","Информация","Выход","Обновление прошивки","Настройка пинов",
	"iTilt","Портал настройки iTilt",
	"iTilt Настройка WiFi", "Настройки записаны:", "Но есть ошибки", "Перезагрузите устройство", "Назад",
	"iTilt Настройка пинов", "Для применения настроек iTilt перезагрузится в течении 30 секунд.",
	"iTilt Настройки устройства", "Настройки устройства", "COG сервер", "Логин", "Пароль", "Параметры полинома плотности", "Коэффициент Tilt^3", "Коэффициент Tilt^2", "Коэффициент Tilt^1",
	"Свободный член", "Остальные параметры", "Коэффициент вольтметра", "Интервал отправки данных (с)", "Интервал сохранения данных в автономном режиме (с)", "Начальная плотность", 
	"Смещение гироскопа", "iTilt ID", "Таймаут портала настройки",
	"iTilt Чтение данных", "Чтение данных", "Обновление данных раз в 2 секунды.","Заряд аккумулятора (В)", "Заряд аккумулятора (%)", 
	"Tilt(Наклон)", "Если устройство лежит на горизонтальной поверхности, значение должно быть около 89°. Если нет, то требуется настройка смещения гироскопа.",
	"Roll(Крен)", "Если устройство плавает в жидкости, значение должно быть около 90°",
	"Удельный вес", "Точная температура по DS18B20", "Температура по MPU6050 (точность 1°C)", "ABV"};

// Массив языковых пакетов
const char* const* const languages[] PROGMEM = {english, russian};



const char* headTemplate = PSTR("<html><head><meta charset='UTF-8'><title>%s</title></head>");

const char* styleTemplate = 
	PSTR("<style>\
		body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088;  max-width:900px; align-content:center; margin: auto; font-size: 30px; }\
		p {font-size: 30px;}\
		h1 {text-align: center;}\
		.button {\
			background-color: blue;\
			border: none;\
			color: white;\
			padding: 30px 15px;\
			text-align: center;\
			text-decoration: none;\
			display: inline-block;\
			font-size: 30px;\
			margin: 4px 2px;\
			cursor: pointer;\
			width: 900px;\
			border-radius: 20px;\
		}\
	</style>");

#endif


