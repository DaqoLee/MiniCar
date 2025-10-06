
/* Data processing node list */
DP_DEF(Storage, 0);
DP_DEF(Clock, 0);
DP_DEF(GPS, sizeof(HAL::GPS_Info_t));
DP_DEF(Power, 0);
DP_DEF(SportStatus, sizeof(HAL::SportStatus_Info_t));
DP_DEF(Recorder, 0);
DP_DEF(IMU, sizeof(HAL::IMU_Info_t));
DP_DEF(Joystick, sizeof(HAL::Joystick_Info_t));
DP_DEF(MAG, sizeof(HAL::MAG_Info_t));
DP_DEF(StatusBar, 0);
DP_DEF(MusicPlayer, 0);
DP_DEF(TzConv, 0);
DP_DEF(SysConfig, 0);
DP_DEF(TrackFilter, 0);
DP_DEF(Pair, sizeof(HAL::device_info_t));
DP_DEF(Calibrate, sizeof(HAL::Joystick_Calibrate_t)*2);
DP_DEF(CarPower, sizeof(uint8_t));
DP_DEF(Connect, sizeof(int16_t));
DP_DEF(Remote, 0);