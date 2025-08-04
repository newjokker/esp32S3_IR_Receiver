#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>

#define BUTTON_PIN 40
#define LED_PIN 48
#define NUM_LEDS 1

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
const char* csvFilename = "/data.csv";

// 初始化文件系统并创建 CSV 文件（带表头）
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
    file.println("Timestamp(ms),Color(HEX)");
    file.close();
  }

  Serial.println("OK:FILESYSTEM_READY");
  return true;
}

uint32_t getRandomColor() {
  return pixels.Color(random(256), random(256), random(256));
}

void appendDataToCSV(uint32_t color) {
  File file = LittleFS.open(csvFilename, FILE_APPEND);
  if (!file) {
    Serial.println("ERROR:CSV_WRITE_FAIL");
    return;
  }
  file.printf("%lu,%06X\n", millis(), color & 0xFFFFFF);  // HEX:6位输出
  file.close();
  Serial.println("OK:DATA_SAVED");
}

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

void clearCSV() {
  if (LittleFS.remove(csvFilename)) {
    Serial.println("OK:CSV_CLEARED");
    initFileSystem(); // 重新创建
  } else {
    Serial.println("ERROR:CSV_DELETE_FAIL");
  }
}

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.clear();
  pixels.show();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  while (!Serial);  // 等串口准备好
  delay(500);
  Serial.println("ESP32-LOGGER_READY");

  initFileSystem();
  Serial.println("COMMANDS: ping | export | clear | (press button to log)");
}

void loop() {
  // 按钮被按下
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // 消抖
    if (digitalRead(BUTTON_PIN) == LOW) {
      uint32_t color = getRandomColor();
      pixels.setPixelColor(0, color);
      pixels.show();
      appendDataToCSV(color);

      while (digitalRead(BUTTON_PIN) == LOW) delay(10);  // 等待释放
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
