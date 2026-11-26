#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "espnow.h"
#include "oled_display.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Biến lưu trạng thái hiển thị dùng chung cho Task OLED
static struct_telemetry g_display_data = {0};
static uint32_t g_display_rx_count = 0;

// =========================================================================
// 1. HÀM XỬ LÝ LỆNH RPC NHẬN TỪ COREIOT -> CHUYỂN TIẾP QUA ESP-NOW VỀ SLAVE
// =========================================================================
void onCoreIOTRpcCallback(char *topic, byte *payload, unsigned int length)
{
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, payload, length))
  {
    return;
  }

  const char *method = doc["method"] | "";
  PumpLevel target_level = PUMP_OFF_GREEN;
  bool auto_mode = false;
  bool valid_cmd = false;

  // Trường hợp 1: CoreIOT gửi mức bơm (0: Tắt/Xanh, 1: Bơm ít/Vàng, 2: Bơm nhiều/Đỏ)
  if (strcmp(method, "setPumpLevel") == 0)
  {
    int lvl = constrain(doc["params"].as<int>(), 0, 2);
    target_level = (PumpLevel)lvl;
    valid_cmd = true;
  }
  // Trường hợp 2: CoreIOT Switch ON/OFF truyền thống (true -> Bơm nhiều, false -> Tắt)
  else if (strcmp(method, "setPump") == 0)
  {
    bool state = doc["params"].as<bool>();
    target_level = state ? PUMP_HIGH_RED : PUMP_OFF_GREEN;
    valid_cmd = true;
  }
  // Trường hợp 3: Chuyển chế độ Tự động / Thủ công
  else if (strcmp(method, "setAutoMode") == 0)
  {
    auto_mode = doc["params"].as<bool>();
    target_level = g_display_data.pump_level;
    valid_cmd = true;
  }

  if (valid_cmd)
  {
    // Bắn lệnh ngược qua ESP-NOW xuống Node Slave đóng/cắt Relay bơm
    espnow_send_pump_level(target_level, auto_mode);

    // Phản hồi xác nhận RPC về lại CoreIOT (để nút Switch trên Dashboard không bị xoay vòng)
    String topicStr(topic);
    int lastSlash = topicStr.lastIndexOf('/');
    if (lastSlash != -1)
    {
      String reqId = topicStr.substring(lastSlash + 1);
      String respTopic = "v1/devices/me/rpc/response/" + reqId;
      StaticJsonDocument<64> respDoc;
      respDoc["pump_level"] = (int)target_level;
      respDoc["status"] = "OK";
      char respBuf[64];
      serializeJson(respDoc, respBuf);
      mqttClient.publish(respTopic.c_str(), respBuf);
    }
  }
}

// Kết nối / duy trì phiên MQTT với CoreIOT
void ensureCoreIOTConnected()
{
  if (!ENABLE_DIRECT_COREIOT_MQTT || WiFi.status() != WL_CONNECTED)
    return;

  if (!mqttClient.connected())
  {
    String clientId = "YoloUNO_Center_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqttClient.connect(clientId.c_str(), COREIOT_ACCESS_TOKEN, NULL))
    {
      // Đăng ký nhận lệnh điều khiển RPC từ Dashboard / RuleChain CoreIOT
      mqttClient.subscribe("v1/devices/me/rpc/request/+");
    }
  }
}

// Đóng gói JSON gửi lên CoreIOT (MQTT) và PC Gateway (Serial)
void publishTelemetry(const struct_telemetry &d, uint32_t rx_count)
{
  StaticJsonDocument<256> doc;
  doc["packet_id"] = d.packet_id;
  doc["rx_count"] = rx_count;
  doc["temp"] = serialized(String(d.temp, 1));
  doc["hum_air"] = serialized(String(d.hum_air, 1));
  doc["hum_soil"] = d.hum_soil;
  doc["light"] = d.light;
  doc["nitrogen"] = d.nitrogen;
  doc["phosphorus"] = d.phosphorus;
  doc["potassium"] = d.potassium;
  doc["pump_level"] = (int)d.pump_level;
  doc["pump_state"] = (d.pump_level > PUMP_OFF_GREEN);
  doc["led_color"] = (d.pump_level == PUMP_OFF_GREEN) ? "GREEN" : (d.pump_level == PUMP_LOW_YELLOW) ? "YELLOW"
                                                                                                    : "RED";

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  // 1. Đẩy qua Serial cho PC Gateway (gateway/main.py)
  Serial.println(jsonBuffer);

  // 2. Đẩy thẳng lên CoreIOT nếu bật kết nối trực tiếp
  if (ENABLE_DIRECT_COREIOT_MQTT && mqttClient.connected())
  {
    mqttClient.publish("v1/devices/me/telemetry", jsonBuffer);
  }
}

