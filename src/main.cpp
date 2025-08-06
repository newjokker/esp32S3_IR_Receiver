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

  // 初始化红外接器
  initIRReceiver(irrecv);

  // 初始化 wifi 和 北京时间
  initWiFiAndTime();

  // 初始化文件系统
  initFileSystem();

  // 设置Web服务器路由
  initServer();

  Serial.println("COMMANDS: ping | export | clear | (press button to log)");
}

unsigned long lastButtonTime = 0;
const unsigned long debounceTime = 50;

void loop() {
  
  // 处理Web服务器请求
  server.handleClient();

  // 按钮处理 - 非阻塞式
  if (digitalRead(BUTTON_PIN) == LOW && millis() - lastButtonTime > debounceTime) {
      lastButtonTime = millis();
      uint32_t color = getRandomColor();
      // pixels.setPixelColor(0, color);
      // pixels.show();
      set_pixel_color(color);
      appendDataToCSV(color, "button");
  }

  // 处理接收到的红外信号
  handleIRSignal(irrecv, pixels);

  // 串口命令处理
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "export") exportCSVToSerial();
    else if (cmd == "clear") clearCSV();
    else if (cmd == "ping") Serial.println("OK:PONG");
    else Serial.println("ERROR:UNKNOWN_COMMAND");
    Serial.println("--------------------------");
    Serial.println("| ping | export | clear | ");
    Serial.println("--------------------------");
  }
}


