#pragma once
#include <Arduino.h>
#include "config.h"

void initRelays() {
  // Ghi mức OFF trước khi chuyển sang OUTPUT để chống nháy relay lúc cấp nguồn
  digitalWrite(RELAY_PUMP_PIN, RELAY_OFF);
  digitalWrite(RELAY_FAN_PIN, RELAY_OFF);
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);
}

void setPump(bool state) {
  digitalWrite(RELAY_PUMP_PIN, state ? RELAY_ON : RELAY_OFF);
  Serial.printf("[RELAY] May bom: %s\n", state ? "BAT" : "TAT");
}

void setFan(bool state) {
  digitalWrite(RELAY_FAN_PIN, state ? RELAY_ON : RELAY_OFF);
  Serial.printf("[RELAY] Quat thong gio: %s\n", state ? "BAT" : "TAT");
}