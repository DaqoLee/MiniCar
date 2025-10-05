#include <Arduino.h>       // Arduino核心库
#include "HAL.h"           // 自定义硬件抽象层
#include "Utils/TonePlayer/TonePlayer.h" // 蜂鸣器播放库
#include "lvgl.h"          // LVGL图形库（未使用可注释）
#include <esp_now.h>       // ESP-NOW通信库
#include <WiFi.h>          // WiFi库
#include "esp_wifi.h"      // ESP32 WiFi底层库
#include <Preferences.h>   // NVS存储库
#include "Common/DataProc/DataProc.h" // 数据处理库（未使用可注释）

using namespace HAL; // 使用HAL命名空间


// ==============================================================================
// 1. 宏定义（全量统一左右摇杆命名）
// ==============================================================================
// 1.1 硬件引脚定义（明确左/右摇杆X/Y轴）
#define LEFT_JOY_X_PIN    4   // 左摇杆X轴引脚
#define LEFT_JOY_Y_PIN    5   // 左摇杆Y轴引脚
#define RIGHT_JOY_X_PIN   2   // 右摇杆X轴引脚
#define RIGHT_JOY_Y_PIN   1   // 右摇杆Y轴引脚

// 1.2 摇杆默认校准参数（按左右区分，初始值）
// 左摇杆默认范围
#define LEFT_JOY_X_MAX    3400
#define LEFT_JOY_X_MID    1960
#define LEFT_JOY_X_MIN    630
#define LEFT_JOY_Y_MAX    2960
#define LEFT_JOY_Y_MID    1650
#define LEFT_JOY_Y_MIN    350

// 右摇杆默认范围
#define RIGHT_JOY_X_MAX   3050
#define RIGHT_JOY_X_MID   1565
#define RIGHT_JOY_X_MIN   100
#define RIGHT_JOY_Y_MAX   3420
#define RIGHT_JOY_Y_MID   2070
#define RIGHT_JOY_Y_MIN   680

// 1.3 系统配置参数（不变）
#define MAX_JOY_AD_VALUE 4095 // 摇杆AD采样最大值（12位AD）
#define MAX_PAIRED_DEVICES 10 // 最大配对设备数
#define PAIR_REQ_INTERVAL 5000 // 配对请求发送间隔（ms）
#define NVS_PAIR_NAMESPACE "esnow_multi" // 配对信息NVS命名空间
#define NVS_CALIB_NAMESPACE "joy_calibrate" // 摇杆校准NVS命名空间


// ==============================================================================
// 2. 全局变量（按左右摇杆区分，静态化减少冲突）
// ==============================================================================
static OperationMode_t currentMode = MODE_JOYSTICK; // 当前操作模式
static CalibrateMode_t calibrateStep = READ_MID;
static uint8_t pairedDeviceCount = 0;               // 已配对设备数量
static uint8_t currentPairedIndex = 0;              // 当前选中的配对设备索引
static bool isDevicePaired = false;                 // 是否已配对
static int16_t sendSuccessCount = 0;                // 数据发送成功计数
static const uint8_t defaultReceiverMac[] = {0x0C, 0x4E, 0xA0, 0x21, 0x29, 0x3C}; // 默认接收端MAC

// 摇杆校准数据（索引0=左摇杆，索引1=右摇杆，明确区分）
static Joystick_Calibrate_t joyCalibData[2] = {
    {LEFT_JOY_X_MAX, LEFT_JOY_X_MID, LEFT_JOY_X_MIN, 
     LEFT_JOY_Y_MAX, LEFT_JOY_Y_MID, LEFT_JOY_Y_MIN}, // 左摇杆校准初始值
    {RIGHT_JOY_X_MAX, RIGHT_JOY_X_MID, RIGHT_JOY_X_MIN, 
     RIGHT_JOY_Y_MAX, RIGHT_JOY_Y_MID, RIGHT_JOY_Y_MIN}  // 右摇杆校准初始值
};
static Joystick_Calibrate_t CalibData = {
    LEFT_JOY_X_MAX, LEFT_JOY_X_MID, LEFT_JOY_X_MIN, 
     LEFT_JOY_Y_MAX, LEFT_JOY_Y_MID, LEFT_JOY_Y_MIN // 左摇杆校准初始值
};
// 配对设备列表（不变）
static device_info_t pairedDevices[MAX_PAIRED_DEVICES] = {0};

// 摇杆数据缓存（核心优化：明确left/right成员）
static Joystick_Info_t joyData = {0};

