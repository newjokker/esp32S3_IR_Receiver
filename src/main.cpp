#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>

// ==== 配置 ====
#define BUTTON_PIN 40
#define LED_PIN 48
#define NUM_LEDS 1

const char* csvFilename = "/data.csv";
const char* ssid     = "Saturn-Guest-2.4g";
const char* password = "Tuxingkeji-0918";

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ==== 初始化文件系统并创建 CSV 文件 ====
bool initFileSystem() {
  if (!LittleFS.begin(true)) {
    Serial.println("ERROR:FILESYSTEM_INIT_FAIL");
    return false;
  }

  if (!LittleFS.exists(csvFilename)) {
    File file = LittleFS.open(csvFilename, FILE_WRITE);
    if (!file) {
      Serial.println("ERROR:CSV_CREATE_FAIL");
      return false;
    }
    file.println("Timestamp(ISO8601),Uptime(ms),Color(HEX),ButtonStatus");
    file.close();
  }

  Serial.println("OK:FILESYSTEM_READY");
  return true;
}

// ==== 获取随机颜色 ====
uint32_t getRandomColor() {
  return pixels.Color(random(256), random(256), random(256));
}

// ==== 获取 ISO 格式时间 ====
String getISOTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "UNKNOWN_TIME";
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  return String(buffer);
}

// ==== 写入数据到 CSV ====
void appendDataToCSV(uint32_t color) {
  File file = LittleFS.open(csvFilename, FILE_APPEND);
  if (!file) {
    Serial.println("ERROR:CSV_WRITE_FAIL");
    return;
  }
  String isoTime = getISOTimestamp();
  file.printf("%s,%lu,%06X,Pressed\n", isoTime.c_str(), millis(), color & 0xFFFFFF);
  file.close();
  Serial.println("OK:DATA_SAVED");
}

// ==== 将 CSV 内容导出到串口 ====
void exportCSVToSerial() {
  if (!LittleFS.exists(csvFilename)) {
    Serial.println("ERROR:CSV_NOT_FOUND");
    return;
  }

  File file = LittleFS.open(csvFilename, FILE_READ);
  if (!file) {
    Serial.println("ERROR:CSV_OPEN_FAIL");
    return;
  }

  Serial.println("CSV_START");
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      Serial.println(line);
    }
  }
  Serial.println("CSV_END");
  file.close();
}

// ==== 清除 CSV 文件 ====
void clearCSV() {
  if (LittleFS.remove(csvFilename)) {
    Serial.println("OK:CSV_CLEARED");
    initFileSystem(); // 重新创建
  } else {
    Serial.println("ERROR:CSV_DELETE_FAIL");
  }
}

// ==== 初始化 WiFi 与时间 ====
void initWiFiAndTime() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.println("Time synced: " + getISOTimestamp());
    } else {
      Serial.println("ERROR:TIME_SYNC_FAIL");
    }
  } else {
    Serial.println("\nERROR:WIFI_CONNECT_FAIL");
  }
}

// ==== setup ====
void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.clear();
  pixels.show();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  while (!Serial);
  delay(500);
  Serial.println("ESP32-LOGGER_READY");

  initWiFiAndTime();
  initFileSystem();

  Serial.println("COMMANDS: ping | export | clear | (press button to log)");
}

// ==== loop ====
void loop() {
  // 按钮按下事件
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // 消抖
    if (digitalRead(BUTTON_PIN) == LOW) {
      uint32_t color = getRandomColor();
      pixels.setPixelColor(0, color);
      pixels.show();
      appendDataToCSV(color);

      while (digitalRead(BUTTON_PIN) == LOW) delay(10);  // 等待按钮释放
    }
  }

  // 处理串口命令
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "export") exportCSVToSerial();
    else if (cmd == "clear") clearCSV();
    else if (cmd == "ping") Serial.println("OK:PONG");
    else Serial.println("ERROR:UNKNOWN_COMMAND");
  }

  delay(100);
}
