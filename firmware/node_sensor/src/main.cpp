#include <Arduino.h>
#include "espnow.h"
#include "dht20_sensor.h"
#include "soil_sensor.h"
#include "npk_sensor.h"
#include "light_sensor.h"
#include "actuator.h"

uint8_t CENTER_MAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; // Đổi thành MAC thật của Center

void on_remote_command(PumpLevel target_level, bool auto_mode)
{
  actuator_set_auto_mode(auto_mode);
  actuator_set_level(target_level); // Cập nhật đồng thời Relay bơm + Đèn Xanh/Vàng/Đỏ
}

void setup()
{
  Serial.begin(115200);
  dht20_init();
  soil_init();
  npk_init();
  light_init();
  actuator_init();

  espnow_sensor_init(CENTER_MAC, "Ten_WiFi_Nha_Ban");
  espnow_set_actuator_callback(on_remote_command);
}

void loop()
{
  // 1. Lấy dữ liệu từ từng file .h của cảm biến
  float t = dht20_read_temp();
  float h = dht20_read_hum();
  int soil = soil_read_percent();
  int light = light_read_percent();
  int n = npk_read_n();
  int p = npk_read_p();
  int k = npk_read_k();

  // 2. Cập nhật logic tưới & đèn giao thông (tự động nếu đang ở auto_mode)
  PumpLevel current_level = actuator_update(soil);

  // 3. Đóng gói toàn bộ dữ liệu, đếm gói và bắn qua ESP-NOW
  espnow_pack_and_send(t, h, soil, light, n, p, k, current_level);

  vTaskDelay(pdMS_TO_TICKS(2000));
}
