#include <Arduino.h>
#include "config.h"
#include "sensor_driver.h"

SensorPacket_t sensorData;

void taskReadSensors(void *pvParameters) {
  for (;;) {
    if (readSoilRS485(sensorData.soilMoisture, sensorData.soilTemp)) {
      Serial.printf("[RS485 OK] Do am dat: %.1f%% | Nhiet do dat: %.1f C\n",
                    sensorData.soilMoisture, sensorData.soilTemp);
    }

    if (readAirDHT20(sensorData.airHumidity, sensorData.airTemp)) {
      Serial.printf("[DHT20 OK] Do am KK:  %.1f%% | Nhiet do KK:  %.1f C\n",
                    sensorData.airHumidity, sensorData.airTemp);
    }

    Serial.println("--------------------------------------------------");
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(500));

  Serial.println("=== KHOI DONG NODE SENSOR (ESP32 RS485 + DHT20) ===");
  initSensors();

  xTaskCreate(taskReadSensors, "ReadSensorsTask", 4096, NULL, 1, NULL);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}