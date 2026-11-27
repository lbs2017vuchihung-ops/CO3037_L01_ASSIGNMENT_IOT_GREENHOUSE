#pragma once
#include <Arduino.h>

// =========================================================================
// 1. CẤU HÌNH MẠNG & ESP-NOW (T06)
// =========================================================================
// Tên Wi-Fi Router mà Node Center đang kết nối (để Node Slave quét đồng bộ kênh)
#define WIFI_SSID "YOUR_WIFI_SSID"

// Địa chỉ MAC mẫu của mạch YoloUNO Node Center
const uint8_t CENTER_MAC_ADDR[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// =========================================================================
// 2. CHÂN GIAO TIẾP CẢM BIẾN (T02 & CÁC CẢM BIẾN MÔI TRƯỜNG)
// =========================================================================
// Cảm biến NPK chuẩn Modbus RS485 qua module MAX485 (UART1)
#define RS485_RX_PIN 17      // Nối qua cầu phân áp từ chân RO của MAX485
#define RS485_TX_PIN 18      // Nối vào chân DI của MAX485
#define MAX485_DE_RE_PIN 4   // Nối chập DE & RE của MAX485
#define RS485_BAUD_RATE 9600 // Đổi thành 4800 nếu cảm biến NPK dùng baud 4800
#define RS485_SLAVE_ID 1

// Bus I2C dùng chung cho cảm biến DHT20 (0x38) và màn hình LCD I2C (0x27)
#define I2C_SDA_PIN 11
#define I2C_SCL_PIN 12

// Cảm biến Analog (Bắt buộc dùng chân thuộc nhóm ADC1: GPIO 1..10 trên ESP32-S3)
#define SOIL_MOISTURE_PIN 1 // Cổng A0: Cảm biến độ ẩm đất 2 càng
#define LIGHT_SENSOR_PIN 2  // Cổng A1: Cảm biến ánh sáng

// =========================================================================
// 3. CHÂN CƠ CẤU CHẤP HÀNH: RELAY MÁY BƠM & ĐÈN GIAO THÔNG
// =========================================================================
#define PUMP_RELAY_PIN 5 // Chân kích Relay đóng/cắt máy bơm (Active HIGH)

#define LED_GREEN_PIN 6  // Đèn Xanh: An toàn / Tắt bơm
#define LED_YELLOW_PIN 7 // Đèn Vàng: Thiếu ẩm nhẹ / Bơm ít
#define LED_RED_PIN 8    // Đèn Đỏ:   Đất quá khô / Bơm nhiều

// Ngưỡng độ ẩm đất (%) cho chế độ tưới tự động
#define SOIL_THRESHOLD_LOW 40  // Dưới 40% -> Mức Vàng (Bơm ít)
#define SOIL_THRESHOLD_CRIT 25 // Dưới 25% -> Mức Đỏ (Bơm nhiều)

#define FAN_RELAY_PIN 9          // Chân kích Relay kênh 2 điều khiển quạt
#define TEMP_THRESHOLD_HIGH 35.0 // Ngưỡng nhiệt độ tự động bật quạt (°C)