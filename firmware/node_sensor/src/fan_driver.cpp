#include "fan_driver.h"
#include "config.h"

static bool g_fan_state = false;
static bool g_fan_auto = false; // Mặc định false để nhường quyền tự động cho CoreIOT RuleChain

void fan_init()
{
    pinMode(FAN_RELAY_PIN, OUTPUT);
    digitalWrite(FAN_RELAY_PIN, LOW);
    g_fan_state = false;
}

void fan_set_state(bool turn_on)
{
    g_fan_state = turn_on;
    digitalWrite(FAN_RELAY_PIN, g_fan_state ? HIGH : LOW);
}

void fan_set_auto_mode(bool auto_mode)
{
    g_fan_auto = auto_mode;
}

bool fan_update(float current_temp)
{
    if (g_fan_auto && !isnan(current_temp))
    {
        if (current_temp > TEMP_THRESHOLD_HIGH)
        {
            fan_set_state(true);
        }
        else if (current_temp <= (TEMP_THRESHOLD_HIGH - 1.0))
        {
            // Trễ 1°C (hysteresis) để tránh Relay đóng/ngắt liên tục quanh 35°C
            fan_set_state(false);
        }
    }
    return g_fan_state;
}

bool fan_get_state()
{
    return g_fan_state;
}