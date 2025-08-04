#include <Arduino.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>

#define IR_RECEIVE_PIN 10
#define BUTTON_PIN 40  
#define LED_PIN 48  
#define NUM_LEDS 1  

IRrecv irrecv(IR_RECEIVE_PIN);
decode_results results;

// 创建 NeoPixel 对象
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// CSV文件名
const char* csvFilename = "/ir_data.csv";

// 函数：初始化文件系统
bool initFileSystem() {
  if (!LittleFS.begin(true)) {  // true表示如果文件系统不存在则格式化
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
    if (!file.println("Timestamp,Protocol,Value,Bits,Color")) {
      Serial.println("写入表头失败");
      file.close();
      return false;
    }
    file.close();
    Serial.println("CSV文件创建成功");
  }
  return true;
}

// 函数：生成随机颜色
uint32_t getRandomColor() {
  uint8_t r = random(256);
  uint8_t g = random(256);
  uint8_t b = random(256);
  return pixels.Color(r, g, b);
}

// 函数：根据信号值生成固定颜色
uint32_t getColorForSignal(uint64_t signalValue) {
  randomSeed(signalValue);
  return getRandomColor();
}

// 函数：将数据追加到CSV文件
void appendToCSV(const decode_results& results, uint32_t color) {
  if (!LittleFS.exists(csvFilename)) {
    Serial.println("CSV文件不存在");
    return;
  }

  File file = LittleFS.open(csvFilename, FILE_APPEND);
  if (!file) {
    Serial.println("无法打开CSV文件进行写入");
    return;
  }

  // 获取当前时间戳
  String timestamp = String(millis());

  // 写入数据
  file.print(timestamp);
  file.print(",");
  
  // 写入协议类型
  switch(results.decode_type) {
    case NEC: file.print("NEC"); break;
    case SONY: file.print("SONY"); break;
    case RC5: file.print("RC5"); break;
    case RC6: file.print("RC6"); break;
    default: file.print("UNKNOWN"); break;
  }
  file.print(",");
  
  // 写入信号值和位数
  file.print("0x");
  file.print(results.value, HEX);
  file.print(",");
  file.print(results.bits, DEC);
  file.print(",");
  
  // 写入颜色值
  file.print(color, HEX);
  
  if (!file.println()) {
    Serial.println("写入数据失败");
  }
  
  file.close();
  Serial.println("数据已保存到CSV文件");
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
  
  Serial.println("ESP32-S3 IR Receiver with Data Logging");
  irrecv.enableIRIn();
  Serial.print("Ready to receive IR signals at pin GPIO");
  Serial.println(IR_RECEIVE_PIN);
  Serial.println("Press button to generate random color");
}

void loop() {
  // 检查按钮是否按下（低电平）
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // 简单消抖
    if (digitalRead(BUTTON_PIN) == LOW) { // 确认按下
      Serial.println("Button pressed - generating random color");
      
      // 生成随机颜色
      uint32_t color = getRandomColor();
      pixels.setPixelColor(0, color);
      pixels.show();
      
      // 等待按钮释放
      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }
  
  // 处理红外信号
  if (irrecv.decode(&results)) {
    Serial.println("\nReceived IR Signal:");
    serialPrintUint64(results.value, HEX);
    Serial.print(" (");
    Serial.print(results.bits, DEC);
    Serial.println(" bits)");
    
    Serial.print("Protocol: ");
    switch(results.decode_type) {
      case NEC: Serial.println("NEC"); break;
      case SONY: Serial.println("SONY"); break;
      case RC5: Serial.println("RC5"); break;
      case RC6: Serial.println("RC6"); break;
      case UNKNOWN: 
      default: 
        Serial.println("UNKNOWN"); 
        Serial.println("Raw data:");
        Serial.println(resultToHumanReadableBasic(&results));
        break;
    }
    
    // 为当前信号获取颜色并设置LED
    uint32_t color = getColorForSignal(results.value);
    pixels.setPixelColor(0, color);
    pixels.show();
    
    // 将数据保存到CSV文件
    appendToCSV(results, color);
    
    irrecv.resume();
  }
  delay(100);
}

// 导出CSV文件内容到串口
void exportCSVToSerial() {
  if (!LittleFS.exists(csvFilename)) {
    Serial.println("CSV文件不存在");
    return;
  }
  
  File file = LittleFS.open(csvFilename, FILE_READ);
  if (!file) {
    Serial.println("无法打开CSV文件进行读取");
    return;
  }
  
  Serial.println("\nCSV文件内容:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}