// 回调函数指针（不变）
static CommitFunc_t joyDataCommitFunc = nullptr;  // 摇杆数据更新回调
static void* joyUserData = nullptr;               // 摇杆回调用户数据
static CommitFunc_t pairInfoCommitFunc = nullptr; // 配对信息更新回调
static void* pairUserData = nullptr;              // 配对回调用户数据
static CommitFunc_t CalibrateInfoCommitFunc = nullptr; // 校准信息更新回调
static void* CalibrateUserData = nullptr;              // 校准回调用户数据

static CommitFunc_t CarPowerInfoCommitFunc = nullptr; // 小车电量信息更新回调
static void* CarPowerUserData = nullptr;              // 小车电量回调用户数据
// NVS存储对象（不变）
static Preferences nvsStorage;


// ==============================================================================
// 3. 函数声明（按模块分组，明确左右摇杆相关函数）
// ==============================================================================
// 3.1 Remote模块（ESP-NOW通信）
// void Remote_Init(void);
// void Remote_SetMode(OperationMode_t mode);
// void Remote_Update(void);

// // 3.2 Joystick模块（左/右摇杆初始化/数据采集）
// void Joystick_Init(void);
// void Joystick_SetCommitCallback(CommitFunc_t func, void* userData);
// void Joystick_Update(void);

// // 3.3 配对与NVS存储模块
// void savePairedDevices(void);
// void loadPairedDevices(void);
// void listPairedDevices(void);

// // 3.4 摇杆校准模块（明确左/右摇杆校准逻辑）
// void loadJoyCalibrateData(void);
// void saveJoyCalibrateData(void);
// void calibrateLeftRightJoystick(void);

// 3.5 ESP-NOW接收回调函数
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len);


// ==============================================================================
// 4. 模块实现（全量替换为左右摇杆命名）
// ==============================================================================

// ------------------------------------------------------------------------------
// 4.1 Remote模块（ESP-NOW通信核心，摇杆数据发送用新命名）
// ------------------------------------------------------------------------------
/**
 * @brief 初始化遥控器（WiFi+ESP-NOW+配对信息加载）
 */
void HAL::Remote_Init()
{
    // WiFi配置：STA模式，关闭休眠（保证ESP-NOW稳定性）
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.disconnect(); // 断开STA连接（仅用ESP-NOW）

    // 初始化ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[Remote] ESP-NOW初始化失败!");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv); // 注册接收回调

    // 加载已配对设备信息并打印
    loadPairedDevices();
    listPairedDevices();

    Serial.println("[Remote] 遥控器初始化完成");
}

/**
 * @brief 设置遥控器操作模式
 * @param mode 目标模式（MODE_JOYSTICK / MODE_PAIR）
 */
void HAL::Remote_SetMode(OperationMode_t mode)
{
    currentMode = mode;
    loadPairedDevices(); // 根据模式重新加载配对信息
    listPairedDevices();
    Serial.printf("[Remote] 模式切换为：%s\n", 
                  mode == MODE_JOYSTICK ? "摇杆控制模式" : "设备配对模式");
}

/**
 * @brief 设置遥控器操作模式
 * @param mode 目标模式（MODE_JOYSTICK / MODE_PAIR）
 */
void HAL::Remote_SetCalibrateStep(CalibrateMode_t step)
{
    calibrateStep = step;
    // loadPairedDevices(); // 根据模式重新加载配对信息
    // listPairedDevices();
    Serial.printf("[Remote] 模式切换为：%d\n", step);
}

/**
 * @brief 遥控器主循环更新（发送配对请求/左/右摇杆数据）
 */
void HAL::Remote_Update()
{
    // 未配对：发送广播配对请求（间隔5秒）
    if (!isDevicePaired) {
        Serial.println("[Remote] 发送配对请求...");
        
        struct_pair pairReq;
        strncpy(pairReq.type, "PAIRING", sizeof(pairReq.type)-1);
        pairReq.data_len = 0; // 暂不携带附加数据

        // 广播地址（所有设备接收）
        uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        esp_err_t result = esp_now_send(broadcastMac, (uint8_t*)&pairReq, sizeof(pairReq));

        if (result == ESP_OK) {
            Serial.println("[Remote] 配对请求发送成功");
        } else {
            Serial.printf("[Remote] 配对请求发送失败，错误码：%d\n", result);
        }
        delay(PAIR_REQ_INTERVAL); // 控制发送频率
        return;
    }

    // 已配对（摇杆模式）：发送左/右摇杆数据
    if (currentMode == MODE_JOYSTICK) {
        esp_err_t result = esp_now_send(
            pairedDevices[currentPairedIndex].mac, 
            (uint8_t*)&joyData, // 发送的是包含left/right的摇杆数据
            sizeof(joyData)
        );

        // 更新发送成功计数（限制范围：-50 ~ 200）
        if (result == ESP_OK) {
            sendSuccessCount = (sendSuccessCount >= 200) ? 200 : sendSuccessCount + 1;
        } else {
            sendSuccessCount = (sendSuccessCount <= -50) ? -50 : sendSuccessCount - 10;
        }
    }
}


