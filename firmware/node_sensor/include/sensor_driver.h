#pragma once
#include <Arduino.h>
#include <ModbusMaster.h>
#include <Wire.h>
#include "DHT20.h"
#include "config.h"

static ModbusMaster rs485Node;
static DHT20 dht20;

void preTransmission() {
  digitalWrite(MAX485_DE_RE_PIN, HIGH); // Bật chế độ Phát (TX)
}

void postTransmission() {
  digitalWrite(MAX485_DE_RE_PIN, LOW);  // Bật chế độ Thu (RX)
}

void initSensors() {
  pinMode(MAX485_DE_RE_PIN, OUTPUT);
  digitalWrite(MAX485_DE_RE_PIN, LOW);

  Serial2.begin(RS485_BAUD_RATE, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  rs485Node.begin(RS485_SLAVE_ID, Serial2);
  rs485Node.preTransmission(preTransmission);
  rs485Node.postTransmission(postTransmission);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  dht20.begin();
}

bool readSoilRS485(float &moisture, float &temp) {
  // Đọc 2 thanh ghi từ 0x0000 (0x0000: Độ ẩm đất, 0x0001: Nhiệt độ đất)
  uint8_t result = rs485Node.readHoldingRegisters(0x0000, 2);
  if (result == rs485Node.ku8MBSuccess) {
    moisture = rs485Node.getResponseBuffer(0) / 10.0f;
    temp     = ((int16_t)rs485Node.getResponseBuffer(1)) / 10.0f;
    return true;
  }
  Serial.printf("[RS485 ERR] Ma loi Modbus: 0x%02X\n", result);
  return false;
}

bool readAirDHT20(float &humidity, float &temp) {
  int status = dht20.read();
  if (status == DHT20_OK) {
    humidity = dht20.getHumidity();
    temp     = dht20.getTemperature();
    return true;
  }
  Serial.printf("[DHT20 ERR] Loi doc cam bien: %d\n", status);
  return false;
}