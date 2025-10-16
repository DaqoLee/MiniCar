#if 1
#include <WiFi.h>

#include <Servo16.h>
#include <FastLED.h>

#include "OneButtonTiny.h"

#include "WebSocket.h"
#include "ota.h"
#include "remote.h"


// 引脚定义
#define MOTOR_A_PWM 6
#define MOTOR_B_PWM 7
#define STEERING_PIN 3
#define CAR_SLEEP_PIN 5
#define POWER_KEY_PIN 1
#define BATTERY_PIN 0
#define POWER_PIN 10
#define RGB_PIN 4
#define MODE_SWITCH_PIN 1

#define NUM_LEDS 1
CRGB leds[NUM_LEDS];
// WiFi设置 - 两个独立的SSID
const char* mobileSsid = "ESP32C3-Car";  // 手机遥控模式热点
const char* password = "12345678";        // 通用密码

// 模式定义
enum OperationMode {
 
  MODE_JOYSTICK,  // 摇杆控制模式
  MODE_MOBILE,    // 手机遥控模式
  MODE_OTA,        // OTA更新模式
  MODE_PAIR
};
// 定义一个结构体作为通信的数据格式
// typedef struct {
//   char instruction[32]; // 指令，例如 "PAIR_REQUEST" 或 "PAIR_RESPONSE"
//   uint8_t mac[6];       // 用于携带MAC地址
// } message_t;
// // 定义通信数据结构
typedef struct  {
  char type[16];    // 消息类型
  uint8_t data[32]; // 数据载荷
  int data_len;     // 数据长度
} message_t;
// 创建一个变量来保存发送端的MAC地址

typedef struct {
  uint8_t mac[6];
  char name[32];
} controller_info_t;

controller_info_t controllerInfo;

uint8_t controllerMac[6];

volatile OperationMode currentMode = MODE_JOYSTICK;


// 舵机参数
const int servoMin = 950;
const int servoMax = 1750;
const int servoCenter = 1350;
int currentSteering = servoCenter;
Servo myservo;

// 电机控制
int currentSpeed = 0;
const int maxSpeed = 255;

// 电池参数
float batteryVoltage = 0.0;
float batteryPercentage = 0.0;
const float maxBatteryVoltage = 4.2;
const float minBatteryVoltage = 3.0;
const float voltageDividerRatio = 2.0;
unsigned long lastBatteryUpdate = 0;
const long batteryUpdateInterval = 2000;


// 模式切换
unsigned long buttonPressTime = 0;
const long longPressDuration = 3000;
const int ledBrightness[] = {50, 150, 255}; // 不同模式的LED亮度


OneButtonTiny button(POWER_KEY_PIN);
// 函数声明
void PowerTask(void *pvParameters);
void updateBatteryInfo();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void setMotorSpeed(int speed);
void stopAllMotors();
void handleButton();
void switchMode(OperationMode newMode);
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);


void cleanupCurrentMode();

// 更新电池信息
void updateBatteryInfo() {
  int rawValue = analogRead(BATTERY_PIN);
  batteryVoltage = (((float)rawValue / 4095.0) * 3.3 * voltageDividerRatio) - 0.4;
  
  batteryPercentage = map(constrain(batteryVoltage*100, minBatteryVoltage*100, maxBatteryVoltage*100), 
                          minBatteryVoltage*100, maxBatteryVoltage*100, 0, 100);
  
  if(currentMode == MODE_MOBILE) {
    webSocket.broadcastTXT("battery:" + String(batteryVoltage, 1) + "," + String(batteryPercentage, 0));
  }
  
  Serial.printf("Battery: %.1fV (%d%%)\n", batteryVoltage, (int)batteryPercentage);
}

// 设置电机速度
void setMotorSpeed(int speed) {
  if (speed > 0) {  
    ledcWrite(0, speed);
    ledcWrite(1, 0);
  } else if (speed < 0) {  
    ledcWrite(0, 0);
    ledcWrite(1, -speed);
  } else {  
    ledcWrite(0, 0);
    ledcWrite(1, 0);
  }
}

