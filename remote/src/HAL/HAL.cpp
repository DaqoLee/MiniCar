#include "HAL.h"

void HAL::HAL_Init()
{

    Power_Init();
    Joystick_Init();
    Remote_Init();
}

void HAL::HAL_Update()
{
    // IMU_Update();
    // MAG_Update();
    // Audio_Update();
    Power_Update();
    // Joystick_Update();
    // Remote_Update();
}