// ------------------------------------------------------------------------------
// 4.2 Joystick模块（核心优化：全量用左/右摇杆命名）
// ------------------------------------------------------------------------------
/**
 * @brief 初始化左/右摇杆（设置引脚模式+AD采样精度）
 */
void HAL::Joystick_Init()
{
    // 设置左摇杆引脚为模拟输入
    pinMode(LEFT_JOY_X_PIN, INPUT);
    pinMode(LEFT_JOY_Y_PIN, INPUT);
    // 设置右摇杆引脚为模拟输入
    pinMode(RIGHT_JOY_X_PIN, INPUT);
    pinMode(RIGHT_JOY_Y_PIN, INPUT);

    analogReadResolution(12); // 设置AD采样精度为12位（0~4095）
    loadJoyCalibrateData();   // 加载已保存的左/右摇杆校准数据
    Serial.println("[Joystick] 左/右摇杆初始化完成");
}

/**
 * @brief 设置摇杆数据更新回调（用于外部处理左/右摇杆数据）
 * @param func 回调函数指针
 * @param userData 回调用户数据（透传）
 */
void HAL::Joystick_SetCommitCallback(CommitFunc_t func, void* userData)
{
    joyDataCommitFunc = func;
    joyUserData = userData;
}

/**
 * @brief 更新左/右摇杆数据（采集+校准+回调通知）
 * 明确区分左摇杆和右摇杆的AD采集引脚，避免混淆
 */
void HAL::Joystick_Update()
{
    static uint32_t updateCounter = 0;
    updateCounter++;
    if (updateCounter > 9999) updateCounter = 0; // 计数溢出重置
    // 1. 采集左/右摇杆AD值（明确对应引脚）
    uint16_t leftJoyX = analogRead(LEFT_JOY_X_PIN);
    uint16_t leftJoyY = analogRead(LEFT_JOY_Y_PIN);
    uint16_t rightJoyX = analogRead(RIGHT_JOY_X_PIN);
    uint16_t rightJoyY = analogRead(RIGHT_JOY_Y_PIN);

    switch (currentMode)
    {
    case MODE_JOYSTICK:
        joyData.left_x  = JoyMap(leftJoyX , joyCalibData[0].min_x, joyCalibData[0].mid_x, joyCalibData[0].max_x, 0, 4095);
        joyData.left_y  = JoyMap(leftJoyY , joyCalibData[0].min_y, joyCalibData[0].mid_y, joyCalibData[0].max_y, 0, 4095);
        joyData.right_x = JoyMap(rightJoyX, joyCalibData[1].min_x, joyCalibData[1].mid_x, joyCalibData[1].max_x, 4095, 0);
        joyData.right_y = JoyMap(rightJoyY, joyCalibData[1].min_y, joyCalibData[1].mid_y, joyCalibData[1].max_y, 4095, 0);
        if (joyDataCommitFunc != nullptr) {
        joyDataCommitFunc(&joyData, joyUserData);
        }
        break;
    case MODE_CALIBRATE:
            // 2. 缓存左/右摇杆原始数据（后续校准使用）
        joyData.left_x = leftJoyX;
        joyData.left_y = leftJoyY;
        joyData.right_x = rightJoyX;
        joyData.right_y = rightJoyY;
        // 3. 执行左/右摇杆校准（实时更新最大/最小值）
        calibrateLeftRightJoystick();
        break;   
    default:
        break;
    }
    // Serial.printf(" %d %d %d %d \r\n",joyData.left_x,joyData.left_y,joyData.right_x,joyData.right_y );
    // 4. 回调通知外部（如LVGL显示、数据处理）

}

    // joystick.x1 = map(joy1X, MIN_JOY1_X, MAX_JOY1_X, 0, 4095);
    // joystick.y1 = map(joy1Y, MIN_JOY1_Y, MAX_JOY1_Y, 4095, 0);
    // joystick.x2 = map(joy2X, MIN_JOY2_X, MAX_JOY2_X, 4095, 0);
    // joystick.y2 = map(joy2Y, MIN_JOY2_Y, MAX_JOY2_Y, 4095, 0);

