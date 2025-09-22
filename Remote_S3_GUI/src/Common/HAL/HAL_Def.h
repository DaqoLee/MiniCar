#ifndef __HAL_DEF_H
#define __HAL_DEF_H

#include <stdint.h>

namespace HAL
{

/* Clock */
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
} Clock_Info_t;

/* GPS */
typedef struct
{
    double longitude;
    double latitude;
    float altitude;
    float course;
    float speed;
    int16_t satellites;
    bool isVaild;
    Clock_Info_t clock;
} GPS_Info_t;

/* MAG */
typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} MAG_Info_t;

/* IMU */
typedef struct
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    int16_t steps;
} IMU_Info_t;

/* IMU */
typedef struct
{
    int16_t x1; 
    int16_t y1; 
    int16_t x2; 
    int16_t y2; 
} Joystick_Info_t;



/* SportStatus */
typedef struct
{
    uint32_t lastTick;

    float weight;

    float speedKph;
    float speedMaxKph;
    float speedAvgKph;

    union
    {
        uint32_t totalTimeUINT32[2];
        uint64_t totalTime;
    };

    float totalDistance;

    union
    {
        uint32_t singleTimeUINT32[2];
        uint64_t singleTime;
    };

    float singleDistance;
    float singleCalorie;
    
} SportStatus_Info_t;

/* Power */
typedef struct
{
    uint16_t voltage;
    uint8_t usage;
    bool isCharging;
} Power_Info_t;


typedef struct struct_pair {
  char type[16];    // 消息类型
  uint8_t data[32]; // 数据载荷
  int data_len;     // 数据长度
} struct_pair;


// 存储配对信息的数据结构
typedef struct {
  uint8_t mac[6];
  char name[32];
  unsigned long lastSeen;
} device_info_t;

typedef enum  {
  MODE_JOYSTICK,  // 摇杆控制模式
  MODE_OTA,        // OTA更新模式
  MODE_PAIR
} OperationMode_t;
typedef struct struct_ack {
  uint8_t mode; // 确认的消息ID
  uint8_t battery;
} struct_ack;

}

#endif
