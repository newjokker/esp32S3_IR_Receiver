#include <Arduino.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Adafruit_NeoPixel.h>

#define IR_RECEIVE_PIN 10
#define BUTTON_PIN 40  
#define LED_PIN 48  
#define NUM_LEDS 1  

IRrecv irrecv(IR_RECEIVE_PIN);
decode_results results;

// 创建 NeoPixel 对象
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 函数：生成随机颜色
uint32_t getRandomColor() {
  uint8_t r = random(256);
  uint8_t g = random(256);
  uint8_t b = random(256);
  return pixels.Color(r, g, b);
}

// 函数：根据信号值生成固定颜色
uint32_t getColorForSignal(uint64_t signalValue) {
  // 使用信号值作为随机种子
  randomSeed(signalValue);
  return getRandomColor();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);  
  
  // 初始化按钮引脚
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // 初始化 NeoPixel
  pixels.begin();
  pixels.clear();  
  pixels.show();  
  
  Serial.println("ESP32-S3 IR Receiver with Button Demo");
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
    
    irrecv.resume();
  }
  delay(100);
}