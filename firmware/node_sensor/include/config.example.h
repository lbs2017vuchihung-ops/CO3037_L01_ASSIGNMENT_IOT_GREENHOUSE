#pragma once
#include <Arduino.h>

// Chân giao tiếp MAX485 (UART2)
#define MAX485_DE_RE_PIN  4
#define RS485_RX_PIN      16
#define RS485_TX_PIN      17

// Thông số Modbus RS485 (Cảm biến T02)
#define RS485_BAUD_RATE   9600   // Đổi thành 4800 nếu datasheet cảm biến yêu cầu
#define RS485_SLAVE_ID    1

// Chân I2C cho cảm biến DHT20
#define I2C_SDA_PIN       21
#define I2C_SCL_PIN       22

// Địa chỉ MAC của YoloUNO node_center (cập nhật khi ghép ESP-NOW T06)
const uint8_t CENTER_MAC_ADDR[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Cấu trúc gói tin cảm biến truyền qua ESP-NOW
typedef struct __attribute__((packed)) {
  float soilMoisture;
  float soilTemp;
  float airTemp;
  float airHumidity;
} SensorPacket_t;