#ifndef __DEVICE_MODEL_H
#define __DEVICE_MODEL_H

#include "Common/DataProc/DataProc.h"

namespace Page
{

class DeviceModel
{
public:
    void Init();
    void Deinit();

    void GetDeviceList(DataProc::DeviceList_Info_t* info);
    void SwitchDevice(uint8_t index);
    void EnterPairingMode();
    void SetStatusBarStyle(DataProc::StatusBar_Style_t style);
    
private:
    Account* account;
};

}

#endif
