#ifndef TPL_H
#define TPL_H

#include <pgmspace.h> // Для PROGMEM
#include <stddef.h>

static const size_t NUM_LANGUAGES = 2;

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
	ABV,
	CALIBRATION_TITLE,
	CALIBRATION_H1,
	CALIBRATION_P1,
	CALIBRATION_P2,
	CALIBRATION_P3,
	CALIBRATION_P4,
	CALIBRATION_SAMPLE_SIZE,
	SUBMIT,
	PIN_UPDATED,
	PIN_H1,
	PIN_P,
	PIN_CURRENT,
	PIN_PERIPH,
	PIN_VCC,
	PIN_GYRO_ORIENTATION,
	PIN_ONEWIRE,
	PIN_SDA,
	PIN_SCL,
	OFFSET_TITLE,
	OFFSET_H1,
	OFFSET_CALCULATED,
	OFFSET_COPY,
	POLYNOM_RESULTS_TITLE,
	POLYNOM_RESULTS_H1,
	TILT_VAL,
	GRAVITY_VAL,
	POLYNOM_VAL_COPIED,
	POLYNOM_GO_AND_SAVE,
	K_DETERM,
	K_DETERM_DESC,
	UPDATE_RESTART,
	CALIBRATION_SD_H1,
	CALIBRATION_SD_DESC,
	CALIBRATION_SD_STEP0,
	CALIBRATION_SD_W1,
	CALIBRATION_SD_W2,
	CALIBRATION_SD_W3,
	CALIBRATION_SD_GUIDE,
	CALIBRATION_SD_500,
	CALIBRATION_SD_600,
	CALIBRATION_SD_700,
	CALIBRATION_SD_800,
	CALIBRATION_SD_900,
	CALIBRATION_SD_1100,
	CALIBRATION_SD_1300,
	LANGUAGE,
	REALTIMEGRAPH,
	REALTIMEDATA,
	HISTOGRAM,
	STARTSTOP
};


// Массив указателей на строки для каждого языка
const char* const english[] PROGMEM = 
	{"CONFIGURE WIFI",	"CONFIGURE DEVICE", "SENSOR READINGS","OFFSET CALIBRATION","POLYNOMIAL CALIBRATION","INFO","EXIT","FIRMWARE UPDATE","PIN AND SENSOR CONFIGURATIONS",
	"iTilt Main Page","CONFIGURATION PORTAL for iTilt",
	"iTilt Config", "Configuration is updated:","But has errors", "Reset device","Back",
	"iTilt Custom Pin Configuration", "The iTilt will restart in less than 30 seconds. Updates will take effect after restart.",
	"iTilt Device Configuration", "Device settings","COG Server", "Cloud USERNAME", "Cloud PASSWORD", "Provide the Parameters of you Polynomial", "Coefficient of Tilt^3", "Coefficient of Tilt^2", "Coefficient of Tilt^1",
	"Constant Term (Regression Model Polynomial)", "Other Parameters for the iTilt", "Battery Conversion Factor", "Data Publication Interval (s)", "Data Publication Interval when offline (s)", "Original Gravity", 
	"Calibrated Tilt Offset", "iTilt ID Number", "WiFi Manager Configuration Portal Time Out",
	"iTilt Sensor Readings", "iTilt Sensor Readings", "Tilt value updates every 2 seconds.", "Battery Voltage", "Battery Remaining Capacity", 
	"Tilt", "This value should be about 89 degrees if the iTilt is on a horizontal surface. If not, do a Tilt Ofset calibration or resolder your MPU6050 Gyroscope",
	"Roll", "This value should be close to 90 when iTilt is free flowing in liquid.",
	"Gravity", "Temperature according to DS18B20 (Very accurate)", "Gyro Internal Temperature (1 degree resolution)", "Alcohol by Volume (ABV)",
	"iTilt Calibration Page", 
	"POLYNOMIAL CALIBRATION WIZARD", 
	"This links will not work if you are connected to the iTilt Access point.",
	"Sugar Wash calculators will indicate how much sugar you need to add into a specific amount of water to reach certain gravity",
	"<a href='https://homedistiller.org/wiki/htm/calcs/calcs_rad14701.htm'>homedistiller.org</a>",
	"It is recomended to use a sample size of 7 and the and water volumes and sugar weights provided in the next step.",
	"SAMPLE SIZE", 
	"Submit",
	"CUSTOM PIN CONFIGURATION IS UPDATED AS FOLLOW", 
	"CUSTOM PIN CONFIGURATION for iTilt",
	"You may reconfigure iTilt pins for your custom design. Use only numerical values for GPIO numbers.",
	"Current Pin Configuration",
	"Periph Power Pin",
	"Battery Voltage Pin",
	"Gyro Orientation: Vcc up=1, Vcc down=0",
	"One Wire DS18B20 Pin",
	"i2c SDA MPU6050 Pin",
	"i2C SCL MPU6050 Pin",
	"iTilt Offset Calibration",
	"OFFSET CALIBRATION",
	"Calculated Offset",
	"Insert this value, with iets sign in the",
	"iTilt POLYNOMIAL RESULTS",
	"POLYNOMIAL CALIBRATION WIZARD: RESULTS",
	"TILT VALUES",
	"GRAVITY VALUES",
	"The values applied and copied to",
	"Go and SAVE it. Or reset device to rollback.",
	"Coefficient of determination",
	"(This measures the strenght of the statistical fit. You should get something in the range of 0.98-0.99", //78
	"UPDATE PINS AND RESTART",
	"POLYNOMIAL (MODEL) CALIBRATION WIZARD: SAMPLED DATA",
	"This wizard should assist you in calibrating the iTilt. If you are here, you may already have a calibration data sample set (ordered pairs of Tilt(Measured in <a href='/readings?'>SENSOR READINGS</a>)\
  	and Gravity (Measured with a Hydrometer). If not, you can do it now. Make sure your publication interval is set to 0, and portal time out is 9999. ",
	"We suggest you use a 3L measuring jar, boil 0.48kg sugar in 1.5L clean water. Let the mix cool down to about 20 degrees Celsius. Add your mix to the jar. \
  	Full the jar with extra water until it reaches 2.5L. Stir the content properly (before each measurement). Measure your Tilt, Measure you Gravity (If you have a hydro meter). If not, use the theoretical values.",
  	"The instructions in the table is a guide only, you may ignore them",
	"ALL RECORDS MUST BE COMLETED. The polynomial will be WRONG otherwise",
	"IMPORTANT: Make sure your iTilt is free floating. It must not touch the bottom or two sides of the jar.",
	"WIZARD INSTRUCTIONS AND THEORETICAL GRAVITY",
	"Your sugar content in the jar of 2.5L is 480g. The Theoretical Gravity is 1.077 SG",
	"Before any measurement, remove 600 ml of mix in jar and replace it with 600 ml clean water. Your sugar content in the jar of 2.5L is 365 g. The Theoretical Gravity is 1.056 SG",
	"Before any measurement, remove 900 ml of mix in jar and replace it with 900 ml clean water. Your sugar content in the jar of 2.5L is 233 g. The Theoretical Gravity is 1.036 SG",
	"Before any measurement, remove 900 ml of mix in jar and replace it with 900 ml clean water. Your sugar content in the jar of 2.5L is 149 g. The Theoretical Gravity is 1.023 SG",
	"Before any measurement, remove 1000 ml of mix in jar and replace it with 1000 ml clean water. Your sugar content in the jar of 2.5L is 90 g. The Theoretical Gravity is 1.014 SG",
	"Before any measurement, remove 900 ml of mix in jar and replace it with 900 ml clean water. Your sugar content in the jar of 2.5L is 57 g. The Theoretical Gravity is 1.009 SG",
	"Before any measurement, remove 1100 ml of mix in jar and replace it with 1100 ml clean water. Your sugar content in the jar of 2.5L is 32 g. The Theoretical Gravity is 1.005 SG",
	"Language",
	"Real-Time Graph",
	"Real-Time Data",
	"Histogram",
	"Start/Stop"
	};

