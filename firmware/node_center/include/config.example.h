#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// 1. Cấu hình Wi-Fi (Dùng chung kênh cho ESP-NOW, Webserver LAN và CoreIOT)
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// 2. Địa chỉ MAC mẫu của mạch YoloUNO Slave (Node Cảm biến)
const uint8_t SLAVE_MAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// 3. Chân I2C nối màn hình OLED 0.96 inch (SSD1306)
#define OLED_SDA_PIN 11
#define OLED_SCL_PIN 12

// 4. Cấu hình kết nối CoreIOT (MQTT)
#define ENABLE_DIRECT_COREIOT_MQTT true
#define COREIOT_SERVER "app.coreiot.io"
#define COREIOT_PORT 1883
#define COREIOT_ACCESS_TOKEN "YOUR_COREIOT_DEVICE_TOKEN"

#endif