/* VL53L0X with TCA9548A - Auto-resurrection of disabled sensors. */

#include <Wire.h>
#include <VL53L0X.h>

const uint8_t TCA_ADDR = 0x73;
const uint8_t MAX_SENSORS = 4;
const uint8_t SENSOR_ADDRESS = 0x29;

VL53L0X sensors[MAX_SENSORS];
bool sensorActive[MAX_SENSORS] = {false, false, false, false};
uint8_t timeoutCount[MAX_SENSORS] = {0, 0, 0, 0};
uint16_t lastValidDistance[MAX_SENSORS] = {0, 0, 0, 0};
bool wasDisabled[MAX_SENSORS] = {false, false, false, false};
const uint8_t MAX_TIMEOUTS = 5;
const uint16_t MAX_DISTANCE_CHANGE = 2000;
const uint32_t RESURRECTION_INTERVAL = 10000;
uint8_t activeSensors = 0;
bool diagnosticMode = false;
uint32_t lastResurrectionTime = 0;

void selectChannel(uint8_t channel)
{
  if (channel > 7) return;
  Wire.flush();
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(25);
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
      Serial.print("x No device on channel ");
      Serial.println(i);
      continue;
    }

    sensors[i].setAddress(SENSOR_ADDRESS);
    sensors[i].setTimeout(200);

    if (sensors[i].init())
    {
      sensorActive[i] = true;
      Serial.print("OK Sensor on channel ");
      Serial.println(i);
      activeSensors++;
    }
    else
    {
      Serial.print("x Init failed on channel ");
      Serial.println(i);
    }
  }

  Serial.print("\nDetected ");
  Serial.print(activeSensors);
  Serial.println(" Sensors");

  if (activeSensors == 0)
  {
    Serial.println("\nNo sensors found - entering diagnostic mode\n");
    diagnosticMode = true;
  }
  else
  {
    Serial.println("Ready.\n");
  }
}

void loop()
{
  uint32_t now = millis();

  if (!diagnosticMode && now - lastResurrectionTime >= RESURRECTION_INTERVAL)
  {
    lastResurrectionTime = now;

    for (uint8_t i = 0; i < MAX_SENSORS; i++)
    {
      if (sensorActive[i]) continue;
      if (!wasDisabled[i]) continue;

      selectChannel(i);

      if (checkDevicePresent())
      {
        sensors[i].setAddress(SENSOR_ADDRESS);
        sensors[i].setTimeout(200);

        if (sensors[i].init())
        {
          sensorActive[i] = true;
          timeoutCount[i] = 0;
          lastValidDistance[i] = 0;
          activeSensors++;

          Serial.print("{\"channel\":");
          Serial.print(i);
          Serial.println(",\"status\":\"resurrected\"}");
        }
      }
    }
  }

  if (diagnosticMode)
  {
    for (uint8_t ch = 0; ch < 4; ch++)
    {
      Serial.print("{\"channel\":");
      Serial.print(ch);
      Serial.print(",\"devices\":[");

      selectChannel(ch);

      bool found = false;
      for (uint8_t addr = 1; addr < 127; addr++)
      {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
          if (found) Serial.print(",");
          Serial.print("\"0x");
          if (addr < 16) Serial.print("0");
          Serial.print(addr, HEX);
          Serial.print("\"");
          found = true;
        }
      }

      Serial.println("]}");
    }
    Serial.println();
    delay(3000);
  }
  else
  {
    Serial.print("{\"detected\":");
    Serial.print(activeSensors);
    Serial.print(",\"distances\":[");

    bool first = true;
    for (uint8_t i = 0; i < MAX_SENSORS; i++)
    {
      if (!sensorActive[i]) continue;

      selectChannel(i);

      bool isTimeout = false;
      uint16_t distance = sensors[i].readRangeSingleMillimeters();

      if (sensors[i].timeoutOccurred())
      {
        isTimeout = true;
        timeoutCount[i]++;

        if (timeoutCount[i] >= MAX_TIMEOUTS)
        {
          wasDisabled[i] = true;
          sensorActive[i] = false;
          activeSensors--;
          Serial.print("{\"channel\":");
          Serial.print(i);
          Serial.print(",\"status\":\"disabled\"}");
          first = false;
          continue;
        }
      }
      else
      {
        timeoutCount[i] = 0;

        if (lastValidDistance[i] > 0)
        {
          int16_t change = (int16_t)distance - (int16_t)lastValidDistance[i];
          if (abs(change) > MAX_DISTANCE_CHANGE)
          {
            isTimeout = true;
            distance = lastValidDistance[i];
          }
        }
        lastValidDistance[i] = distance;
      }

      if (!first) Serial.print(",");
      first = false;

      Serial.print("{\"channel\":");
      Serial.print(i);
      Serial.print(",\"distance\":");
      Serial.print(distance);
      Serial.print(",\"timeout\":");
      Serial.print(isTimeout ? "true" : "false");
      Serial.print("}");
    }

    Serial.println("]}");
    delay(100);
  }
}
