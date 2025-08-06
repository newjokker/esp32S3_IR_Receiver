#include "tools.h"

// ==== 全局变量定义 ====
const char* csvFilename = "/data.csv";
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

IRrecv irrecv(IR_RECEIVE_PIN);

WebServer server(80);

// Pixel 亮一下
void set_pixel_color(uint32_t color){
    pixels.setPixelColor(0, color);
    pixels.show();
    delay(100);
    pixels.clear();
    pixels.show();
}

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
    file.println("Timestamp(ISO8601),Uptime(ms),Color(HEX),Who");
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
void appendDataToCSV(uint32_t color, std::string who) {
  File file = LittleFS.open(csvFilename, FILE_APPEND);
  if (!file) {
    Serial.println("ERROR:CSV_WRITE_FAIL");
    return;
  }
  String isoTime = getISOTimestamp();
  file.printf("%s,%lu,%06X,%s\n", isoTime.c_str(), millis(), color & 0xFFFFFF, who.c_str());
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

// 服务初始化
void initServer(){
  server.on("/", handleRoot);          // 主页
  server.on("/setcolor", handleColor); // 设置颜色
  server.onNotFound(handleNotFound);   // 404处理
  
  server.begin();  // 启动Web服务器
  Serial.println("HTTP server started");
  
  // 打印IP地址
  Serial.print("Use this URL to connect: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

// 处理根路径
void handleRoot() {
  String html = R"====(
<html>
<head>
  <title>ESP32 LED Control</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
    button {
      padding: 15px 30px;
      font-size: 18px;
      background-color: #4CAF50;
      color: white;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    button:hover { background-color: #45a049; }
    .color-box {
      width: 200px;
      height: 200px;
      margin: 20px auto;
      border: 2px solid #333;
    }
  </style>
</head>
<body>
  <h1>ESP32 LED Color Generator</h1>
  <div class="color-box" id="colorBox"></div>
  <button onclick="generateColor()">Generate Random Color</button>
  
  <script>
    function generateColor() {
      // 生成随机RGB值
      const r = Math.floor(Math.random() * 256);
      const g = Math.floor(Math.random() * 256);
      const b = Math.floor(Math.random() * 256);
      
      // 显示颜色
      document.getElementById('colorBox').style.backgroundColor = 
        `rgb(${r},${g},${b})`;
      
      // 发送到服务器
      fetch(`/setcolor?rgb=${r},${g},${b}`)
        .then(response => response.text())
        .then(data => console.log(data))
        .catch(error => console.error('Error:', error));
    }
  </script>
</body>
</html>
  )====";
  
  server.send(200, "text/html", html);
}

// 处理颜色设置请求
void handleColor() {
  if (server.hasArg("rgb")) {
    String rgbStr = server.arg("rgb");
    int r, g, b;
    
    // 解析RGB值
    if (sscanf(rgbStr.c_str(), "%d,%d,%d", &r, &g, &b) == 3) {
      // 限制数值范围
      r = constrain(r, 0, 255);
      g = constrain(g, 0, 255);
      b = constrain(b, 0, 255);
      
      // 设置LED颜色
      uint32_t color = pixels.Color(r, g, b);
      // pixels.setPixelColor(0, color);
      // pixels.show();
      set_pixel_color(color);
      
      // 记录到CSV
      appendDataToCSV(color, "web");
      
      server.send(200, "text/plain", "Color set to R:" + String(r) + " G:" + String(g) + " B:" + String(b));
    } else {
      server.send(400, "text/plain", "Invalid color format. Use RRR,GGG,BBB");
    }
  } else {
    server.send(400, "text/plain", "Missing 'rgb' parameter");
  }
}

// 处理404错误
void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

// 初始化红外接收
void initIRReceiver(IRrecv &irrecv) {
  irrecv.enableIRIn();
  Serial.print("Ready to receive IR signals at pin GPIO");
  Serial.println(IR_RECEIVE_PIN);
}

// 根据信号值生成颜色
uint32_t getColorForSignal(uint64_t signalValue) {
  randomSeed(signalValue);
  return getRandomColor();
}

// 打印红外信号详情
void printIRDetails(decode_results &results) {
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
    default: 
      Serial.println("UNKNOWN"); 
      Serial.println("Raw data:");
      Serial.println(resultToHumanReadableBasic(&results));
      break;
  }
}

// 处理红外信号
void handleIRSignal(IRrecv &irrecv, Adafruit_NeoPixel &pixels) {
  decode_results results;
  if (irrecv.decode(&results)) {
    
    // 过滤重复信号
    if (results.value != 0xFFFFFFFFFFFFFFFF) {  
      printIRDetails(results);
      uint32_t color = getColorForSignal(results.value);
      
      // pixels.setPixelColor(0, color);
      // pixels.show();

      set_pixel_color(color);

      appendDataToCSV(color, "ir_reciver");
    } else {
      Serial.println("忽略重复信号");
    }

    irrecv.resume();
  }
}

