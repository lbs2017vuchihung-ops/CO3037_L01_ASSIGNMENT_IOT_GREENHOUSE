#include "espnow.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static uint8_t g_center_mac[6];
static uint32_t g_sent_count = 0;
static uint32_t g_success_count = 0;
static uint32_t g_fail_count = 0;
static ActuatorCommandCallback g_actuator_cb = nullptr;

// Callback thống kê gói gửi thành công / thất bại ở tầng MAC
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void on_data_sent(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
#else
static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
#endif
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        g_success_count++;
    }
    else
    {
        g_fail_count++;
    }
}

// Callback nhận lệnh điều khiển mức bơm (0/1/2) từ Node Center / CoreIOT
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void on_data_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
#else
static void on_data_recv(const uint8_t *mac, const uint8_t *data, int len)
{
#endif
    if (len == sizeof(struct_command))
    {
        struct_command cmd;
        memcpy(&cmd, data, sizeof(struct_command));

        const char *label = (cmd.target_level == PUMP_OFF_GREEN) ? "XANH (TAT)" : (cmd.target_level == PUMP_LOW_YELLOW) ? "VANG (BOM IT)"
                                                                                                                        : "DO (BOM NHIEU)";
        Serial.printf("[ESP-NOW RX] Lenh tu Center -> %s | Mode: %s\n",
                      label, cmd.auto_mode ? "AUTO" : "MANUAL");

        if (g_actuator_cb != nullptr)
        {
            g_actuator_cb(cmd.target_level, cmd.set_fan, cmd.auto_mode);
        }
    }
}

// Tìm đúng kênh Wi-Fi mà Node Center đang kết nối
static int32_t scan_router_channel(const char *ssid)
{
    if (ssid == nullptr || strlen(ssid) == 0)
        return 0;
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++)
    {
        if (strcmp(ssid, WiFi.SSID(i).c_str()) == 0)
        {
            return WiFi.channel(i);
        }
    }
    return 0;
}

bool espnow_sensor_init(const uint8_t *center_mac, const char *router_ssid)
{
    memcpy(g_center_mac, center_mac, 6);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    int32_t ch = scan_router_channel(router_ssid);
    if (ch > 0)
    {
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);
        Serial.printf("[ESP-NOW] Dong bo kenh Wi-Fi: %d\n", ch);
    }

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("[ESP-NOW] Loi khoi tao ESP-NOW!");
        return false;
    }

    esp_now_register_send_cb(on_data_sent);
    esp_now_register_recv_cb(on_data_recv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, g_center_mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("[ESP-NOW] Loi them Node Center vao Peer!");
        return false;
    }

    return true;
}

void espnow_set_actuator_callback(ActuatorCommandCallback cb)
{
    g_actuator_cb = cb;
}

bool espnow_pack_and_send(
    float temp, float hum_air, int hum_soil, int light,
    int n, int p, int k, PumpLevel pump_level, bool fan_state)
{
    g_sent_count++; // Mỗi lần gọi hàm gửi là tăng bộ đếm

    struct_telemetry pkt;
    pkt.packet_id = g_sent_count;
    pkt.temp = temp;
    pkt.hum_air = hum_air;
    pkt.hum_soil = hum_soil;
    pkt.light = light;
    pkt.nitrogen = n;
    pkt.phosphorus = p;
    pkt.potassium = k;
    pkt.pump_level = pump_level;
    pkt.fan_state = fan_state;

    esp_err_t res = esp_now_send(g_center_mac, (uint8_t *)&pkt, sizeof(pkt));

    Serial.printf("[ESP-NOW TX #%lu | OK:%lu | Fail:%lu] T:%.1f H:%.1f Soil:%d%% Light:%d%% NPK:%d/%d/%d Level:%d\n",
                  g_sent_count, g_success_count, g_fail_count,
                  temp, hum_air, hum_soil, light, n, p, k, pump_level);

    return (res == ESP_OK);
}

uint32_t espnow_get_sent_count() { return g_sent_count; }
uint32_t espnow_get_success_count() { return g_success_count; }
uint32_t espnow_get_fail_count() { return g_fail_count; }