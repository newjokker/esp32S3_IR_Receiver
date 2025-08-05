#include "tools.h"

// ==== 全局变量定义 ====
const char* csvFilename = "/data.csv";
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ==== 文件系统初始化 ====
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

// ==== 获取ISO格式时间 ====
String getISOTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "UNKNOWN_TIME";
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  return String(buffer);
}

// ==== 数据记录到CSV ====
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

// ==== 导出CSV数据 ====
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
    Serial.println(file.readStringUntil('\n'));
  }
  Serial.println("CSV_END");
  file.close();
}

// ==== 清空CSV文件 ====
void clearCSV() {
  if (LittleFS.remove(csvFilename)) {
    Serial.println("OK:CSV_CLEARED");
    initFileSystem();
  } else {
    Serial.println("ERROR:CSV_DELETE_FAIL");
  }
}

// ==== WiFi和时间初始化 ====
void initWiFiAndTime() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
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