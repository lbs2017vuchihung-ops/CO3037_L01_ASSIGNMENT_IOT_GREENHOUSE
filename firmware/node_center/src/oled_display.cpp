#include "oled_display.h"
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDR 0x3C

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static bool g_oled_ready = false;

bool oled_init(int sda_pin, int scl_pin)
{
    Wire.begin(sda_pin, scl_pin);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR))
    {
        g_oled_ready = false;
        return false;
    }
    g_oled_ready = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    return true;
}

void oled_show_boot(const String &mac_addr)
{
    if (!g_oled_ready)
        return;
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("FARMLAB NODE CENTER");
    display.println("---------------------");
    display.println("MAC Address:");
    display.println(mac_addr);
    display.println("");
    display.println("Dang ket noi WiFi...");
    display.display();
}

void oled_update_telemetry(
    const struct_telemetry &data,
    uint32_t rx_count,
    bool wifi_connected,
    bool coreiot_connected)
{
    if (!g_oled_ready)
        return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Dòng 1: Địa chỉ IP LAN (cho Webserver T07) + trạng thái CoreIOT
    display.setCursor(0, 0);
    if (wifi_connected)
    {
        display.print(WiFi.localIP().toString());
        display.println(coreiot_connected ? " [IOT]" : " [LAN]");
    }
    else
    {
        display.println("WiFi: Offline");
    }

    // Dòng 2: Thống kê gói tin ESP-NOW (Gửi từ Slave / Nhận tại Center)
    display.setCursor(0, 11);
    display.printf("PKT Gui:%lu Nhan:%lu", data.packet_id, rx_count);

    // Dòng 3: Nhiệt độ & Độ ẩm không khí (DHT20)
    display.setCursor(0, 22);
    display.printf("T:%.1fC   H:%.1f%%", data.temp, data.hum_air);

    // Dòng 4: Độ ẩm đất 2 càng & Ánh sáng
    display.setCursor(0, 33);
    display.printf("Dat:%d%%   AS:%d%%", data.hum_soil, data.light);

    // Dòng 5: Chỉ số NPK (RS485)
    display.setCursor(0, 44);
    display.printf("NPK: %d/%d/%d", data.nitrogen, data.phosphorus, data.potassium);

    // Dòng 6: Trạng thái máy bơm & màu đèn giao thông tương ứng
    const char *pump_str = (data.pump_level == PUMP_OFF_GREEN) ? "BOM: TAT [DEN XANH]" : (data.pump_level == PUMP_LOW_YELLOW) ? "BOM: IT  [DEN VANG]"
                                                                                                                              : "BOM: NHIEU [DEN DO]";
    display.setCursor(0, 55);
    display.print(pump_str);

    display.display();
}