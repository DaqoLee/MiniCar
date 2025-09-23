#include "HAL.h"
#include "Utils/TonePlayer/TonePlayer.h"
#include "lvgl.h"
#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <Preferences.h>
#include "Common/DataProc/DataProc.h"

using namespace HAL;

#define JOY1_X 4
#define JOY1_Y 5
#define JOY2_X 2
#define JOY2_Y 1

#define MAX_JOY1_X 3400
#define MID_JOY1_X 1960
#define MIN_JOY1_X 630

#define MAX_JOY2_X 3050
#define MID_JOY2_X 1565
#define MIN_JOY2_X 100

#define MAX_JOY1_Y 2960
#define MID_JOY1_Y 1650
#define MIN_JOY1_Y 350

#define MAX_JOY2_Y 3420
#define MID_JOY2_Y 2070
#define MIN_JOY2_Y 680


#define JOY_MAX 100
#define JOY_MINI 0

#define MAX_VALUE 100
#define MIN_VALUE 0


// 定义最大设备数量
#define MAX_DEVICES 10

uint8_t pairedCount = 0;
uint8_t pairIndex = 0;
bool isPaired = false;
int16_t sendSuccessCount = 0;
uint8_t receiverMac[] = {0x0C, 0x4E, 0xA0, 0x21, 0x29, 0x3C};

Preferences preferences;

volatile OperationMode_t currentMode = MODE_JOYSTICK;
struct_ack ackData;

// static void Tone_Callback(uint32_t freq, uint16_t volume)
// {
//     HAL::Buzz_Tone(freq);
// }
device_info_t pairedDevices[MAX_DEVICES];

HAL::Joystick_Info_t joystick;

static HAL::CommitFunc_t CommitFunc;
static void* UserData;

static HAL::CommitFunc_t PairCommitFunc;
static void* PairData;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) ;

void HAL::Remote_Init()
{
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  /* 读取配对信息 */
  loadPairedDevices();
  /* 
  */
  listPairedDevices();
}

void HAL::Remote_SetMode(OperationMode_t mode)
{
//    esp_err_t result = esp_now_send(Mac, (uint8_t*)&joystick, sizeof(joystick));
    currentMode = mode;
    loadPairedDevices();
    listPairedDevices();
}

void HAL::Joystick_Init()
{
  pinMode(JOY1_X, INPUT);
  pinMode(JOY1_Y, INPUT);
  pinMode(JOY2_X, INPUT);
  pinMode(JOY2_Y, INPUT);

  analogReadResolution(12);
}
void HAL::Joystick_SetCommitCallback(CommitFunc_t func, void* userData)
{
    CommitFunc = func;
    UserData = userData;
}

void HAL::Pair_SetCommitCallback(CommitFunc_t func, void* userData)
{
    PairCommitFunc = func;
    PairData = userData;
}


void HAL::Joystick_Update()
{
    static int16_t steps;
    steps++;
    if (steps > 9999)
    {
        steps = 0;
    }

    uint16_t joy1Y = analogRead(JOY1_Y);
    uint16_t joy2Y = analogRead(JOY2_Y);
    uint16_t joy1X = analogRead(JOY1_X);
    uint16_t joy2X = analogRead(JOY2_X);

    joystick.x1 = map(joy1X, MIN_JOY1_X, MAX_JOY1_X, 0, 4095);
    joystick.y1 = map(joy1Y, MIN_JOY1_Y, MAX_JOY1_Y, 4095, 0);
    joystick.x2 = map(joy2X, MIN_JOY2_X, MAX_JOY2_X, 4095, 0);
    joystick.y2 = map(joy2Y, MIN_JOY2_Y, MAX_JOY2_Y, 4095, 0);

    // joystick.x1 = analogRead(JOY1_Y);
    // joystick.y1 = analogRead(JOY2_Y);
    // joystick.x2 = analogRead(JOY1_X);
    // joystick.y2 = analogRead(JOY2_X);


    if (CommitFunc)
    {
        CommitFunc(&joystick, UserData);
    }
}