long HAL::JoyMap(long x, long in_min, long in_mid, long in_max, long out_min, long out_max)
{
    // 计算输出范围的中间值（保持原逻辑：整数除法向上取整趋势）
    const long mid_out = (out_max > out_min) ? (out_max + 1) / 2 : (out_min + 1) / 2;

    // 边界处理：输入超出范围时直接返回对应输出极值
    if (x <= in_min)  return out_min;
    if (x >= in_max)  return out_max;

    // 分区间映射（根据输入与中间值的关系）
    if (x <= in_mid) {
        // 输入在 [in_min, in_mid] 区间，映射到 [out_min, mid_out]
        return map(x, in_min, in_mid, out_min, mid_out);
    } else {
        // 输入在 [in_mid, in_max] 区间，映射到 [mid_out, out_max]
        return map(x, in_mid, in_max, mid_out, out_max);
    }
}

// ------------------------------------------------------------------------------
// 4.3 配对与NVS存储模块（不变，仅适配变量名）
// ------------------------------------------------------------------------------
/**
 * @brief 设置配对信息更新回调（用于外部显示配对状态）
 * @param func 回调函数指针
 * @param userData 回调用户数据（透传）
 */
void HAL::Pair_SetCommitCallback(CommitFunc_t func, void* userData)
{
    pairInfoCommitFunc = func;
    pairUserData = userData;
}

void HAL::Calibrate_SetCommitCallback(CommitFunc_t func, void* userData)
{
    CalibrateInfoCommitFunc = func;
    CalibrateUserData = userData;

}

void HAL::CarPower_SetCommitCallback(CommitFunc_t func, void* userData)
{
    CarPowerInfoCommitFunc = func;
    CarPowerUserData = userData;

}


/**
 * @brief 保存配对设备信息到NVS
 */
void HAL::savePairedDevices()
{
    nvsStorage.begin(NVS_PAIR_NAMESPACE, false); // 以可写模式打开NVS

    // 保存配对设备数量和当前选中索引
    nvsStorage.putUInt("paired_count", pairedDeviceCount);
    nvsStorage.putUInt("current_index", currentPairedIndex);

    // 保存每个设备的MAC和名称
    for (int i = 0; i < pairedDeviceCount; i++) {
        // 保存MAC地址（键：mac_0, mac_1...）
        char macKey[20];
        snprintf(macKey, sizeof(macKey), "mac_%d", i);
        nvsStorage.putBytes(macKey, pairedDevices[i].mac, 6);

        // 保存设备名称（键：name_0, name_1...）
        char nameKey[20];
        snprintf(nameKey, sizeof(nameKey), "name_%d", i);
        nvsStorage.putString(nameKey, pairedDevices[i].name);

        Serial.printf("[NVS] 保存设备%d:%s\n", i, pairedDevices[i].name);
    }

    nvsStorage.end(); // 关闭NVS
    Serial.println("[NVS] 配对设备信息保存完成");
}

/**
 * @brief 从NVS加载配对设备信息
 * 根据当前模式（配对/摇杆）配置ESP-NOW Peer
 */
