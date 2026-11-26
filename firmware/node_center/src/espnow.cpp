#include "espnow.h"
#include <esp_now.h>
#include <WiFi.h>

static uint8_t g_slave_mac[6];
static struct_telemetry g_latest = {0};
static volatile bool g_has_new_packet = false;
static uint32_t g_rx_count = 0;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void on_data_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
#else
static void on_data_recv(const uint8_t *mac, const uint8_t *data, int len)
{
#endif
    if (len == sizeof(struct_telemetry))
    {
        memcpy(&g_latest, data, sizeof(struct_telemetry));
        g_rx_count++;
        g_has_new_packet = true;
    }
}

bool espnow_center_init(const uint8_t *slave_mac)
{
    memcpy(g_slave_mac, slave_mac, 6);

    if (esp_now_init() != ESP_OK)
    {
        return false;
    }

    esp_now_register_recv_cb(on_data_recv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, g_slave_mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    return (esp_now_add_peer(&peerInfo) == ESP_OK);
}

bool espnow_send_pump_level(PumpLevel level, bool auto_mode)
{
    struct_command cmd;
    cmd.target_level = level;
    cmd.auto_mode = auto_mode;
    return (esp_now_send(g_slave_mac, (uint8_t *)&cmd, sizeof(cmd)) == ESP_OK);
}

bool espnow_consume_new_telemetry(struct_telemetry *out_data, uint32_t *out_rx_count)
{
    if (!g_has_new_packet)
        return false;
    g_has_new_packet = false;
    if (out_data)
        *out_data = g_latest;
    if (out_rx_count)
        *out_rx_count = g_rx_count;
    return true;
}

struct_telemetry espnow_get_latest_telemetry()
{
    return g_latest;
}