#ifndef __PAIR_MODEL_H
#define __PAIR_MODEL_H

#include "Common/DataProc/DataProc.h"

namespace Page
{

class PairModel
{
public:
    typedef enum
    {
        REC_START    = DataProc::RECORDER_CMD_START,
        REC_PAUSE    = DataProc::RECORDER_CMD_PAUSE,
        REC_CONTINUE = DataProc::RECORDER_CMD_CONTINUE,
        REC_STOP     = DataProc::RECORDER_CMD_STOP,
        REC_READY_STOP
    } RecCmd_t;

public:
    HAL::SportStatus_Info_t sportStatusInfo;

public:
    void Init();
    void Deinit();

    bool GetGPSReady();
    void GetDeviceInfo(char name[32]);
    float GetSpeed()
    {
        return sportStatusInfo.speedKph;
    }

    float GetAvgSpeed()
    {
        return sportStatusInfo.speedAvgKph;
    }

    void RecorderCommand(RecCmd_t cmd);
    void PlayMusic(const char* music);
    void SetStatusBarStyle(DataProc::StatusBar_Style_t style);
    void SetRemoteMode(DataProc::OperationMode_t mode);
private:
    Account* account;

private:
    static int onEvent(Account* account, Account::EventParam_t* param);
};

}

#endif