void HAL::loadPairedDevices()
{
    switch (currentMode) {
        case MODE_JOYSTICK: {
            nvsStorage.begin(NVS_PAIR_NAMESPACE, true); // 只读模式打开NVS

            // 读取配对设备数量
            pairedDeviceCount = nvsStorage.getUInt("paired_count", 0);
            if (pairedDeviceCount > 0 && pairedDeviceCount <= MAX_PAIRED_DEVICES) {
                // 读取每个设备的MAC和名称
                for (int i = 0; i < pairedDeviceCount; i++) {
                    char macKey[20];
                    snprintf(macKey, sizeof(macKey), "mac_%d", i);
                    nvsStorage.getBytes(macKey, pairedDevices[i].mac, 6);

                    char nameKey[20];
                    snprintf(nameKey, sizeof(nameKey), "name_%d", i);
                    pairedDevices[i].name[31] = '\0'; // 确保字符串终止
                    nvsStorage.getString(nameKey, pairedDevices[i].name, 32);

                    pairedDevices[i].lastSeen = 0; // 初始化最后通信时间
                }

                // 读取当前选中的设备索引（防止越界）
                currentPairedIndex = nvsStorage.getUInt("current_index", 0);
                currentPairedIndex = (currentPairedIndex >= pairedDeviceCount) ? 0 : currentPairedIndex;

                // 添加当前设备为ESP-NOW Peer
                esp_now_peer_info_t peerInfo = {0};
                memcpy(peerInfo.peer_addr, pairedDevices[currentPairedIndex].mac, 6);
                peerInfo.channel = 0;       // 自动匹配信道
                peerInfo.encrypt = false;   // 不加密
                peerInfo.ifidx = WIFI_IF_STA;// 绑定STA接口
                if (esp_now_add_peer(&peerInfo) != ESP_OK) {
                    Serial.println("[NVS] 添加ESP-NOW Peer失败");
                }

                isDevicePaired = true;
                Serial.printf("[NVS] 加载配对设备%d个,当前选中,%s\n", 
                              pairedDeviceCount, pairedDevices[currentPairedIndex].name);
            } else {
                // 无配对信息：使用默认MAC作为Peer
                Serial.println("[NVS] 无已配对设备,使用默认MAC");
                esp_now_peer_info_t peerInfo = {0};
                memcpy(peerInfo.peer_addr, defaultReceiverMac, 6);
                peerInfo.channel = 0;
                peerInfo.encrypt = false;
                peerInfo.ifidx = WIFI_IF_STA;
                if (esp_now_add_peer(&peerInfo) != ESP_OK) {
                    Serial.println("[NVS] 添加默认Peer失败");
                }
                isDevicePaired = true;
            }

            nvsStorage.end();
            break;
        }

        case MODE_PAIR: {
            // 配对模式：添加广播地址作为Peer（用于发送配对请求）
            uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            esp_now_peer_info_t peerInfo = {0};
            memcpy(peerInfo.peer_addr, broadcastMac, 6);
            peerInfo.channel = 0;
            peerInfo.encrypt = false;
            peerInfo.ifidx = WIFI_IF_STA;
            if (esp_now_add_peer(&peerInfo) != ESP_OK) {
                Serial.println("[NVS] 添加广播Peer失败");
            }
            isDevicePaired = false;
            pairedDeviceCount = 0; // 配对模式下重置配对计数
            Serial.println("[NVS] 进入配对模式,已添加广播Peer");
            break;
        }

        default:
            break;
    }
}

/**
 * @brief 打印已配对设备列表（串口输出）
 */
void HAL::listPairedDevices()
{
    Serial.println("\n==================== 已配对设备列表 ====================");
    Serial.printf("%-4s | %-16s | %-20s | %s\n", "索引", "设备名称", "MAC地址", "状态");
    Serial.println("-------------------------------------------------------");

    if (pairedDeviceCount == 0) {
        Serial.println("  -    | 无已配对设备       | -                  | -");
    } else {
        for (int i = 0; i < pairedDeviceCount; i++) {
            // 格式化MAC地址字符串（AA:BB:CC:DD:EE:FF）
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     pairedDevices[i].mac[0], pairedDevices[i].mac[1], pairedDevices[i].mac[2],
                     pairedDevices[i].mac[3], pairedDevices[i].mac[4], pairedDevices[i].mac[5]);

            // 打印设备信息（对齐格式）
            Serial.printf("%-4d | %-16s | %-20s | %s\n",
                         i,
                         pairedDevices[i].name,
                         macStr,
                         (i == currentPairedIndex) ? "【当前设备】" : "");
        }
    }
    Serial.println("=======================================================\n");
}


// ------------------------------------------------------------------------------
// 4.4 摇杆校准模块（核心优化：明确左/右摇杆校准数据）
// ------------------------------------------------------------------------------
/**
 * @brief 从NVS加载左/右摇杆校准数据
 */
