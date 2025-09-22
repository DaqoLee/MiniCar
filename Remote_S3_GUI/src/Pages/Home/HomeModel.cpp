#include "HomeModel.h"

using namespace Page;
 static uint8_t Mac[] = {0x0C, 0x4E, 0xA0, 0x21, 0x29, 0x3C};
void HomeModel::Init()
{
    account = new Account("HomeModel", DataProc::Center(), 0, this);
    account->Subscribe("SportStatus");
    account->Subscribe("Recorder");
    account->Subscribe("StatusBar");
    account->Subscribe("GPS");
    account->Subscribe("MusicPlayer");
    account->Subscribe("Joystick");
    account->Subscribe("Remote");
    account->SetEventCallback(onEvent);

    SetRemoteMode(DataProc::OperationMode_t::MODE_JOYSTICK);

}

void HomeModel::Deinit()
{
    if (account)
    {
        delete account;
        account = nullptr;
    }
}

bool HomeModel::GetGPSReady()
{
    HAL::GPS_Info_t gps;
    if(account->Pull("GPS", &gps, sizeof(gps)) != Account::RES_OK)
    {
        return false;
    }
    return (gps.satellites > 0);
}

void HomeModel::GetJoystickInfo(uint16_t data[4])
{
  
    HAL::Joystick_Info_t joystick = { 0 };

    account->Pull("Joystick", &joystick, sizeof(joystick));
    data[0] = joystick.x1;
    data[1] = joystick.y1;
    data[2] = joystick.x2;
    data[3] = joystick.y2;
  
}

int HomeModel::onEvent(Account* account, Account::EventParam_t* param)
{
    if (param->event != Account::EVENT_PUB_PUBLISH)
    {
        return Account::RES_UNSUPPORTED_REQUEST;
    }

    if (strcmp(param->tran->ID, "SportStatus") != 0
            || param->size != sizeof(HAL::SportStatus_Info_t))
    {
        return Account::RES_PARAM_ERROR;
    }

    HomeModel* instance = (HomeModel*)account->UserData;
    memcpy(&(instance->sportStatusInfo), param->data_p, param->size);

    return Account::RES_OK;
}

void HomeModel::RecorderCommand(RecCmd_t cmd)
{
    if (cmd != REC_READY_STOP)
    {
        DataProc::Recorder_Info_t recInfo;
        DATA_PROC_INIT_STRUCT(recInfo);
        recInfo.cmd = (DataProc::Recorder_Cmd_t)cmd;
        recInfo.time = 1000;
        account->Notify("Recorder", &recInfo, sizeof(recInfo));
    }

    DataProc::StatusBar_Info_t statInfo;
    DATA_PROC_INIT_STRUCT(statInfo);
    statInfo.cmd = DataProc::STATUS_BAR_CMD_SET_LABEL_REC;

    switch (cmd)
    {
    case REC_START:
    case REC_CONTINUE:
        statInfo.param.labelRec.show = true;
        statInfo.param.labelRec.str = "REC";
        break;
    case REC_PAUSE:
        statInfo.param.labelRec.show = true;
        statInfo.param.labelRec.str = "PAUSE";
        break;  
    case REC_READY_STOP:
        statInfo.param.labelRec.show = true;
        statInfo.param.labelRec.str = "STOP";
        break;
    case REC_STOP:
        statInfo.param.labelRec.show = false;
        break;
    default:
        break;
    }

    account->Notify("StatusBar", &statInfo, sizeof(statInfo));
}

void HomeModel::PlayMusic(const char* music)
{
    DataProc::MusicPlayer_Info_t info;
    DATA_PROC_INIT_STRUCT(info);

    info.music = music;
    account->Notify("MusicPlayer", &info, sizeof(info));
}

void HomeModel::SetStatusBarStyle(DataProc::StatusBar_Style_t style)
{
    DataProc::StatusBar_Info_t info;
    DATA_PROC_INIT_STRUCT(info);

    info.cmd = DataProc::STATUS_BAR_CMD_SET_STYLE;
    info.param.style = style;

    account->Notify("StatusBar", &info, sizeof(info));
}
void HomeModel::SetRemoteMode(DataProc::OperationMode_t mode)
{
    DataProc::Remote_Info_t info;
    DATA_PROC_INIT_STRUCT(info);

    info.mode = mode;
   
    account->Notify("Remote", &info, sizeof(info));
}
