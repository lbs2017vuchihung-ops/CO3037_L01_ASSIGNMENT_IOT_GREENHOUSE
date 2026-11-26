#pragma once
#include <Arduino.h>

// Chân GPIO trên ESP32-S3 YoloUNO nối với Relay 2 kênh
#define RELAY_PUMP_PIN    8   // D5 -> IN1 (Máy bơm tưới mini)
#define RELAY_FAN_PIN     9   // D6 -> IN2 (Quạt thông gió)

// Mức logic kích Relay (LOW = Bật, HIGH = Tắt cho module opto-isolated)
#define RELAY_ON          LOW
#define RELAY_OFF         HIGH