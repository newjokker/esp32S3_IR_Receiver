# 说明

### 环境

```bash

platformio lib install "Adafruit GFX Library"

platformio lib install "Adafruit SSD1306"

platformio lib install "DHT sensor library"

platformio lib install "PubSubClient"

pio lib install "Adafruit NeoPixel"

```

### 需求

* 可以快速去采集数据，但是采集的数据都保存在本地的文件系统中或者哪个存储中（断电可恢复），数据一小时或者指定周期上传一次

* 可以换一个板子 esp32C6 看看省电的效果, 


### 注意点

* 将写到文件系统的中数据拿出来时候，一定不要开 monitor 数据会被它截取，而没法写到日志中去。







