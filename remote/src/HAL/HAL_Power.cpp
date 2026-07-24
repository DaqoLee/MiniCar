#include "HAL.h"
#include "Arduino.h"
#include "esp_adc_cal.h"

namespace HAL {
// 硬件配置（根据实际电路修改）
static const int BATTERY_PIN = 10;    // ADC引脚（GPIO34 = A2）
static const float VOLTAGE_DIVIDER = 2.15f; // 分压系数（1/3 → ×3）
static const float REFERENCE_VOLTAGE = 3.3f; // ESP32 ADC参考电压
static const int ADC_RESOLUTION = 4095;     // 12位ADC（0~4095）

// 电池参数（锂电池典型值）
static const float FULL_VOLTAGE = 4.2f;    // 满电电压
static const float EMPTY_VOLTAGE = 3.0f;   // 空电电压

// 状态变量
static float s_batteryVoltage = 0.0f;
static uint8_t s_batteryPercentage = 0;
static bool s_isCharging = false;
esp_adc_cal_characteristics_t adc_chars;
// 初始化：配置ADC引脚
void Power_Init() {
    pinMode(BATTERY_PIN, INPUT);  // 设置ADC引脚为输入
    analogSetPinAttenuation(BATTERY_PIN, ADC_11db); // 设置 11dB 衰减 (0-3.3V)
            /* 电池电压ADC设置 */
    analogReadResolution(12);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc_chars);

}

// 读取ADC值并转换为实际电压（Arduino简化版）
static float readVoltage() {
    // 多次采样取平均，减少噪声
    uint32_t sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(BATTERY_PIN);  // Arduino analogRead()返回0~4095
        delay(1);
    }
    uint16_t adcValue = sum / 10;

    // 转换为电压：ADC值 → 引脚电压 → 电池电压
    float pinVoltage = (adcValue * REFERENCE_VOLTAGE) / ADC_RESOLUTION;
    return pinVoltage * VOLTAGE_DIVIDER;  // 还原电池电压
}

// 电压转电量百分比（线性映射）
static uint8_t voltageToPercentage(float voltage) {
    if (voltage >= FULL_VOLTAGE) return 100;
    if (voltage <= EMPTY_VOLTAGE) return 0;
    return (uint8_t)((voltage - EMPTY_VOLTAGE) / (FULL_VOLTAGE - EMPTY_VOLTAGE) * 100);
}

// 检测充电状态（需硬件支持，示例）
bool Power_IsCharging() {
    // 若充电芯片的CHG引脚低电平表示充电中（根据硬件调整）
    // s_isCharging = (digitalRead(CHARGE_PIN) == LOW);
    return s_isCharging;  // 若无硬件，默认返回false
}

// 定期更新电量（建议每1秒调用一次）
void Power_Update() {
    s_batteryVoltage = readVoltage();
    s_batteryPercentage = voltageToPercentage(s_batteryVoltage);
    // （可选）更新充电状态
    // Power_IsCharging();
}

void Power_GetInfo(Power_Info_t* info)
{
    int usage = 100;
    bool isCharging = false;
    

    if (usage > 100)
    {
        usage = 100;
    }

    info->isCharging = isCharging;
    info->voltage = s_batteryVoltage*1000;
    info->usage = s_batteryPercentage;
}

void Power_SetEventCallback(Power_CallbackFunction_t callback)
{

}
}