void HAL::Remote_Update()
{
   if (!isPaired) {
    // 如果还未配对，则每隔5秒发送一次配对请求
    Serial.println("Sending pairing request...");

    struct_pair myRequest;
    strcpy(myRequest.type, "PAIRING");
    // 广播MAC地址已经在setup中添加
    uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_err_t result = esp_now_send(broadcastMac, (uint8_t *) &myRequest, sizeof(myRequest));

    if (result == ESP_OK) {
      Serial.println("Pairing request sent.");
    } else {
      Serial.println("Error sending pairing request.");
    }
    delay(5000);
  } else {

    if (currentMode == MODE_JOYSTICK)
    {     
        esp_err_t result = esp_now_send(pairedDevices[pairIndex].mac, (uint8_t*)&joystick, sizeof(joystick));
        
        if (result == ESP_OK) {

        sendSuccessCount = sendSuccessCount >= 200 ? 200 : sendSuccessCount+1; 
        // Serial.println(sendSuccessCount);
        } else {

        sendSuccessCount =  sendSuccessCount <= -50 ? -50 : sendSuccessCount-10; ;
        // Serial.println(sendSuccessCount);
        }
    }

  }



}

// 保存配对设备信息到NVS
void HAL::savePairedDevices() {
  preferences.begin("esnow_multi", false);
  preferences.putUInt("paired_count", pairedCount);
  preferences.putUInt("paired_index", pairIndex);
  
  for (int i = 0; i < pairedCount; i++) {
    char macKey[20];
    sprintf(macKey, "mac_%d", i);
    preferences.putBytes(macKey, pairedDevices[i].mac, 6);
    
    char nameKey[20];
    sprintf(nameKey, "name_%d", i);
    preferences.putString(nameKey, pairedDevices[i].name);

    Serial.println(macKey);
    Serial.println(nameKey);
  }
  
  preferences.end();
  Serial.println("配对设备信息已保存");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {

  switch (currentMode)
  {
  case MODE_JOYSTICK:{

    memcpy(&ackData, incomingData, sizeof(ackData));
    // Serial.printf("ACK mode: %d battery: %d\r\n",ackData.mode, ackData.battery);

    break;
  }  
  case MODE_PAIR:{


    struct_pair myData;
    memcpy(&myData, incomingData, sizeof(myData));
    
    // 处理配对响应
    if (strcmp(myData.type, "PAIRING_RESP") == 0) {
      // 检查是否已存在此设备
      bool exists = false;
      for (int i = 0; i < pairedCount; i++) {
        if (memcmp(pairedDevices[i].mac, mac, 6) == 0) {
          exists = true;
        if (PairCommitFunc)
        {
            PairCommitFunc(&pairedDevices[i], PairData);
        }
          break;
        }
      }
    // 如果不存在且未超过最大数量，则添加到配对列表
      if (!exists && pairedCount < MAX_DEVICES) {
        memcpy(pairedDevices[pairedCount].mac, mac, 6);
        
        // 从数据中提取设备名称（如果有）
        if (myData.data_len > 0) {
          strncpy(pairedDevices[pairedCount].name, (char*)myData.data, min(myData.data_len, 31));
          pairedDevices[pairedCount].name[31] = '\0';
        } else {
          // 如果没有名称，使用MAC地址后4位作为默认名称
          snprintf(pairedDevices[pairedCount].name, sizeof(pairedDevices[pairedCount].name), 
                    "Device_%02X%02X", mac[4], mac[5]);
        }
        pairedDevices[pairedCount].lastSeen = millis();

        if (PairCommitFunc)
        {
            PairCommitFunc(&pairedDevices[pairedCount], PairData);
        }

        /* 当前设备切换为新配对的设备 */
        pairIndex = pairedCount;
        pairedCount++;
        isPaired = true;
        Serial.print("新设备已添加: ");
        Serial.print(pairedDevices[pairedCount-1].name);
        Serial.print("，当前配对设备数: ");
        Serial.println(pairedCount);

        esp_now_peer_info_t peerInfo;
        memcpy(peerInfo.peer_addr, pairedDevices[pairIndex].mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;//老版本没有
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
          Serial.println("Error initializing ESP-NOW");
        // return;
        }
        
        // 保存配对信息
        savePairedDevices();

        currentMode = MODE_JOYSTICK;
      } else if (exists) {
        Serial.println("设备已存在，无需重复添加");
        isPaired = true;
      } else {

        memcpy(pairedDevices[MAX_DEVICES - 1].mac, mac, 6);
        
        // 从数据中提取设备名称（如果有）
        if (myData.data_len > 0) {
          strncpy(pairedDevices[MAX_DEVICES - 1].name, (char*)myData.data, min(myData.data_len, 31));
          pairedDevices[MAX_DEVICES - 1].name[31] = '\0';
        } else {
          // 如果没有名称，使用MAC地址后4位作为默认名称
          snprintf(pairedDevices[MAX_DEVICES - 1].name, sizeof(pairedDevices[MAX_DEVICES - 1].name), 
                    "Device_%02X%02X", mac[4], mac[5]);
        }
        
        pairedDevices[MAX_DEVICES - 1].lastSeen = millis();
        /* 当前设备切换为新配对的设备 */
        pairIndex = MAX_DEVICES - 1;
        isPaired = true;
        Serial.println("已达到最大设备数量限制，覆盖最后一个设备");

        esp_now_peer_info_t peerInfo;
        memcpy(peerInfo.peer_addr, pairedDevices[pairIndex].mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;//老版本没有
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
          Serial.println("Error initializing ESP-NOW");
        // return;
        }
        savePairedDevices();
        currentMode = MODE_JOYSTICK;

      }
    }      

    break;
  } 
  default:
    break;

  }
}


void HAL::listPairedDevices() {
  Serial.println("已配对设备列表:");
  Serial.println("索引\t设备名称\t\tMAC地址\t\t\t状态");
  Serial.println("----------------------------------------------------------------");
  
  for (int i = 0; i < pairedCount; i++) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             pairedDevices[i].mac[0], pairedDevices[i].mac[1], pairedDevices[i].mac[2], 
             pairedDevices[i].mac[3], pairedDevices[i].mac[4], pairedDevices[i].mac[5]);
    
    Serial.print(i);
    Serial.print("\t");
    Serial.print(pairedDevices[i].name);
    // 格式化输出对齐
    if (strlen(pairedDevices[i].name) < 8) Serial.print("\t");
    if (strlen(pairedDevices[i].name) < 16) Serial.print("\t");
    Serial.print("\t");
    Serial.print(macStr);
    Serial.print("\t");
    
    if (i == pairIndex) {
      Serial.println("[当前设备]");
    } else {
      Serial.println("");
    }
  }
  
  if (pairedCount == 0) {
    Serial.println("无已配对设备");
  }
}

