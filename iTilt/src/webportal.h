#ifndef WEBPORTAL_H
#define WEBPORTAL_H

#include <Arduino.h>


void runConfigurationPortal();

size_t getHtmlMenu( char* buffer, size_t bufferSize );
void handleNotFound();
void handleFavicon();
void runConfigurationPortal();
void bindServerCallback();
void handleRoute();
void handleReadings();
void handleOffsetCalibration();
void handlePolynomialCalibrationStart();
void handlePolynomialCalibrationInput();
void handlePolynomialCalibrationResults();
void handlePinConfInput();
void handlePinConfResults();
void handleDeviceConfInput();
void handleDeviceConfResults();
void getReadingsHtml(char* buffer, size_t bufferSize, float batvolt, float batp, float tilt, float roll, float grav, float temp, float gyro_temp, float abv, float ema);
void doWiFiManager();
void handleExit(void);

#endif