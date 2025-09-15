#include "DataProc.h"
#include "../HAL/HAL.h"

static DataCenter center("CENTER");

DataCenter* DataProc::Center()
{
    return &center;
}

void DataProc_Init()
{
#define DP_DEF(NODE_NAME, BUFFER_SIZE)\
    Account* act##NODE_NAME = new Account(#NODE_NAME, &center, BUFFER_SIZE);
//  #  include "DP_LIST.inc"

DP_DEF(Storage, 0);
DP_DEF(Clock, 0);
DP_DEF(GPS, sizeof(HAL::GPS_Info_t));
DP_DEF(Power, 0);
DP_DEF(SportStatus, sizeof(HAL::SportStatus_Info_t));
DP_DEF(Recorder, 0);
DP_DEF(IMU, sizeof(HAL::IMU_Info_t));
DP_DEF(MAG, sizeof(HAL::MAG_Info_t));
DP_DEF(StatusBar, 0);
DP_DEF(MusicPlayer, 0);
DP_DEF(TzConv, 0);
DP_DEF(SysConfig, 0);
DP_DEF(TrackFilter, 0);
#undef DP_DEF

#define DP_DEF(NODE_NAME, BUFFER_SIZE)\
do{\
    DATA_PROC_INIT_DEF(NODE_NAME);\
    _DP_##NODE_NAME##_Init(act##NODE_NAME);\
}while(0)
// #  include "DP_LIST.inc"
// DP_DEF(Storage, 0);
// DP_DEF(Clock, 0);
// DP_DEF(GPS, sizeof(HAL::GPS_Info_t));
// DP_DEF(Power, 0);
// DP_DEF(SportStatus, sizeof(HAL::SportStatus_Info_t));
// DP_DEF(Recorder, 0);
// DP_DEF(IMU, sizeof(HAL::IMU_Info_t));
// DP_DEF(MAG, sizeof(HAL::MAG_Info_t));
// DP_DEF(StatusBar, 0);
// DP_DEF(MusicPlayer, 0);
// DP_DEF(TzConv, 0);
// DP_DEF(SysConfig, 0);
// DP_DEF(TrackFilter, 0);
#undef DP_DEF

}