// =========================================================================
// 2. FREERTOS TASK 1: XỬ LÝ COREIOT, SERIAL GATEWAY & CHUYỂN TIẾP ESP-NOW
// =========================================================================
void taskCoreIOTAndGateway(void *pvParameters)
{
  uint32_t lastReconnectAttempt = 0;

  for (;;)
  {
    // Duy trì kết nối CoreIOT MQTT (thử lại mỗi 5 giây nếu mất kết nối)
    if (ENABLE_DIRECT_COREIOT_MQTT && WiFi.status() == WL_CONNECTED)
    {
      if (!mqttClient.connected())
      {
        if (millis() - lastReconnectAttempt > 5000)
        {
          lastReconnectAttempt = millis();
          ensureCoreIOTConnected();
        }
      }
      else
      {
        mqttClient.loop();
      }
    }

    // Kiểm tra nếu có gói telemetry mới từ Node Slave qua ESP-NOW
    struct_telemetry new_pkt;
    uint32_t rx_cnt = 0;
    if (espnow_consume_new_telemetry(&new_pkt, &rx_cnt))
    {
      g_display_data = new_pkt;
      g_display_rx_count = rx_cnt;
      publishTelemetry(new_pkt, rx_cnt);
    }

    // Lắng nghe lệnh JSON từ PC Gateway qua cổng Serial (nếu điều khiển từ PC/Voice)
    if (Serial.available() > 0)
    {
      String cmdLine = Serial.readStringUntil('\n');
      cmdLine.trim();
      if (cmdLine.startsWith("{"))
      {
        StaticJsonDocument<128> cmdDoc;
        if (!deserializeJson(cmdDoc, cmdLine))
        {
          bool auto_cmd = cmdDoc["auto"] | false;
          if (cmdDoc.containsKey("pump_level"))
          {
            int lvl = constrain(cmdDoc["pump_level"].as<int>(), 0, 2);
            espnow_send_pump_level((PumpLevel)lvl, auto_cmd);
          }
          else if (cmdDoc.containsKey("pump"))
          {
            bool on = cmdDoc["pump"].as<bool>();
            espnow_send_pump_level(on ? PUMP_HIGH_RED : PUMP_OFF_GREEN, auto_cmd);
          }
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// =========================================================================
// 3. FREERTOS TASK 2: CẬP NHẬT MÀN HÌNH OLED 0.96 INCH
// =========================================================================
void taskOledDisplay(void *pvParameters)
{
  for (;;)
  {
    bool wifi_ok = (WiFi.status() == WL_CONNECTED);
    bool iot_ok = (ENABLE_DIRECT_COREIOT_MQTT && mqttClient.connected());

    oled_update_telemetry(g_display_data, g_display_rx_count, wifi_ok, iot_ok);

    vTaskDelay(pdMS_TO_TICKS(500)); // Làm tươi màn hình mỗi 0.5 giây
  }
}

// =========================================================================
// 4. HÀM SETUP & LOOP CHÍNH
// =========================================================================
void setup()
{
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(500));

  // In địa chỉ MAC của YoloUNO Center để nạp vào mảng CENTER_MAC bên Node Slave
  WiFi.mode(WIFI_STA);
  String macAddr = WiFi.macAddress();
  Serial.println("=== KHOI DONG NODE CENTER (YOLOUNO ESP32-S3) ===");
  Serial.print("Dia chi MAC YoloUNO Center: ");
  Serial.println(macAddr);

  // Khởi tạo màn hình OLED 0.96 inch
  oled_init(OLED_SDA_PIN, OLED_SCL_PIN);
  oled_show_boot(macAddr);

  // Kết nối Wi-Fi (tối đa 10s để tránh treo mạch nếu thử nghiệm offline)
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t startWifi = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startWifi) < 10000)
  {
    vTaskDelay(pdMS_TO_TICKS(250));
  }

  // Cấu hình MQTT Server cho CoreIOT
  mqttClient.setServer(COREIOT_SERVER, COREIOT_PORT);
  mqttClient.setCallback(onCoreIOTRpcCallback);

  // Khởi tạo ESP-NOW (tự động dùng chung kênh Wi-Fi vừa kết nối)
  if (!espnow_center_init(SLAVE_MAC))
  {
    Serial.println("[LOI] Khong the khoi tao ESP-NOW!");
  }

  // Tạo 2 FreeRTOS Task chạy song song
  xTaskCreate(taskCoreIOTAndGateway, "CoreIOT_Gateway_Task", 6144, NULL, 2, NULL);
  xTaskCreate(taskOledDisplay, "OLED_Display_Task", 4096, NULL, 1, NULL);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}