// 停止所有电机
void stopAllMotors() {
  ledcWrite(0, 0);
  ledcWrite(1, 0);
}

// 初始化摇杆模式
void initJoystickMode() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  Serial.print("ESP32-C3 MAC Address: ");
  Serial.println(WiFi.macAddress());
  esp_now_register_recv_cb(onDataRecv);
  
  Serial.println("Joystick mode activated");
}

// 初始化手机遥控模式
void initMobileMode() {
  // 设置AP热点
  WiFi.softAP(mobileSsid, password);
  Serial.print("Mobile AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  // 设置HTTP服务器
  mobileServer.on("/", handleMobileRoot);
  mobileServer.begin();
  
  // 启动WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  Serial.println("Mobile mode activated");
}



// 模式切换函数
void switchMode(OperationMode newMode) {
 if(currentMode == newMode) return;
  
  Serial.print("Switching to mode: ");
  switch(newMode) {
    case MODE_MOBILE: 
    Serial.println("Mobile"); 
    leds[0] =CHSV(HUE_BLUE, 255, 30);
    setMotorSpeed(0);
    break;
    case MODE_JOYSTICK: 
    Serial.println("Joystick"); 
    leds[0] =CHSV(HUE_GREEN, 255, 30);
    break;
    case MODE_OTA: 
    Serial.println("OTA");
    leds[0] =CHSV(HUE_RED, 255, 30);
    break;

    case MODE_PAIR: 
    Serial.println("Pair");
    leds[0] =CHSV(HUE_PURPLE, 255, 30);
    break;
  }
  
  cleanupCurrentMode();
  currentMode = newMode;
  
  switch(newMode) {
    case MODE_MOBILE: initMobileMode(); break;
    case MODE_JOYSTICK: initJoystickMode(); break;
    case MODE_OTA: initOtaMode(); break;
    case MODE_PAIR: initJoystickMode(); break;
  }
  
  // 更新LED指示
 // leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
  FastLED.show();
}

// WebSocket事件处理（仅用于手机遥控模式）
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(currentMode != MODE_MOBILE) return;
  
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Received: %s\n", num, payload);
      
      char *token = strtok((char *)payload, ",");
      if(token != NULL) {
        int x = atoi(token);
        token = strtok(NULL, ",");
        if(token != NULL) {
          int y = atoi(token);
          
          int steering = map(-x, -100, 100, servoMin, servoMax);
          steering = constrain(steering, servoMin, servoMax);
          myservo.writeMicroseconds(steering);
          currentSteering = steering;
          
          int speed = map(abs(y), 0, 100, 0, maxSpeed);
          speed = constrain(speed, 0, maxSpeed);
          
          if(y > 10) {
            ledcWrite(0, speed);
            ledcWrite(1, 0);
            currentSpeed = speed;
          } 
          else if(y < -10) {
            ledcWrite(0, 0);
            ledcWrite(1, speed);
            currentSpeed = -speed;
          } 
          else {
            stopAllMotors();
          }
        }
      }
      break;
  }
}

