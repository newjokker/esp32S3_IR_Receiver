#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>

// 硬件配置
extern const char* csvFilename;
extern Adafruit_NeoPixel pixels;

// 辅助函数声明
bool initFileSystem();
uint32_t getRandomColor();
String getISOTimestamp();
void appendDataToCSV(uint32_t color);
void exportCSVToSerial();
void clearCSV();
void initWiFiAndTime();

#endif