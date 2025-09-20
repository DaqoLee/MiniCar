#include "DataProc.h"
#include "../HAL/HAL.h"

DATA_PROC_INIT_DEF(Joystick)
{
    HAL::Joystick_SetCommitCallback([](void* info, void* userData){
        Account* account = (Account*)userData;
        return account->Commit(info, sizeof(HAL::Joystick_Info_t));
    }, account);
}
