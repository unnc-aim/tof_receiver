#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;

// 切换 TCA9548A 通道
void selectI2CChannel(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(0x70); 
  Wire.write(1 << channel);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200); // 串口初始化
  Wire.begin(21, 22);   // I2C 初始化 (21=SDA, 22=SCL)

  // 初始化 4 个通道上的传感器
  for (uint8_t i = 0; i < 4; i++) {
    selectI2CChannel(i);
    sensor.setTimeout(500);
    if (sensor.init()) {
      sensor.startContinuous();
    }
  }
}

void loop() {
  int16_t distances[4];

  // 循环读取 4 路数据
  for (uint8_t i = 0; i < 4; i++) {
    selectI2CChannel(i);
    distances[i] = sensor.readRangeContinuousMillimeters();
  }

  // ── 直接通过物理串口发送文本（格式例如：120,450,1100,850） ──
  Serial.print(distances[0]); Serial.print(",");
  Serial.print(distances[1]); Serial.print(",");
  Serial.print(distances[2]); Serial.print(",");
  Serial.println(distances[3]); // 以换行符结尾

  delay(20); // 50Hz 的刷新率
}