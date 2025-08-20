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

// WiFi设置 - 两个独立的SSID
const char* mobileSsid = "ESP32C3-Car";  // 手机遥控模式热点
const char* password = "12345678";        // 通用密码

// 模式定义
enum OperationMode {
  MODE_MOBILE,    // 手机遥控模式
  MODE_JOYSTICK,  // 摇杆控制模式
  MODE_OTA        // OTA更新模式
};

volatile OperationMode currentMode = MODE_MOBILE;
CRGB leds[NUM_LEDS];

// 舵机参数
const int servoMin = 1050;
const int servoMax = 1650;
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
 // if(currentMode == newMode) return;
  
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
  }
  
  cleanupCurrentMode();
  currentMode = newMode;
  
  switch(newMode) {
    case MODE_MOBILE: initMobileMode(); break;
    case MODE_JOYSTICK: initJoystickMode(); break;
   case MODE_OTA: initOtaMode(); break;
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
  if(currentMode != MODE_JOYSTICK) return;
  
  memcpy(&rxData, data, sizeof(rxData));
  int servoAngle = map(rxData.joy1Y, 0, 4095, servoMin, servoMax);
  myservo.writeMicroseconds(servoAngle); 
  int motorSpeed = map(rxData.joy2X, 0, 4095, 255, -255);
  setMotorSpeed(motorSpeed);
  
  Serial.print("angle:");
  Serial.print(servoAngle);
  Serial.print(" speed:");
  Serial.println(motorSpeed);
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

void setup() {
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  Serial.begin(115200);
  
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  
  // 舵机初始化
  myservo.setPeriodHertz(50);
  myservo.attach(STEERING_PIN, 2, 1000, 2000);
  myservo.writeMicroseconds(servoCenter); 
  
  // 电机PWM初始化
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_A_PWM, 0);
  ledcSetup(1, 5000, 8);
  ledcAttachPin(MOTOR_B_PWM, 1);

  pinMode(CAR_SLEEP_PIN, OUTPUT);
  digitalWrite(CAR_SLEEP_PIN, HIGH);

  pinMode(POWER_KEY_PIN, INPUT);
  // button.setup(POWER_KEY_PIN, INPUT_PULLUP, false);
  // link the doubleclick function to be called on a doubleclick event.
  button.setPressMs(1500);

  button.attachDoubleClick(doubleClick);
  button.attachLongPressStart(longPress);

  analogReadResolution(12);
  updateBatteryInfo();


  // LED初始化
  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS); 
  leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
  FastLED.show();

  // 初始模式
  switchMode(currentMode);
  
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