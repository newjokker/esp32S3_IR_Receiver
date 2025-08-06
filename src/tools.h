#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <WebServer.h>

// 硬件配置
extern const char* csvFilename;
extern Adafruit_NeoPixel pixels;
extern IRrecv irrecv;
extern WebServer server;

// 辅助函数声明
bool initFileSystem();
uint32_t getRandomColor();
String getISOTimestamp();
void appendDataToCSV(uint32_t color, std::string who);
void exportCSVToSerial();
void clearCSV();
void initWiFiAndTime();

void initIRReceiver(IRrecv &irrecv);
void handleIRSignal(IRrecv &irrecv, Adafruit_NeoPixel &pixels);
uint32_t getColorForSignal(uint64_t signalValue);

void initServer();
void handleRoot();
void handleColor();
void handleNotFound();

#endif