void HAL::loadPairedDevices() {

  switch (currentMode)
  {
  case MODE_JOYSTICK:{

    preferences.begin("esnow_multi", true); // 只读模式打开
  /* 读取配对数 */
    pairedCount = preferences.getUInt("paired_count", 0);
    if (pairedCount)
    {
      /* 读取默认配对 */
      for (int i = 0; i < pairedCount; i++) {
          char macKey[20];
          sprintf(macKey, "mac_%d", i);
          preferences.getBytes(macKey, pairedDevices[i].mac, 6);
          
          char nameKey[20];
          sprintf(nameKey, "name_%d", i);
          pairedDevices[i].name[31] = '\0'; // 确保字符串终止
          preferences.getString(nameKey, pairedDevices[i].name, 32);
          
          pairedDevices[i].lastSeen = 0;
          Serial.println(macKey);
          Serial.println(nameKey);
      }
      pairIndex = preferences.getUInt("paired_index", 0);

      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, pairedDevices[pairIndex].mac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;//老版本没有
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
      // return;
      }
      isPaired = true;

    }
    else/* 如果没有配对信息就使用默认值 */
    {
      Serial.println("NO paired\r\n");
      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, receiverMac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;//老版本没有
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
      // return;
      }
      isPaired = true;
    }
    
    preferences.end();
    break;
  }  

  case MODE_PAIR:{
    /* 首先添加一个广播对等设备，用于发送配对请求 */
    uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, broadcastMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;//老版本没有
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
    // return;
    }
    isPaired = false;
    break;
  }  
  default:
    break;
  }
}


