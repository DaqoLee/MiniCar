#include "DataProc.h"
#include "../HAL/HAL.h"

static int onEvent(Account* account, Account::EventParam_t* param)
{
    if (param->event != Account::EVENT_NOTIFY)
    {
        return Account::RES_UNSUPPORTED_REQUEST;
    }

    if (param->size != sizeof(DataProc::Remote_Info_t))
    {
        return Account::RES_SIZE_MISMATCH;
    }

    DataProc::Remote_Info_t* info = (DataProc::Remote_Info_t*)param->data_p;

    HAL::Remote_SetMode((HAL::OperationMode_t)info->mode);
    HAL::Remote_SetCalibrateStep((HAL::CalibrateMode_t)info->step);
    return Account::RES_OK;
}




DATA_PROC_INIT_DEF(Remote)
{
    account->SetEventCallback(onEvent);
}

DATA_PROC_INIT_DEF(Joystick)
{
    HAL::Joystick_SetCommitCallback([](void* info, void* userData){
        Account* account = (Account*)userData;
        return account->Commit(info, sizeof(HAL::Joystick_Info_t));
    }, account);
}

DATA_PROC_INIT_DEF(Pair)
{
    HAL::Pair_SetCommitCallback([](void* info, void* userData){
        Account* account = (Account*)userData;
        return account->Commit(info, sizeof(HAL::device_info_t));
    }, account);
}

DATA_PROC_INIT_DEF(Calibrate)
{
    HAL::Calibrate_SetCommitCallback([](void* info, void* userData){
        Account* account = (Account*)userData;
        return account->Commit(info, sizeof(HAL::Joystick_Calibrate_t)*2);
    }, account);
}