// ESP-NOW数据接收回调
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {

  // if(currentMode != MODE_JOYSTICK) return;
  switch (currentMode)
  {
    case MODE_JOYSTICK:
    {
      static uint16_t count = 200;
      Serial.printf("rxData: %d,  len: %d \r\n",sizeof(rxData), len);
      if ((sizeof(rxData) != len)){
         myservo.writeMicroseconds(servoCenter); 
         setMotorSpeed(0);
        return;
      }
      memcpy(&rxData, data, sizeof(rxData));
      int servoAngle = map(rxData.joy2Y, 0, 4095, servoMax, servoMin);
      myservo.writeMicroseconds(servoAngle); 
      int motorSpeed = map(rxData.joy1X, 0, 4095, 255, -255);
      setMotorSpeed(motorSpeed);
      
      // Serial.print("angle:");
      // Serial.print(servoAngle);
      // Serial.print(" speed:");
      // Serial.println(motorSpeed);

      if (count++ >= 200)
      {
          // 检查是否已经添加过此设备
        if (esp_now_is_peer_exist(mac)) {
          // 设备已存在，无需再次添加
        } else {
          esp_now_peer_info_t peerInfo;
          memcpy(peerInfo.peer_addr, mac, 6);
          peerInfo.channel = 0;
          peerInfo.encrypt = false;
          peerInfo.ifidx = WIFI_IF_STA;//老版本没有
          if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Error initializing ESP-NOW");
          // return;
          }
          Serial.println("New peer added successfully");
        }
        ackData.mode = currentMode;
        ackData.battery = batteryPercentage;
        esp_err_t result = esp_now_send(mac, (uint8_t *) &ackData, sizeof(ackData));

        if (result == ESP_OK) {
          Serial.println("ACK sent successfully");
        } else {
          Serial.println("Error sending ACK");
        }
        count = 0;
      }
      
      break;
    }
    case MODE_PAIR:
    {
      // memcpy(&controllerMac, mac, sizeof(controllerMac)); // 保存发送端的MAC
      // Serial.print("Bytes received: ");
      // Serial.println(len);
      Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      message_t myData;
      memcpy(&myData, data, sizeof(myData));

      Serial.print("type: ");
      Serial.println(myData.type);



      bool isBroadcast = true;
      // for (int i = 0; i < 6; i++) {
      //   // Serial.print(mac[i]);
      //   if (mac[i] != 0xFF) {
      //     isBroadcast = false;
      //     break;
      //   }
      //   Serial.println();
      // }
      Serial.println(isBroadcast);
      Serial.println(strcmp(myData.type, "PAIRING"));

      // 判断是否是配对请求
      if (isBroadcast && (strcmp(myData.type, "PAIRING") == 0)) {

        memcpy(controllerInfo.mac, mac, 6);
            // 生成设备名称（使用MAC地址后4位）
        String macAddress = WiFi.macAddress();
        String devName = "ESP_Device_" + macAddress.substring(12, 14) + macAddress.substring(15, 17);
        strncpy(controllerInfo.name, devName.c_str(), sizeof(controllerInfo.name) - 1);
        controllerInfo.name[sizeof(controllerInfo.name) - 1] = '\0';
        Serial.println(controllerInfo.name);
        // 准备一个配对响应消息
        // 获取本设备的MAC地址并填入结构体
        // WiFi.macAddress(response.mac);

        // 将之前保存的“控制器”MAC地址添加为对等设备
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;
        esp_now_add_peer(&peerInfo);

        message_t response;
        strcpy(response.type, "PAIRING_RESP");
        strncpy((char*)response.data, controllerInfo.name, sizeof(response.data) - 1);
        response.data_len = strlen(controllerInfo.name);
        
        esp_err_t result = esp_now_send(controllerInfo.mac, (uint8_t *) &response, sizeof(response));
        
        if (result == ESP_OK) {
          Serial.println("配对响应已发送");
        } else {
          Serial.println("发送配对响应失败");
        }
      }
      else if (isBroadcast && (sizeof(rxData) == len))
      {
        switchMode(MODE_JOYSTICK);
      }
      break;  
    }
   
  // default:
  //   return;
  //   break;
  }



}


// 清理当前模式资源
void cleanupCurrentMode() {
  switch(currentMode) {
    case MODE_MOBILE:
      mobileServer.close();
      webSocket.close();
      WiFi.softAPdisconnect(true);
      break;
      
    case MODE_JOYSTICK:
      esp_now_deinit();
      WiFi.mode(WIFI_OFF);
      break;
      
    case MODE_OTA:
      otaServer.close();
      WiFi.softAPdisconnect(true);
      break;
  }
}