void HAL::loadJoyCalibrateData()
{
    nvsStorage.begin(NVS_CALIB_NAMESPACE, true); // 只读模式打开NVS

    // 加载左摇杆校准数据（键：left_joy_xxx）
    joyCalibData[0].max_x = nvsStorage.getUInt("left_joy_x_max", LEFT_JOY_X_MAX);
    joyCalibData[0].mid_x = nvsStorage.getUInt("left_joy_x_mid", LEFT_JOY_X_MID);
    joyCalibData[0].min_x = nvsStorage.getUInt("left_joy_x_min", LEFT_JOY_X_MIN);
    joyCalibData[0].max_y = nvsStorage.getUInt("left_joy_y_max", LEFT_JOY_Y_MAX);
    joyCalibData[0].mid_y = nvsStorage.getUInt("left_joy_y_mid", LEFT_JOY_Y_MID);
    joyCalibData[0].min_y = nvsStorage.getUInt("left_joy_y_min", LEFT_JOY_Y_MIN);

    // 加载右摇杆校准数据（键：right_joy_xxx）
    joyCalibData[1].max_x = nvsStorage.getUInt("right_joy_x_max", RIGHT_JOY_X_MAX);
    joyCalibData[1].mid_x = nvsStorage.getUInt("right_joy_x_mid", RIGHT_JOY_X_MID);
    joyCalibData[1].min_x = nvsStorage.getUInt("right_joy_x_min", RIGHT_JOY_X_MIN);
    joyCalibData[1].max_y = nvsStorage.getUInt("right_joy_y_max", RIGHT_JOY_Y_MAX);
    joyCalibData[1].mid_y = nvsStorage.getUInt("right_joy_y_mid", RIGHT_JOY_Y_MID);
    joyCalibData[1].min_y = nvsStorage.getUInt("right_joy_y_min", RIGHT_JOY_Y_MIN);

    nvsStorage.end();
    // Serial.println("[Calibrate] 左/右摇杆校准数据加载完成");
    Serial.printf("[Calibrate] 左摇杆:X(%d,%d,%d) Y(%d,%d,%d) | 右摇杆:X(%d,%d,%d) Y(%d,%d,%d)\n",
                joyCalibData[0].min_x, joyCalibData[0].mid_x, joyCalibData[0].max_x,
                joyCalibData[0].min_y, joyCalibData[0].mid_y, joyCalibData[0].max_y,
                joyCalibData[1].min_x, joyCalibData[1].mid_x, joyCalibData[1].max_x,
                joyCalibData[1].min_y, joyCalibData[1].mid_y, joyCalibData[1].max_y);
}

/**
 * @brief 保存左/右摇杆校准数据到NVS
 */
void HAL::saveJoyCalibrateData()
{
    nvsStorage.begin(NVS_CALIB_NAMESPACE, false); // 可写模式打开NVS

    // 保存左摇杆校准数据
    nvsStorage.putUInt("left_joy_x_max", joyCalibData[0].max_x);
    nvsStorage.putUInt("left_joy_x_mid", joyCalibData[0].mid_x);
    nvsStorage.putUInt("left_joy_x_min", joyCalibData[0].min_x);
    nvsStorage.putUInt("left_joy_y_max", joyCalibData[0].max_y);
    nvsStorage.putUInt("left_joy_y_mid", joyCalibData[0].mid_y);
    nvsStorage.putUInt("left_joy_y_min", joyCalibData[0].min_y);

    // 保存右摇杆校准数据
    nvsStorage.putUInt("right_joy_x_max", joyCalibData[1].max_x);
    nvsStorage.putUInt("right_joy_x_mid", joyCalibData[1].mid_x);
    nvsStorage.putUInt("right_joy_x_min", joyCalibData[1].min_x);
    nvsStorage.putUInt("right_joy_y_max", joyCalibData[1].max_y);
    nvsStorage.putUInt("right_joy_y_mid", joyCalibData[1].mid_y);
    nvsStorage.putUInt("right_joy_y_min", joyCalibData[1].min_y);

    nvsStorage.end();
    Serial.println("[Calibrate] 左/右摇杆校准数据保存完成");
}

/**
 * @brief 左/右摇杆校准逻辑（实时更新最大/最小值，需外部触发保存）
 * 步骤说明：0=初始化中间值 → 1=更新最大/最小值 → 2=预留确认 → 3=保存 → 4=退出
 */
