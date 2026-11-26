#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include "espnow.h"

// Khởi tạo màn hình OLED 0.96" (SSD1306 128x64, địa chỉ I2C 0x3C)
bool oled_init(int sda_pin, int scl_pin);

// Hiển thị màn hình khởi động kèm địa chỉ MAC của Node Center
void oled_show_boot(const String &mac_addr);

// Cập nhật toàn bộ thông số cảm biến, gói tin và trạng thái kết nối lên OLED
void oled_update_telemetry(
    const struct_telemetry &data,
    uint32_t rx_count,
    bool wifi_connected,
    bool coreiot_connected);

#endif