const char* const russian[] PROGMEM = 
	{"Настройка WiFi", "Настройки устройства", "Данные датчиков","Настройка гироскопа","Калибровка плотности","Информация","Выход","Обновление прошивки","Настройка пинов",
	"iTilt","Портал настройки iTilt",
	"iTilt Настройка WiFi", "Настройки записаны:", "Но есть ошибки", "Перезагрузите устройство", "Назад",
	"iTilt Настройка пинов", "Для применения настроек iTilt перезагрузится в течении 30 секунд.",
	"iTilt Настройки устройства", "Настройки устройства", "COG сервер", "Логин", "Пароль", "Параметры полинома плотности", "Коэффициент Tilt^3", "Коэффициент Tilt^2", "Коэффициент Tilt^1",
	"Свободный член", "Остальные параметры", "Коэффициент вольтметра", "Интервал отправки данных (с)", "Интервал сохранения данных в автономном режиме (с)", "Начальная плотность", 
	"Смещение гироскопа", "iTilt ID", "Таймаут портала настройки",
	"iTilt Чтение данных", "Чтение данных", "Значение Tilt обновляется раз в 2 секунды.","Заряд аккумулятора (В)", "Заряд аккумулятора (%)", 
	"Tilt(Наклон)", "Если устройство лежит на горизонтальной поверхности, значение должно быть около 89°. Если нет, то требуется настройка смещения гироскопа.",
	"Roll(Крен)", "Если устройство плавает в жидкости, значение должно быть около 90°",
	"Удельный вес", "Точная температура по DS18B20", "Температура по MPU6050 (точность 1°C)", 
	"ABV", //48
	"iTilt калибровка", //49
	"Помощник калибровки", //50 CALIBRATION_H1
	"Вы можете использовать пропорции сахара и воды со следующего шага или воспользоваться калькулятором саханой браги, чтобы получить желаемый удельный вес (SG)", //51
	"<a href='https://homedistiller.org/wiki/htm/calcs/calcs_rad14701.htm'>homedistiller.org</a>", //52
	"(Ссылки не откроются в режиме портала настройки)", //53
	"Рекомендуется сделать 7 замеров используя пропорции из следующего шага", //54
	"Замеры", //55 CALIBRATION_SAMPLE_SIZE
	"Отправить", //56
	"Новая конфигурация портов:", //57
	"Конфигурация портов", //58
	"Вы можете переназначить пины устройства под ваш нестандартный дизайн. Используйте только цифровые значения GPIO.", //59
	"Текущие настройки", //60
	"Питание датчиков", //61
	"Вольтметр", //62
	"Ориентация MPU6050:<br>1 - Vcc сверху / 0 - Vcc снизу", //63
	"Пин DS18B20", //64
	"i2C SDA MPU6050", //PIN_SDA - 65
	"i2C SCL MPU6050", //66
	"iTilt Калибровка смещения", //67
	"Калибровка смещения", //68
	"Полученное смещение", //69
	"Скопируйте это значение в", //70
	"iTilt Полином", //71
	"Результаты калибровки", //72
	"Значения наклона", //73
	"Значения плотности", //74
	"Параметры применены и скопированы в", //75
	"Сохраните их. Или перезагрузите устройство для возврата к предыдущим значениям.", //76
	"Коэффициент детерминации", //77
	"(Если коэффициент детерминации близок к 1, это указывает на то, что модель работает очень хорошо. Вам нужно попасть в диапазон 0.98-0.99)", //78
	"Сохранить и перезагрузить", //79 
	"Помощник полиномиальной калибровки: набор данных", //80
	"Откалибруем ваш iTilt. Здесь нужно ввести значения полученные на странице <a href='/readings?'>Данные датчиков</a>)\
  	и удельный вес SG измеренный гидрометром.", //81
	"Возьмите 3-литровую банку. Добавьте 480г сахара в 1.5л чистой воды. Подогрейте для быстрого растворения. Остудите до 20С. Перелейте в банку. \
  	Долейте в банку воды до объема 2.5л. Тщательно перемешайте. Мешать нужно при каждом добавлении воды. Измерьте наклон iTilt и удельный вес(SG) гидрометром. Если гидрометра нет - используйте рассчетные значения.", //82
  	"Инструкции в таблице опциональны, вы пожете использовать свои пропорции", //83
	"Сделайте все измерения. Иначе полином будет неправильным", //84
	"ВАЖНО: убедитесь, что iTilt плавает свободно. Он не должен касаться дна или двух стенок банки.", //85
	"Указания и рассчетные значения SG", //86
	"480г сахара в 2.5л раствора. Удельный вес 1.077 SG", //87
	"Перед измерением удалите 600мл раствора и долейте 600мл чистой воды. Вы получите 365г сахара в 2.5л раствора. Удельный вес 1.056 SG", //88
	"Перед измерением удалите 900мл раствора и долейте 900мл чистой воды. Вы получите 233г сахара в 2.5л раствора. Удельный вес 1.036 SG", //89
	"Перед измерением удалите 900мл раствора и долейте 900мл чистой воды. Вы получите 149г сахара в 2.5л раствора. Удельный вес 1.023 SG", //90
	"Перед измерением удалите 1000мл раствора и долейте 1000мл чистой воды. Вы получите 90г сахара в 2.5л раствора. Удельный вес 1.014 SG", //91
	"Перед измерением удалите 900мл раствора и долейте 900мл чистой воды. Вы получите 57г сахара в 2.5л раствора. Удельный вес 1.009 SG", //92
	"Перед измерением удалите 1100мл раствора и долейте 1100мл чистой воды. Вы получите 32г сахара в 2.5л раствора. Удельный вес 1.005 SG", //93 - CALIBRATION_SD_1300
	"Язык",
	"Графики", 
	"Поток данных"
	"Гистограмма",
	"Пуск/Стоп"
	};

// Массив языковых пакетов
const char* const* const languages[NUM_LANGUAGES] PROGMEM = {english, russian};


const char* headTemplate = PSTR("<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>%s</title></head>");

const char* styleTemplate = 
	PSTR("<style>\
		body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; color: #000088; font-size:16px; padding:10px; }\
		h1 {text-align:center; font-size:24px;}\
		table {width:100%; border-collapse:collapse; font-size:14px;}\
		th,td {padding:8px; text-align:left;}\
		input {width:100%; height:40px; font-size: 14px; box-sizing: border-box; vertical-align: middle;}\
		button {margin:4px 2px;}\
		.save {width:100%; height:50px; background-color:blue; color:white; border:none; border-radius:10px; font-size:18px;}\
		.back {display:block; text-align:center; margin-top:20px; font-size:16px; color:#000088; text-decoration:none;}\
		.button {background-color:blue; border:none; color:white; padding:15px; text-align:center;\
    		text-decoration:none; display:inline-block; font-size:16px; margin:4px 2px; width:100%;\
    		border-radius:10px; box-sizing:border-box;}\
	</style>");

#endif
