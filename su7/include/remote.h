#pragma once
#include <Arduino.h>
#include <esp_now.h>

// ESP-NOW 数据结构


typedef struct struct_message {
  int16_t joy1X;
  int16_t joy1Y;
  int16_t joy2X;
  int16_t joy2Y;
} struct_message;

typedef struct struct_ack {
  uint8_t mode; // 确认的消息ID
  uint8_t battery;
} struct_ack;



extern struct_message rxData;
extern struct_ack ackData;
void initJoystickMode();