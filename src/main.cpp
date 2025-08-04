#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>

#define BUTTON_PIN 40  
#define LED_PIN 48  
#define NUM_LEDS 1  

// 创建 NeoPixel 对象
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// CSV文件名
const char* csvFilename = "/data.csv";

// 函数声明
void exportCSVToSerial();
bool initFileSystem();
uint32_t getRandomColor();
void appendDataToCSV(uint32_t color);

// 初始化文件系统
bool initFileSystem() {
  if (!LittleFS.begin(true)) {
    Serial.println("无法挂载文件系统");
    return false;
  }
  
  Serial.println("文件系统挂载成功");
  
  // 检查文件是否存在，不存在则创建并写入表头
  if (!LittleFS.exists(csvFilename)) {
    File file = LittleFS.open(csvFilename, FILE_WRITE);
    if (!file) {
      Serial.println("无法创建CSV文件");
      return false;
    }
    if (!file.println("Timestamp,Color")) {
      Serial.println("写入表头失败");
      file.close();
      return false;
    }
    file.close();
    Serial.println("CSV文件创建成功");
  }
  return true;
}

// 生成随机颜色
uint32_t getRandomColor() {
  uint8_t r = random(256);
  uint8_t g = random(256);
  uint8_t b = random(256);
  return pixels.Color(r, g, b);
}

// 添加数据到CSV
void appendDataToCSV(uint32_t color) {
  File file = LittleFS.open(csvFilename, FILE_APPEND);
  if (!file) {
    Serial.println("无法打开CSV文件进行写入");
    return;
  }

  // 写入时间戳和颜色值
  file.print(millis());
  file.print(",");
  file.println(color, HEX);
  
  file.close();
  Serial.println("数据已保存到CSV文件");
}

void exportCSVToSerial() {
  if (!LittleFS.exists(csvFilename)) {
    Serial.println("ERROR:CSV_NOT_FOUND");
    return;
  }
  
  File file = LittleFS.open(csvFilename, FILE_READ);
  if (!file) {
    Serial.println("ERROR:FILE_OPEN_FAILED");
    return;
  }

  // 添加起始标记
  Serial.println("CSV_START");
  Serial.flush();
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      Serial.println(line); // 确保每行以换行结束
      Serial.flush();
    }
  }
  
  // 添加结束标记
  Serial.println("CSV_END");
  Serial.flush();
  
  file.close();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);  
  
  // 初始化文件系统
  if (!initFileSystem()) {
    Serial.println("文件系统初始化失败，数据将不会被保存");
  }
  
  // 初始化按钮引脚
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // 初始化 NeoPixel
  pixels.begin();
  pixels.clear();  
  pixels.show();  
  
  Serial.println("ESP32-S3 数据记录器");
  Serial.println("按下按钮记录数据");
}

void loop() {
  // 检查按钮是否按下（低电平）
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // 消抖
    if (digitalRead(BUTTON_PIN) == LOW) { // 确认按下
      Serial.println("按钮按下 - 记录数据");
      
      // 生成随机颜色
      uint32_t color = getRandomColor();
      pixels.setPixelColor(0, color);
      pixels.show();
      
      // 记录数据到CSV
      appendDataToCSV(color);
      
      // 等待按钮释放
      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }
  
  // 处理串口命令
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "export") {
      exportCSVToSerial();
    }
    else if (command == "clear") {
      if (LittleFS.remove(csvFilename)) {
        Serial.println("CSV文件已删除");
        initFileSystem(); // 重新创建文件
      }
    }
  }
  
  delay(100);
}

