#include "DeviceModel.h"
#include <Arduino.h>
#include "Common/HAL/HAL.h"
#include <stdio.h>

using namespace Page;

void DeviceModel::Init()
{
    account = new Account("DeviceModel", DataProc::Center(), 0, this);
    account->Subscribe("DeviceList");
    account->Subscribe("StatusBar");
    account->Subscribe("Remote");
}

void DeviceModel::Deinit()
{
    if (account)
    {
        delete account;
        account = nullptr;
    }
}

void DeviceModel::GetDeviceList(DataProc::DeviceList_Info_t* info)
{
    // Try DataCenter first, fall back to direct HAL call if Pull fails
    if (account->Pull("DeviceList", info, sizeof(DataProc::DeviceList_Info_t)) != Account::RES_OK) {
        // Direct HAL fallback
        info->count = HAL::Remote_GetDeviceCount();
        Serial.printf("[DeviceModel] Pull failed, fallback HAL count=%d\n", info->count);
        info->currentIndex = HAL::Remote_GetCurrentDeviceIndex();
        for (int i = 0; i < info->count && i < MAX_DEVICE_LIST_COUNT; i++) {
            char macStr[18] = {0};
            HAL::Remote_GetDeviceInfo(i, info->devices[i].name,
                sizeof(info->devices[i].name), macStr, sizeof(macStr));
            HAL::Remote_GetDeviceMAC(i, info->devices[i].mac);
        }
    }
}

void DeviceModel::SwitchDevice(uint8_t index)
{
    Serial.printf("[DeviceModel] SwitchDevice: index=%d\n", index);
    HAL::Remote_SwitchDevice(index);
}

void DeviceModel::EnterPairingMode()
{
    DataProc::Remote_Info_t info;
    DATA_PROC_INIT_STRUCT(info);
    info.mode = DataProc::OperationMode_t::MODE_PAIR;
    account->Notify("Remote", &info, sizeof(info));
}

void DeviceModel::SetStatusBarStyle(DataProc::StatusBar_Style_t style)
{
    DataProc::StatusBar_Info_t info;
    DATA_PROC_INIT_STRUCT(info);
    info.cmd = DataProc::STATUS_BAR_CMD_SET_STYLE;
    info.param.style = style;
    account->Notify("StatusBar", &info, sizeof(info));
}
