/* VL53L0X with TCA9548A - Simplified detection. */

#include <Wire.h>
#include <VL53L0X.h>

const uint8_t TCA_ADDR = 0x73;
const uint8_t MAX_SENSORS = 4;
const uint8_t SENSOR_ADDRESS = 0x29;

VL53L0X sensors[MAX_SENSORS];
bool sensorActive[MAX_SENSORS] = {false, false, false, false};
uint8_t activeSensors = 0;

void selectChannel(uint8_t channel)
{
  if (channel > 7) return;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(10);
}

bool checkDevicePresent()
{
  Wire.beginTransmission(SENSOR_ADDRESS);
  return Wire.endTransmission() == 0;
}

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  esp_log_level_set("i2c.master", ESP_LOG_NONE);

  Serial.println("\n=== VL53L0X with TCA9548A ===");
  Serial.println("Scanning channels 0-3...\n");

  for (uint8_t i = 0; i < MAX_SENSORS; i++)
  {
    selectChannel(i);

    if (!checkDevicePresent())
    {
      Serial.print("✗ No device on channel ");
      Serial.println(i);
      continue;
    }

    sensors[i].setAddress(SENSOR_ADDRESS);
    sensors[i].setTimeout(500);

    if (sensors[i].init())
    {
      sensorActive[i] = true;
      Serial.print("✓ Sensor on channel ");
      Serial.println(i);
      activeSensors++;
    }
    else
    {
      Serial.print("✗ Init failed on channel ");
      Serial.println(i);
    }
  }

  Serial.print("\nDetected ");
  Serial.print(activeSensors);
  Serial.println(" Sensors");

  if (activeSensors == 0)
  {
    Serial.println("\nERROR: No sensors!");
    while (1) {}
  }

  Serial.println("Ready.\n");
}

void loop()
{
  Serial.print("{\"detected\":");
  Serial.print(activeSensors);
  Serial.print(",\"distances\":[");

  bool first = true;
  for (uint8_t i = 0; i < MAX_SENSORS; i++)
  {
    if (sensorActive[i])
    {
      selectChannel(i);
      uint16_t distance = sensors[i].readRangeSingleMillimeters();

      if (!first) Serial.print(",");
      first = false;

      Serial.print("{\"channel\":");
      Serial.print(i);
      Serial.print(",\"distance\":");
      Serial.print(distance);
      Serial.print("}");
    }
  }

  Serial.println("]}");
}
