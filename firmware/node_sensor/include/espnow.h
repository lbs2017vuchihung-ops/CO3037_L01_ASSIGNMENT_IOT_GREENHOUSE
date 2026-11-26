#ifndef NODE_SENSOR_ESPNOW_H
#define NODE_SENSOR_ESPNOW_H

#include <Arduino.h>

// Định nghĩa 3 mức tưới đồng bộ giữa Bơm và Đèn giao thông
enum PumpLevel : uint8_t
{
    PUMP_OFF_GREEN = 0,  // Đèn Xanh: Không bơm
    PUMP_LOW_YELLOW = 1, // Đèn Vàng: Bơm ít
    PUMP_HIGH_RED = 2    // Đèn Đỏ:   Bơm nhiều nước
};

// Gói tin dữ liệu gửi đi: Slave -> Center
typedef struct __attribute__((packed))
{
    uint32_t packet_id;   // Số thứ tự gói tin đã gửi (1, 2, 3...)
    float temp;           // Nhiệt độ không khí - DHT20 (°C)
    float hum_air;        // Độ ẩm không khí - DHT20 (%)
    int hum_soil;         // Độ ẩm đất - Cảm biến 2 càng (%)
    int light;            // Cường độ ánh sáng (%)
    int nitrogen;         // Chỉ số Nitơ (N) - RS485 (mg/kg)
    int phosphorus;       // Chỉ số Phốt pho (P) - RS485 (mg/kg)
    int potassium;        // Chỉ số Kali (K) - RS485 (mg/kg)
    PumpLevel pump_level; // Mức bơm & màu đèn giao thông hiện tại (0, 1, 2)
} struct_telemetry;

// Gói tin lệnh nhận về: Center / CoreIOT -> Slave
typedef struct __attribute__((packed))
{
    PumpLevel target_level; // Mức bơm yêu cầu (0: Xanh/Tắt, 1: Vàng/Ít, 2: Đỏ/Nhiều)
    bool auto_mode;         // true: Tự động theo ngưỡng, false: Điều khiển tay
} struct_command;

// Callback gọi sang actuator.h khi nhận lệnh từ Node Center
typedef void (*ActuatorCommandCallback)(PumpLevel target_level, bool auto_mode);

// Khởi tạo ESP-NOW và đồng bộ kênh Wi-Fi với Router của Node Center
bool espnow_sensor_init(const uint8_t *center_mac, const char *router_ssid = nullptr);

// Đăng ký hàm xử lý bên actuator.cpp để điều khiển Relay + Đèn giao thông
void espnow_set_actuator_callback(ActuatorCommandCallback cb);

// Nhận data từ các file .h của sensor, đóng gói, tăng biến đếm và gửi sang Center
bool espnow_pack_and_send(
    float temp, float hum_air, int hum_soil, int light,
    int n, int p, int k, PumpLevel pump_level);

// Lấy thống kê số gói tin (dùng hiển thị LCD tại chỗ & đo đạc Chương 4)
uint32_t espnow_get_sent_count();
uint32_t espnow_get_success_count();
uint32_t espnow_get_fail_count();

#endif