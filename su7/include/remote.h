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

extern struct_message rxData;

void initJoystickMode();