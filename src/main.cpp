#include "tools.h"

// ==== 主程序 ====
void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.clear();
  pixels.show();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 
  while (!Serial);
  delay(500);
  Serial.println("ESP32-LOGGER_READY");

  // 初始化 wifi 和 北京时间
  initWiFiAndTime();

  // 初始化文件系统
  initFileSystem();

  Serial.println("COMMANDS: ping | export | clear | (press button to log)");
}

void loop() {
  // 按钮处理
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      uint32_t color = getRandomColor();
      pixels.setPixelColor(0, color);
      pixels.show();
      appendDataToCSV(color);
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }

  // 串口命令处理
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