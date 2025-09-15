#ifndef _MAIN_H
#define _MAIN_H
#include "SimpleSerialShell.h"


#define BL 38
#define PWOR_PIN 44

#define KEY_R1_PIN 45
#define KEY_R2_PIN 46
#define KEY_R3_PIN 0

#define BATTERY_PIN 10
#define RGB_PIN 15


void GuiTask(void *pvParameters);
#endif // !_MAIN_H
