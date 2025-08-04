#include <Arduino.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Adafruit_NeoPixel.h>  // 添加 NeoPixel 库

#define IR_RECEIVE_PIN 10
#define LED_PIN 48  
#define NUM_LEDS 1  

IRrecv irrecv(IR_RECEIVE_PIN);
decode_results results;

// 创建 NeoPixel 对象
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 函数：根据信号值生成固定颜色
uint32_t getColorForSignal(uint64_t signalValue) {
  // 使用信号值作为随机种子
  randomSeed(signalValue);
  
  // 生成RGB颜色分量
  uint8_t r = random(256);
  uint8_t g = random(256);
  uint8_t b = random(256);
  
  return pixels.Color(r, g, b);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);  
  
  // 初始化 NeoPixel
  pixels.begin();
  pixels.clear();  
  pixels.show();  
  
  Serial.println("ESP32-S3 IR Receiver Demo");
  irrecv.enableIRIn();
  Serial.print("Ready to receive IR signals at pin GPIO");
  Serial.println(IR_RECEIVE_PIN);
}

void loop() {
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