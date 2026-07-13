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

DATA_PROC_INIT_DEF(CarPower)
{
    HAL::CarPower_SetCommitCallback([](void* info, void* userData){
        Account* account = (Account*)userData;
        return account->Commit(info, sizeof(uint8_t));
    }, account);
}

DATA_PROC_INIT_DEF(Connect)
{
    HAL::Connect_SetCommitCallback([](void* info, void* userData){
        Account* account = (Account*)userData;
        return account->Commit(info, sizeof(int16_t));
    }, account);
}

DATA_PROC_INIT_DEF(DeviceList)
{
    HAL::DeviceList_SetCommitCallback([](void* info, void* userData){
        Account* account = (Account*)userData;
        DataProc::DeviceList_Info_t devInfo;
        DATA_PROC_INIT_STRUCT(devInfo);
        
        devInfo.count = HAL::Remote_GetDeviceCount();
        devInfo.currentIndex = HAL::Remote_GetCurrentDeviceIndex();
        
        for (int i = 0; i < devInfo.count && i < MAX_DEVICE_LIST_COUNT; i++)
        {
            char macStr[18] = {0};
            HAL::Remote_GetDeviceInfo(
                i,
                devInfo.devices[i].name,
                sizeof(devInfo.devices[i].name),
                macStr, sizeof(macStr)
            );
            HAL::Remote_GetDeviceMAC(i, devInfo.devices[i].mac);
        }
        
        return account->Commit(&devInfo, sizeof(devInfo));
    }, account);

    // 初始化完成后立即推一次设备列表
    // (HAL::Remote_Init 在 DataProc_Init 之前运行，当时 callback 尚未注册)
    {
        DataProc::DeviceList_Info_t devInfo;
        DATA_PROC_INIT_STRUCT(devInfo);
        devInfo.count = HAL::Remote_GetDeviceCount();
        devInfo.currentIndex = HAL::Remote_GetCurrentDeviceIndex();
        for (int i = 0; i < devInfo.count && i < MAX_DEVICE_LIST_COUNT; i++)
        {
            char macStr[18] = {0};
            HAL::Remote_GetDeviceInfo(i, devInfo.devices[i].name,
                sizeof(devInfo.devices[i].name), macStr, sizeof(macStr));
            HAL::Remote_GetDeviceMAC(i, devInfo.devices[i].mac);
        }
        account->Commit(&devInfo, sizeof(devInfo));
    }
}

static int onDeviceSwitchEvent(Account* account, Account::EventParam_t* param)
{
    if (param->event != Account::EVENT_NOTIFY)
    {
        return Account::RES_UNSUPPORTED_REQUEST;
    }
    if (param->size != sizeof(uint8_t))
    {
        return Account::RES_SIZE_MISMATCH;
    }
    uint8_t index = *(uint8_t*)param->data_p;
    HAL::Remote_SwitchDevice(index);
    return Account::RES_OK;
}

DATA_PROC_INIT_DEF(DeviceSwitch)
{
    account->SetEventCallback(onDeviceSwitchEvent);
}

