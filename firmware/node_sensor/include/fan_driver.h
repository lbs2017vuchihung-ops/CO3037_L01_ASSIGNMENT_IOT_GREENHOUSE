#pragma once
#include <Arduino.h>

// Khởi tạo chân GPIO điều khiển Relay quạt (mặc định TẮT)
void fan_init();

// Bật/tắt quạt trực tiếp (dùng khi nhận lệnh từ Center / CoreIOT / Web)
void fan_set_state(bool turn_on);

// Chuyển đổi chế độ Tự động / Thủ công cho quạt
void fan_set_auto_mode(bool auto_mode);

// Cập nhật trạng thái quạt theo nhiệt độ (chỉ tự bật/tắt nếu đang ở auto_mode)
bool fan_update(float current_temp);

// Lấy trạng thái thực tế hiện tại của quạt (true: BẬT, false: TẮT)
bool fan_get_state();