// 电源管理任务
void PowerTask(void *pvParameters) {
  static uint16_t count = 0;
  while (1) {
    // 关机检测
    button.tick();
    
    // 电池状态检测
    unsigned long currentMillis = millis();
    if (currentMillis - lastBatteryUpdate >= batteryUpdateInterval) {
      updateBatteryInfo();
      
    //   // 低电量指示（红灯）
    //   if (batteryVoltage < 3.5f) {
    //     leds[0] = CHSV(HUE_RED, 255, ledBrightness[currentMode]);
    //   } else {
    //     leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
    //   }
    //   FastLED.show();
      
      lastBatteryUpdate = currentMillis;
    }
    
    delay(10);
  }
}

void doubleClick()
{
  Serial.println("x2");
  OperationMode nextMode = static_cast<OperationMode>((currentMode + 1) % 3);
  switchMode(nextMode);
  // ledState = !ledState; // reverse the LED
  // digitalWrite(PIN_LED, ledState);
} // doubleClick



void longPress() {
  Serial.println("Button 1 longPress...");

  digitalWrite(POWER_PIN, LOW);
  leds[0] = CHSV(HUE_YELLOW, 255, 0);
  FastLED.show();
}  // lo


bool isPair()
{
  uint16_t count = 0;
  /* If the button is pressed continuously */
  while (!digitalRead(POWER_KEY_PIN))
  {
    count++;
    if (count >= 700)
    {
      currentMode = MODE_PAIR;
      break;
      /* code */
    }
    /* code */
    delay(10);
  }

  return true;
  
}

void setup() {
  /*上电保持*/
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);

  pinMode(POWER_KEY_PIN, INPUT_PULLUP);

  Serial.begin(115200);


  /*判断按键是否一直按下*/

  //isPress()
    /* LED初始化 */
  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS); 

  leds[0] = CHSV(HUE_GREEN, 255, 30);
  FastLED.show();

  isPair();

  /*如果按键一直按下超过7s，进入配对模式*/
  
  /* 否则正常初始化 */ 
  
   /* 舵机初始化 */
  myservo.setPeriodHertz(50);
  myservo.attach(STEERING_PIN, 2, 1000, 2000);
  myservo.writeMicroseconds(servoCenter); 
  
   /* 电机PWM初始化 */
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_A_PWM, 0);
  ledcSetup(1, 5000, 8);
  ledcAttachPin(MOTOR_B_PWM, 1);

  pinMode(CAR_SLEEP_PIN, OUTPUT);
  digitalWrite(CAR_SLEEP_PIN, HIGH);

  /* 按键设置 */
  button.setPressMs(1500);
  button.attachDoubleClick(doubleClick);
  button.attachLongPressStart(longPress);

  /* 电池电压ADC设置 */
  analogReadResolution(12);
  updateBatteryInfo();

  /* 初始模式 */
  switch(currentMode) {
    case MODE_MOBILE: 
      Serial.println("Mobile"); 
      leds[0] =CHSV(HUE_BLUE, 255, 30);
      setMotorSpeed(0);
      initMobileMode(); 
      break;
    case MODE_JOYSTICK:
      Serial.println("Joystick"); 
      leds[0] =CHSV(HUE_GREEN, 255, 30);
      initJoystickMode(); 
      break;
    case MODE_OTA:
      Serial.println("OTA");
      leds[0] =CHSV(HUE_RED, 255, 30);
      initOtaMode();
      break;
    case MODE_PAIR: 
      Serial.println("Pair");
      leds[0] =CHSV(HUE_PURPLE, 255, 30);
      initJoystickMode(); 
      break;
  }
  FastLED.show();
  // 创建电源管理任务
  xTaskCreate(PowerTask, "PowerTask", 4096, NULL, 1, NULL);
}

void loop() {
  switch(currentMode) {
    case MODE_MOBILE:
      webSocket.loop();
      mobileServer.handleClient();
      break;
      
    case MODE_JOYSTICK:
      delay(10);
      break;
      
    case MODE_OTA:
      otaServer.handleClient();
      delay(10);
      break;
  }
}
#endif