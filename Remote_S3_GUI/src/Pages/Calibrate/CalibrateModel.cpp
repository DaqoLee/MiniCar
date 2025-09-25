#include "CalibrateModel.h"

using namespace Page;

void CalibrateModel::Init()
{
    account = new Account("CalibrateModel", DataProc::Center(), 0, this);
    account->Subscribe("SportStatus");
    account->Subscribe("Recorder");
    account->Subscribe("StatusBar");
    account->Subscribe("GPS");
    account->Subscribe("MusicPlayer");
    account->Subscribe("Joystick");
    account->Subscribe("Remote");
    account->Subscribe("Calibrate");
    account->SetEventCallback(onEvent);

    SetRemoteMode(DataProc::OperationMode_t::MODE_CALIBRATE,DataProc::CalibrateMode_t::READ_MID);
}

void CalibrateModel::Deinit()
{
    if (account)
    {
        delete account;
        account = nullptr;
    }
}

bool CalibrateModel::GetGPSReady()
{
    HAL::GPS_Info_t gps;
    if(account->Pull("GPS", &gps, sizeof(gps)) != Account::RES_OK)
    {
        return false;
    }
    return (gps.satellites > 0);
}

void CalibrateModel::GetCalibrateInfo(uint16_t data[12])
{

    HAL::Joystick_Calibrate_t Calibrate_info[2] = { 0 };

    account->Pull("Calibrate", Calibrate_info, sizeof(Calibrate_info));

    data[0] = Calibrate_info[0].max_x;
    data[1] = Calibrate_info[0].max_y;
    data[2] = Calibrate_info[1].max_x;
    data[3] = Calibrate_info[1].max_y;
    data[4] = Calibrate_info[0].mid_x;
    data[5] = Calibrate_info[0].mid_y;

    data[6] = Calibrate_info[1].mid_x;
    data[7] = Calibrate_info[1].mid_y;
    data[8] = Calibrate_info[0].min_x;
    data[9] = Calibrate_info[0].min_y;
    data[10] = Calibrate_info[1].min_x;
    data[11] = Calibrate_info[1].min_y;

}

int CalibrateModel::onEvent(Account* account, Account::EventParam_t* param)
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

    CalibrateModel* instance = (CalibrateModel*)account->UserData;
    memcpy(&(instance->sportStatusInfo), param->data_p, param->size);

    return Account::RES_OK;
}

void CalibrateModel::RecorderCommand(RecCmd_t cmd)
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

void CalibrateModel::PlayMusic(const char* music)
{
    DataProc::MusicPlayer_Info_t info;
    DATA_PROC_INIT_STRUCT(info);

    info.music = music;
    account->Notify("MusicPlayer", &info, sizeof(info));
}

void CalibrateModel::SetStatusBarStyle(DataProc::StatusBar_Style_t style)
{
    DataProc::StatusBar_Info_t info;
    DATA_PROC_INIT_STRUCT(info);

    info.cmd = DataProc::STATUS_BAR_CMD_SET_STYLE;
    info.param.style = style;

    account->Notify("StatusBar", &info, sizeof(info));
}

void CalibrateModel::SetRemoteMode(DataProc::OperationMode_t mode, DataProc::CalibrateMode_t step )
{
    DataProc::Remote_Info_t info;
    DATA_PROC_INIT_STRUCT(info);

    info.mode = mode;
    info.step = step;
    account->Notify("Remote", &info, sizeof(info));
}
