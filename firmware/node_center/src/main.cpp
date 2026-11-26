#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "relay_driver.h"

void taskRelayTest(void *pvParameters) {
  for (;;) {
    // 1. Test Bơm 3 giây
    setPump(true);
    vTaskDelay(pdMS_TO_TICKS(3000));
    setPump(false);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 2. Test Quạt 3 giây
    setFan(true);
    vTaskDelay(pdMS_TO_TICKS(3000));
    setFan(false);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  initRelays();

  // In sẵn địa chỉ MAC của YoloUNO để đưa cho bạn làm ESP-NOW (T06)
  WiFi.mode(WIFI_STA);
  Serial.println("=== KHOI DONG NODE CENTER (YOLOUNO ESP32-S3) ===");
  Serial.print("Dia chi MAC YoloUNO: ");
  Serial.println(WiFi.macAddress());

  xTaskCreate(taskRelayTest, "RelayTestTask", 4096, NULL, 1, NULL);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}