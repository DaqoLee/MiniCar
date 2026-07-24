#include "HAL.h"
#include <stdlib.h>
#include "Common/DataProc/DataProc.h"
#include "Arduino.h"
#include "esp_adc_cal.h"

// #define JOY1_X 4
// #define JOY1_Y 5
// #define JOY2_X 2
// #define JOY2_Y 1

// #define MAX_JOY1_X 3400
// #define MID_JOY1_X 1960
// #define MIN_JOY1_X 630

// #define MAX_JOY2_X 3050
// #define MID_JOY2_X 1565
// #define MIN_JOY2_X 100

// #define MAX_JOY1_Y 2960
// #define MID_JOY1_Y 1650
// #define MIN_JOY1_Y 350

// #define MAX_JOY2_Y 3420
// #define MID_JOY2_Y 2070
// #define MIN_JOY2_Y 680


// #define JOY_MAX 100
// #define JOY_MINI 0

// #define MAX_VALUE 100
// #define MIN_VALUE 0

// HAL::Joystick_Info_t joystick;

// static HAL::CommitFunc_t CommitFunc;
// static void* UserData;
// void HAL::Joystick_Init()
// {
//   pinMode(JOY1_X, INPUT);
//   pinMode(JOY1_Y, INPUT);
//   pinMode(JOY2_X, INPUT);
//   pinMode(JOY2_Y, INPUT);

//   analogReadResolution(12);
// }
// void HAL::Joystick_SetCommitCallback(CommitFunc_t func, void* userData)
// {
//     CommitFunc = func;
//     UserData = userData;
// }

// void HAL::Joystick_Update()
// {
//     static int16_t steps;
//     steps++;
//     if (steps > 9999)
//     {
//         steps = 0;
//     }



//     uint16_t joy1Y = analogRead(JOY1_Y);
//     uint16_t joy2Y = analogRead(JOY2_Y);
//     uint16_t joy1X = analogRead(JOY1_X);
//     uint16_t joy2X = analogRead(JOY2_X);

//     joystick.x1 = map(joy1X, MIN_JOY1_X, MAX_JOY1_X, 0, 4095);
//     joystick.y1 = map(joy1Y, MIN_JOY1_Y, MAX_JOY1_Y, 4095, 0);
//     joystick.x2 = map(joy2X, MIN_JOY2_X, MAX_JOY2_X, 4095, 0);
//     joystick.y2 = map(joy2Y, MIN_JOY2_Y, MAX_JOY2_Y, 4095, 0);

//     // joystick.x1 = analogRead(JOY1_Y);
//     // joystick.y1 = analogRead(JOY2_Y);
//     // joystick.x2 = analogRead(JOY1_X);
//     // joystick.y2 = analogRead(JOY2_X);


//     if (CommitFunc)
//     {
//         CommitFunc(&joystick, UserData);
//     }
// }

