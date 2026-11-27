#ifndef NODE_CENTER_ESPNOW_H
#define NODE_CENTER_ESPNOW_H

#include <Arduino.h>

enum PumpLevel : uint8_t
{
    PUMP_OFF_GREEN = 0,  // Đèn Xanh: Không bơm
    PUMP_LOW_YELLOW = 1, // Đèn Vàng: Bơm ít
    PUMP_HIGH_RED = 2    // Đèn Đỏ:   Bơm nhiều nước
};

typedef struct __attribute__((packed))
{
    uint32_t packet_id;
    float temp;
    float hum_air;
    int hum_soil;
    int light;
    int nitrogen;
    int phosphorus;
    int potassium;
    PumpLevel pump_level;
    bool fan_state;
} struct_telemetry;

typedef struct __attribute__((packed))
{
    PumpLevel target_level;
    bool set_fan;
    bool auto_mode;
} struct_command;

bool espnow_center_init(const uint8_t *slave_mac);
bool espnow_send_pump_level(PumpLevel level, bool auto_mode = false);

// Kiểm tra và lấy gói tin mới nhất nếu vừa nhận được từ Node Slave
bool espnow_consume_new_telemetry(struct_telemetry *out_data, uint32_t *out_rx_count);
struct_telemetry espnow_get_latest_telemetry();

#endif