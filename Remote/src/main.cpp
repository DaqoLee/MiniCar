#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

// 摇杆引脚定义
#define JOY1_X 35
#define JOY1_Y 34
#define JOY2_X 32
#define JOY2_Y 36

// 接收端MAC地址（替换为小车实际的MAC地址）
// uint8_t receiverMac[] = {0xDC, 0x06, 0x75, 0xA9, 0x93, 0x20};
uint8_t receiverMac[] = {0x0C, 0x4E, 0xA0, 0x21, 0x29, 0x3C};

// 数据结构（必须与接收端一致）
typedef struct struct_message {
  int16_t joy1X;
  int16_t joy1Y;
  int16_t joy2X;
  int16_t joy2Y;
} struct_message;
struct_message txData;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // 初始化ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW初始化失败");
    return;
  }

  // 注册对等设备
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("添加对等设备失败");
   // return;
  }

  WiFi.setSleep(false);

  Serial.print("ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());

  // 配置摇杆引脚
  pinMode(JOY1_X, INPUT);
  pinMode(JOY1_Y, INPUT);
  pinMode(JOY2_X, INPUT);
  pinMode(JOY2_Y, INPUT);
}

void loop() {
  // 读取摇杆值（12位ADC，0-4095）
  txData.joy1Y = analogRead(JOY1_X);
  txData.joy2Y = analogRead(JOY1_Y);
  txData.joy1X = analogRead(JOY2_X);
  txData.joy2X = analogRead(JOY2_Y);

  // 发送数据
  esp_err_t result = esp_now_send(receiverMac, (uint8_t*)&txData, sizeof(txData));
  
  if (result == ESP_OK) {
    Serial.println("数据发送成功");
  } else {
    Serial.println("数据发送失败");
  }

  // Serial.println(txData.joy1X);
  // Serial.println(txData.joy1Y);
  // Serial.println(txData.joy2X);
  // Serial.println(txData.joy2Y);
  delay(20);  // 控制发送频率（约50Hz）
}