void HAL::calibrateLeftRightJoystick()
{
    // 左摇杆临时缓存（最大/最小值）
    static uint16_t leftJoyMaxX = 0, leftJoyMaxY = 0, leftJoyMinX = 0, leftJoyMinY = 0;
    // 右摇杆临时缓存（最大/最小值）
    static uint16_t rightJoyMaxX = 0, rightJoyMaxY = 0, rightJoyMinX = 0, rightJoyMinY = 0;

    switch (calibrateStep) {
        // 步骤0：初始化中间值和临时缓存
        case READ_MID:
            // 以当前摇杆值为中间值（左摇杆+右摇杆）
            joyCalibData[0].mid_x = joyData.left_x;
            joyCalibData[0].mid_y = joyData.left_y;
            joyCalibData[1].mid_x = joyData.right_x;
            joyCalibData[1].mid_y = joyData.right_y;

            // 初始化最大/最小值为当前值
            leftJoyMaxX = leftJoyMinX = joyData.left_x;
            leftJoyMaxY = leftJoyMinY = joyData.left_y;
            rightJoyMaxX = rightJoyMinX = joyData.right_x;
            rightJoyMaxY = rightJoyMinY = joyData.right_y;

            // Serial.println("[Calibrate] 开始校准，请将左/右摇杆移动到所有极限位置...");
            break;

        // 步骤1：实时更新左/右摇杆的最大/最小值
        case READ_MAX_MIN:
            // 左摇杆：更新X/Y轴最大/最小值
            leftJoyMaxX = (joyData.left_x > leftJoyMaxX) ? joyData.left_x : leftJoyMaxX;
            leftJoyMinX = (joyData.left_x < leftJoyMinX) ? joyData.left_x : leftJoyMinX;
            leftJoyMaxY = (joyData.left_y > leftJoyMaxY) ? joyData.left_y : leftJoyMaxY;
            leftJoyMinY = (joyData.left_y < leftJoyMinY) ? joyData.left_y : leftJoyMinY;

            // 右摇杆：更新X/Y轴最大/最小值
            rightJoyMaxX = (joyData.right_x > rightJoyMaxX) ? joyData.right_x : rightJoyMaxX;
            rightJoyMinX = (joyData.right_x < rightJoyMinX) ? joyData.right_x : rightJoyMinX;
            rightJoyMaxY = (joyData.right_y > rightJoyMaxY) ? joyData.right_y : rightJoyMaxY;
            rightJoyMinY = (joyData.right_y < rightJoyMinY) ? joyData.right_y : rightJoyMinY;

            // 更新校准数据（同步到全局校准缓存）
            joyCalibData[0].max_x = leftJoyMaxX;
            joyCalibData[0].min_x = leftJoyMinX;
            joyCalibData[0].max_y = leftJoyMaxY;
            joyCalibData[0].min_y = leftJoyMinY;
            joyCalibData[1].max_x = rightJoyMaxX;
            joyCalibData[1].min_x = rightJoyMinX;
            joyCalibData[1].max_y = rightJoyMaxY;
            joyCalibData[1].min_y = rightJoyMinY;

            // 打印实时校准数据（调试用，明确左/右摇杆）
            Serial.printf("[Calibrate] 左摇杆:X(%d,%d,%d) Y(%d,%d,%d) | 右摇杆:X(%d,%d,%d) Y(%d,%d,%d)\n",
                         joyCalibData[0].min_x, joyCalibData[0].mid_x, joyCalibData[0].max_x,
                         joyCalibData[0].min_y, joyCalibData[0].mid_y, joyCalibData[0].max_y,
                         joyCalibData[1].min_x, joyCalibData[1].mid_x, joyCalibData[1].max_x,
                         joyCalibData[1].min_y, joyCalibData[1].mid_y, joyCalibData[1].max_y);
            break;

        // 步骤2：如果校准退出，使用初始值
        case EXIT:
            loadJoyCalibrateData();
            break;

        // 步骤3：保存左/右摇杆校准数据到NVS
        case SAVE:
            
            saveJoyCalibrateData();
            Serial.println("[Calibrate] 左/右摇杆校准完成！");
            calibrateStep = CALI_MAX;
           
            break;

        default:
            break;
    }

    if ( CalibrateInfoCommitFunc != nullptr) {
         CalibrateInfoCommitFunc(joyCalibData, CalibrateUserData);
        //   Serial.println("[Calibrate]CalibrateInfoCommitFunc");
    }
}


// ------------------------------------------------------------------------------
// 4.5 ESP-NOW接收回调函数（适配摇杆数据新命名）
// ------------------------------------------------------------------------------
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len)
{
    // 检查数据长度（至少大于结构体头部）
    if (len <= 0) return;

    // 根据当前模式处理数据
    switch (currentMode) {
        // 摇杆模式：处理接收端ACK反馈（不变）
        case MODE_JOYSTICK: {
            if (len != sizeof(struct_ack)) {
                Serial.printf("[Recv] ACK数据长度错误:%d(应为%d)\n", len, sizeof(struct_ack));
                return;
            }

            struct_ack recvAck;
            memcpy(&recvAck, incomingData, sizeof(recvAck));
            
            if ( CarPowerInfoCommitFunc != nullptr) {
                CarPowerInfoCommitFunc(&recvAck.battery, CarPowerUserData);
                //   Serial.println("[Calibrate]CalibrateInfoCommitFunc");
            }
            Serial.printf("[Recv] 接收端反馈：模式=%d,电量=%d%%\n", recvAck.mode, recvAck.battery);
            break;
        }

        // 配对模式：处理配对响应（不变，仅适配变量名）
        case MODE_PAIR: {
            if (len < sizeof(struct_pair)) {
                Serial.printf("[Recv] 配对数据长度错误：%d(最小应为%d)\n", len, sizeof(struct_pair));
                return;
            }

            struct_pair recvPair;
            memcpy(&recvPair, incomingData, sizeof(recvPair));

            // 仅处理配对响应（"PAIRING_RESP"）
            if (strcmp(recvPair.type, "PAIRING_RESP") != 0) {
                return;
            }

            // 检查设备是否已在配对列表中
            bool isDeviceExist = false;
            for (int i = 0; i < pairedDeviceCount; i++) {
                if (memcmp(pairedDevices[i].mac, mac, 6) == 0) {
                    isDeviceExist = true;
                    // 回调通知外部（如更新UI）
                    if (pairInfoCommitFunc != nullptr) {
                        pairInfoCommitFunc(&pairedDevices[i], pairUserData);
                    }
                    Serial.printf("[Recv] 设备已存在：%s\n", pairedDevices[i].name);
                    break;
                }
            }

            // 设备未存在：添加到配对列表
            if (!isDeviceExist) {
                // 未达最大设备数：追加新设备
                if (pairedDeviceCount < MAX_PAIRED_DEVICES) {
                    memcpy(pairedDevices[pairedDeviceCount].mac, mac, 6);

                    // 设置设备名称（优先使用接收数据中的名称，无则用MAC后4位）
                    if (recvPair.data_len > 0 && recvPair.data_len <= 31) {
                        strncpy(pairedDevices[pairedDeviceCount].name, (char*)recvPair.data, recvPair.data_len);
                        pairedDevices[pairedDeviceCount].name[recvPair.data_len] = '\0';
                    } else {
                        snprintf(pairedDevices[pairedDeviceCount].name, sizeof(pairedDevices[pairedDeviceCount].name),
                                 "Device_%02X%02X", mac[4], mac[5]);
                    }

                    pairedDevices[pairedDeviceCount].lastSeen = millis();
                    currentPairedIndex = pairedDeviceCount; // 当前设备切换为新配对设备
                    pairedDeviceCount++;
                } 
                // 已达最大设备数：覆盖最后一个设备
                else {
                    memcpy(pairedDevices[MAX_PAIRED_DEVICES - 1].mac, mac, 6);

                    if (recvPair.data_len > 0 && recvPair.data_len <= 31) {
                        strncpy(pairedDevices[MAX_PAIRED_DEVICES - 1].name, (char*)recvPair.data, recvPair.data_len);
                        pairedDevices[MAX_PAIRED_DEVICES - 1].name[recvPair.data_len] = '\0';
                    } else {
                        snprintf(pairedDevices[MAX_PAIRED_DEVICES - 1].name, sizeof(pairedDevices[MAX_PAIRED_DEVICES - 1].name),
                                 "Device_%02X%02X", mac[4], mac[5]);
                    }

                    pairedDevices[MAX_PAIRED_DEVICES - 1].lastSeen = millis();
                    currentPairedIndex = MAX_PAIRED_DEVICES - 1;
                    Serial.println("[Recv] 已达最大设备数，覆盖最后一个设备");
                }

                // 回调通知外部
                if (pairInfoCommitFunc != nullptr) {
                    pairInfoCommitFunc(&pairedDevices[currentPairedIndex], pairUserData);
                }

                // 添加新设备为ESP-NOW Peer
                esp_now_peer_info_t peerInfo = {0};
                memcpy(peerInfo.peer_addr, pairedDevices[currentPairedIndex].mac, 6);
                peerInfo.channel = 0;
                peerInfo.encrypt = false;
                peerInfo.ifidx = WIFI_IF_STA;
                if (esp_now_add_peer(&peerInfo) != ESP_OK) {
                    Serial.println("[Recv] 添加新Peer失败");
                }

                // 保存配对信息并切换到摇杆模式
                savePairedDevices();
                currentMode = MODE_JOYSTICK;
                isDevicePaired = true;
                Serial.printf("[Recv] 新设备配对成功：%s\n", pairedDevices[currentPairedIndex].name);
                listPairedDevices();
            }

            isDevicePaired = true;
            break;
        }

        default:
            break;
    }
}