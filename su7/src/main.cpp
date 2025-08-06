#if 1
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Servo16.h>
#include <FastLED.h>
#include <esp_now.h>
#include <Update.h>
#include "OneButtonTiny.h"
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

// 模式定义
enum OperationMode {
  MODE_MOBILE,    // 手机遥控模式
  MODE_JOYSTICK,  // 摇杆控制模式
  MODE_OTA        // OTA更新模式
};

volatile OperationMode currentMode = MODE_MOBILE;
CRGB leds[NUM_LEDS];

// WiFi设置 - 两个独立的SSID
const char* mobileSsid = "ESP32C3-Car";  // 手机遥控模式热点
const char* otaSsid = "ESP32C3-OTA";     // OTA模式热点
const char* password = "12345678";        // 通用密码

// 两个独立的服务器
WebServer mobileServer(80);    // 手机遥控模式服务器
WebServer otaServer(8080);     // OTA模式服务器（使用不同端口）
WebSocketsServer webSocket(81); // WebSocket服务器

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

// ESP-NOW 数据结构
typedef struct struct_message {
  int16_t joy1X;
  int16_t joy1Y;
  int16_t joy2X;
  int16_t joy2Y;
} struct_message;
struct_message rxData;

// 模式切换
unsigned long buttonPressTime = 0;
const long longPressDuration = 3000;
const int ledBrightness[] = {50, 150, 255}; // 不同模式的LED亮度


OneButtonTiny button(POWER_KEY_PIN);
// 函数声明
void PowerTask(void *pvParameters);
void updateBatteryInfo();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void handleMobileRoot();
void handleOtaRoot();
void handleOtaUpdate();
void setMotorSpeed(int speed);
void stopAllMotors();
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);
void handleButton();
void switchMode(OperationMode newMode);
void initMobileMode();
void initJoystickMode();
void initOtaMode();
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

// 手机遥控模式网页
void handleMobileRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32C3 Smart Car Control</title>
    <style>
      * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
      }
      body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d);
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
        color: white;
        overflow: hidden;
        padding: 20px;
      }
      .container {
        width: 100%;
        max-width: 500px;
        background: rgba(0, 0, 0, 0.7);
        border-radius: 20px;
        box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
        backdrop-filter: blur(10px);
        padding: 25px;
        text-align: center;
      }
      h1 {
        color: #ffffff;
        margin-bottom: 10px;
        text-shadow: 0 0 10px rgba(255, 255, 255, 0.5);
        font-size: 28px;
      }
      .subtitle {
        color: #cccccc;
        margin-bottom: 25px;
        font-size: 16px;
      }
      .status-panel {
        display: flex;
        justify-content: space-between;
        flex-wrap: wrap;
        margin-bottom: 25px;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 15px;
        padding: 15px;
        gap: 10px;
      }
      .status-item {
        text-align: center;
        flex: 1;
        min-width: 100px;
      }
      .status-label {
        font-size: 14px;
        margin-bottom: 5px;
        color: #aaaaaa;
      }
      .status-value {
        font-size: 24px;
        font-weight: bold;
        margin-top: 5px;
      }
      .battery-container {
        width: 100%;
        background: rgba(0, 0, 0, 0.3);
        border-radius: 10px;
        padding: 10px;
        margin-bottom: 20px;
      }
      .battery-header {
        display: flex;
        justify-content: space-between;
        margin-bottom: 5px;
      }
      .battery-info {
        font-size: 14px;
        color: #cccccc;
      }
      .battery-level {
        height: 30px;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 5px;
        overflow: hidden;
        position: relative;
      }
      .battery-fill {
        height: 100%;
        background: linear-gradient(90deg, #e74c3c, #f39c12, #2ecc71);
        border-radius: 5px;
        transition: width 0.5s ease;
      }
      .battery-percentage {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        font-weight: bold;
        text-shadow: 0 0 3px rgba(0, 0, 0, 0.8);
      }
      .joystick-container {
        position: relative;
        width: 300px;
        height: 300px;
        margin: 0 auto 30px;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 50%;
        display: flex;
        justify-content: center;
        align-items: center;
        box-shadow: inset 0 0 20px rgba(0, 0, 0, 0.5);
      }
      .joystick {
        position: absolute;
        width: 100px;
        height: 100px;
        background: linear-gradient(145deg, #3498db, #2980b9);
        border-radius: 50%;
        cursor: pointer;
        display: flex;
        justify-content: center;
        align-items: center;
        box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
        user-select: none;
        transition: transform 0.1s;
      }
      .joystick:active {
        transform: scale(0.95);
      }
      .joystick::before {
        content: "";
        position: absolute;
        width: 40px;
        height: 40px;
        background: rgba(255, 255, 255, 0.3);
        border-radius: 50%;
      }
      .steering-wheel {
        width: 120px;
        height: 120px;
        border: 4px solid #3498db;
        border-radius: 50%;
        margin: 0 auto 20px;
        position: relative;
        transform: rotate(0deg);
        transition: transform 0.2s ease;
      }
      .steering-wheel::before {
        content: "";
        position: absolute;
        top: 50%;
        left: 0;
        right: 0;
        height: 4px;
        background: #3498db;
      }
      .steering-wheel::after {
        content: "";
        position: absolute;
        top: 0;
        left: 50%;
        bottom: 0;
        width: 4px;
        background: #3498db;
      }
      .steering-center {
        position: absolute;
        top: 50%;
        left: 50%;
        width: 20px;
        height: 20px;
        background: #e74c3c;
        border-radius: 50%;
        transform: translate(-50%, -50%);
      }
      .connection-status {
        display: inline-block;
        padding: 8px 15px;
        border-radius: 20px;
        font-weight: bold;
        background: #e74c3c;
        margin-bottom: 20px;
      }
      .connected {
        background: #2ecc71;
      }
      .instructions {
        text-align: left;
        background: rgba(255, 255, 255, 0.1);
        padding: 20px;
        border-radius: 15px;
        font-size: 14px;
        margin-top: 20px;
      }
      .instructions h3 {
        margin-bottom: 10px;
        display: flex;
        align-items: center;
      }
      .instructions p {
        margin-bottom: 8px;
        display: flex;
        align-items: center;
      }
      .instructions i {
        margin-right: 10px;
        font-size: 18px;
      }
      .footer {
        margin-top: 20px;
        font-size: 12px;
        color: #aaaaaa;
      }
      
      @media (max-width: 500px) {
        .container {
          padding: 15px;
        }
        .joystick-container {
          width: 250px;
          height: 250px;
        }
        .joystick {
          width: 80px;
          height: 80px;
        }
        .status-item {
          min-width: 80px;
        }
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>ESP32C3 SMART CAR</h1>
      <div class="subtitle">Joystick Control with Battery Monitoring</div>
      
      <div class="connection-status" id="connectionStatus">Disconnected</div>
      
      <!-- 电池电量显示 -->
      <div class="battery-container">
        <div class="battery-header">
          <div class="battery-info">Battery Status</div>
          <div class="battery-info" id="batteryVoltage">0.0V</div>
        </div>
        <div class="battery-level">
          <div class="battery-fill" id="batteryFill" style="width: 0%"></div>
          <div class="battery-percentage" id="batteryPercentage">0%</div>
        </div>
      </div>
      
      <div class="status-panel">
        <div class="status-item">
          <div class="status-label">Steering</div>
          <div class="status-value" id="steeringValue">90°</div>
        </div>
        <div class="status-item">
          <div class="status-label">Speed</div>
          <div class="status-value" id="speedValue">0%</div>
        </div>
        <div class="status-item">
          <div class="status-label">Battery</div>
          <div class="status-value" id="batteryValue">0%</div>
        </div>
      </div>
      
      <div class="steering-wheel" id="steeringWheel">
        <div class="steering-center"></div>
      </div>
      
      <div class="joystick-container" id="joystickContainer">
        <div class="joystick" id="joystick"></div>
      </div>
      
      <div class="instructions">
        <h3>📋 Instructions:</h3>
        <p>📶 1. Connect to WiFi: <strong>ESP32C3-Car</strong> (Password: 12345678)</p>
        <p>🕹️ 2. Drag the joystick to control the car</p>
        <p>🔋 3. Battery status is updated every 2 seconds</p>
        <p>⚠️ 4. Release joystick to stop the car</p>
        <p>🔌 5. Connection Status: <span id="wsStatus">Disconnected</span></p>
      </div>
      
      <div class="footer">
        ESP32C3 Joystick Control | Battery Monitoring | Made with ❤️
      </div>
    </div>
    
    <script>
      let websocket;
      let isConnected = false;
      const joystick = document.getElementById('joystick');
      const joystickContainer = document.getElementById('joystickContainer');
      const steeringWheel = document.getElementById('steeringWheel');
      let joystickPos = { x: 0, y: 0 };
      let isDragging = false;
      
      // 初始化WebSocket连接
      function initWebSocket() {
        const ip = window.location.hostname;
        websocket = new WebSocket('ws://' + ip + ':81/');
        
        websocket.onopen = function() {
          console.log('WebSocket connected');
          isConnected = true;
          document.getElementById('connectionStatus').textContent = 'Connected';
          document.getElementById('connectionStatus').className = 'connection-status connected';
          document.getElementById('wsStatus').textContent = 'Connected';
        };
        
        websocket.onclose = function() {
          console.log('WebSocket disconnected');
          isConnected = false;
          document.getElementById('connectionStatus').textContent = 'Disconnected';
          document.getElementById('connectionStatus').className = 'connection-status';
          document.getElementById('wsStatus').textContent = 'Disconnected';
          setTimeout(initWebSocket, 2000);
        };
        
        websocket.onmessage = function(event) {
          console.log('Received: ' + event.data);
          
          // 检查是否是电池信息
          if(event.data.startsWith("battery:")) {
            const batteryData = event.data.substring(8).split(",");
            const voltage = parseFloat(batteryData[0]);
            const percentage = parseFloat(batteryData[1]);
            
            updateBatteryUI(voltage, percentage);
          }
        };
        
        websocket.onerror = function(error) {
          console.error('WebSocket Error: ', error);
        };
      }
      
      // 更新电池UI
      function updateBatteryUI(voltage, percentage) {
        document.getElementById('batteryVoltage').textContent = voltage.toFixed(1) + 'V';
        document.getElementById('batteryPercentage').textContent = percentage.toFixed(0) + '%';
        document.getElementById('batteryValue').textContent = percentage.toFixed(0) + '%';
        
        const batteryFill = document.getElementById('batteryFill');
        batteryFill.style.width = percentage + '%';
        
        // 根据电量改变颜色
        if(percentage > 60) {
          batteryFill.style.background = 'linear-gradient(90deg, #2ecc71, #2ecc71)';
        } else if(percentage > 30) {
          batteryFill.style.background = 'linear-gradient(90deg, #f39c12, #f39c12)';
        } else {
          batteryFill.style.background = 'linear-gradient(90deg, #e74c3c, #e74c3c)';
        }
      }
      
      // 发送控制命令
      function sendCommand(x, y) {
        if (isConnected) {
          const command = `${x},${y}`;
          websocket.send(command);
          
          // 更新UI
          document.getElementById('steeringValue').textContent = 
            Math.round(mapRange(x, -100, 100, 0, 180)) + '°';
            
          document.getElementById('speedValue').textContent = 
            Math.round(Math.abs(y)) + '%';
            
          // 更新方向盘角度
          const steeringAngle = mapRange(x, -100, 100, -180, 180);
          steeringWheel.style.transform = `rotate(${steeringAngle}deg)`;
        }
      }
      
      // 映射范围函数
      function mapRange(value, inMin, inMax, outMin, outMax) {
        return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
      }
      
      // 处理摇杆拖动
      function startDrag(e) {
        isDragging = true;
        joystick.style.transition = 'none';
        updateJoystick(e);
      }
      
      function stopDrag() {
        isDragging = false;
        joystick.style.transition = 'transform 0.3s ease';
        joystick.style.transform = 'translate(0, 0)';
        sendCommand(0, 0);
        joystickPos = { x: 0, y: 0 };
        
        // 重置UI
        document.getElementById('steeringValue').textContent = '90°';
        document.getElementById('speedValue').textContent = '0%';
        steeringWheel.style.transform = 'rotate(0deg)';
      }
      
      function updateJoystick(e) {
        if (!isDragging) return;
        
        const rect = joystickContainer.getBoundingClientRect();
        const centerX = rect.width / 2;
        const centerY = rect.height / 2;
        
        let clientX, clientY;
        
        if (e.type.includes('touch')) {
          clientX = e.touches[0].clientX;
          clientY = e.touches[0].clientY;
        } else {
          clientX = e.clientX;
          clientY = e.clientY;
        }
        
        const x = clientX - rect.left - centerX;
        const y = clientY - rect.top - centerY;
        
        // 限制在圆形范围内
        const distance = Math.min(Math.sqrt(x*x + y*y), centerX - 50);
        const angle = Math.atan2(y, x);
        
        joystickPos.x = Math.cos(angle) * distance;
        joystickPos.y = Math.sin(angle) * distance;
        
        // 更新摇杆位置
        joystick.style.transform = `translate(${joystickPos.x}px, ${joystickPos.y}px)`;
        
        // 转换为百分比 (-100到100)
        const xPercent = Math.round(mapRange(joystickPos.x, -centerX + 50, centerX - 50, -100, 100));
        const yPercent = Math.round(mapRange(joystickPos.y, -centerY + 50, centerY - 50, -100, 100));
        
        sendCommand(xPercent, yPercent);
      }
      
      // 初始化事件监听器
      function initControls() {
        // 鼠标事件
        joystick.addEventListener('mousedown', startDrag);
        document.addEventListener('mousemove', updateJoystick);
        document.addEventListener('mouseup', stopDrag);
        
        // 触摸事件
        joystick.addEventListener('touchstart', (e) => {
          e.preventDefault();
          startDrag(e);
        });
        document.addEventListener('touchmove', (e) => {
          if (isDragging) {
            e.preventDefault();
            updateJoystick(e);
          }
        });
        document.addEventListener('touchend', stopDrag);
        document.addEventListener('touchcancel', stopDrag);
      }
      
      // 页面加载时初始化
      window.onload = function() {
        initWebSocket();
        initControls();
      };
    </script>
  </body>
  </html>
  )rawliteral";
  
  
  mobileServer.send(200, "text/html", html);
}
// OTA模式网页
void handleOtaRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32C3 OTA Update</title>
    <style>
      * { margin: 0; padding: 0; box-sizing: border-box; }
      body { 
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #2c3e50, #1a1a2e);
        display: flex; justify-content: center; align-items: center;
        min-height: 100vh; color: white; overflow: hidden; padding: 20px;
      }
      .container {
        width: 100%; max-width: 500px; background: rgba(0, 0, 0, 0.7);
        border-radius: 20px; box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
        backdrop-filter: blur(10px); padding: 25px; text-align: center;
      }
      h1 { color: #fff; margin-bottom: 10px; text-shadow: 0 0 10px rgba(255,255,255,0.5); }
      .upload-container { margin: 30px 0; }
      .file-input { margin-bottom: 20px; }
      .upload-btn {
        background: #3498db; color: white; border: none; padding: 12px 24px;
        border-radius: 5px; cursor: pointer; font-size: 18px; font-weight: bold;
        transition: background 0.3s; display: inline-block;
      }
      .upload-btn:hover { background: #2980b9; }
      .progress-container {
        width: 100%; height: 25px; background: rgba(255,255,255,0.1);
        border-radius: 12px; margin: 25px 0; overflow: hidden; display: none;
      }
      .progress-bar {
        height: 100%; background: linear-gradient(90deg, #2ecc71, #3498db);
        width: 0%; transition: width 0.3s;
      }
      .status {
        margin-top: 20px; font-size: 18px; min-height: 27px;
      }
      .success { color: #2ecc71; }
      .error { color: #e74c3c; }
      .warning { color: #f39c12; }
      .remote-btn {
        background: #9b59b6; margin-top: 20px;
      }
      .remote-btn:hover { background: #8e44ad; }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>ESP32C3 FIRMWARE UPDATE111</h1>
      <p>Upload a new firmware (.bin file) to your device</p>
      
      <div class="upload-container">
        <div class="file-input">
          <input type="file" id="firmwareFile" accept=".bin">
        </div>
        <button class="upload-btn" id="uploadButton">Upload Firmware</button>
        
        <div class="progress-container" id="progressContainer">
          <div class="progress-bar" id="progressBar"></div>
        </div>
        
        <div class="status" id="status"></div>
      </div>
      
      <button class="upload-btn remote-btn" 
              onclick="location.href='http://192.168.4.1'">
        Switch to Remote Control
      </button>
    </div>
    
    <script>
      document.getElementById('uploadButton').addEventListener('click', uploadFirmware);
      
      function uploadFirmware() {
        const fileInput = document.getElementById('firmwareFile');
        const status = document.getElementById('status');
        const progressContainer = document.getElementById('progressContainer');
        const progressBar = document.getElementById('progressBar');
        
        if (!fileInput.files.length) {
          status.textContent = "Please select a firmware file first!";
          status.className = "status error";
          return;
        }
        
        const file = fileInput.files[0];
        if (!file.name.endsWith('.bin')) {
          status.textContent = "Please select a valid .bin file!";
          status.className = "status error";
          return;
        }
        
        const formData = new FormData();
        formData.append("firmware", file);
        
        status.textContent = "Uploading firmware...";
        status.className = "status warning";
        progressContainer.style.display = 'block';
        progressBar.style.width = '0%';
        
        const xhr = new XMLHttpRequest();
        xhr.open("POST", "/update", true);
        
        xhr.upload.onprogress = function(e) {
          if (e.lengthComputable) {
            const percent = Math.round((e.loaded / e.total) * 100);
            progressBar.style.width = percent + '%';
            status.textContent = `Uploading: ${percent}%`;
          }
        };
        
        xhr.onload = function() {
          if (xhr.status === 200) {
            progressBar.style.width = '100%';
            status.textContent = "Update successful! Rebooting device...";
            status.className = "status success";
            
            setTimeout(() => {
              status.innerHTML = "Reconnect to the device in <span id='countdown'>10</span> seconds...";
              startCountdown(10);
            }, 2000);
          } else {
            status.textContent = "Update failed: " + xhr.responseText;
            status.className = "status error";
          }
        };
        
        xhr.onerror = function() {
          status.textContent = "Upload error. Please try again.";
          status.className = "status error";
        };
        
        xhr.send(formData);
      }
      
      function startCountdown(seconds) {
        let count = seconds;
        const countdownEl = document.getElementById('countdown');
        
        const timer = setInterval(() => {
          count--;
          countdownEl.textContent = count;
          
          if (count <= 0) {
            clearInterval(timer);
            window.location.href = 'http://192.168.4.1';
          }
        }, 1000);
      }
    </script>
  </body>
  </html>
  )rawliteral";
  
  otaServer.send(200, "text/html", html);
}

// OTA固件上传处理
void handleOtaUpdate() {
  HTTPUpload& upload = otaServer.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update Start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

// OTA更新完成处理
void handleOtaUpdateDone() {
  otaServer.send(200, "text/plain", "OK");
  delay(1000);
  ESP.restart();
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

// ESP-NOW数据接收回调
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if(currentMode != MODE_JOYSTICK) return;
  
  memcpy(&rxData, data, sizeof(rxData));
  int servoAngle = map(rxData.joy2Y, 0, 4095, servoMin, servoMax);
  myservo.writeMicroseconds(servoAngle); 
  int motorSpeed = map(rxData.joy1X, 0, 4095, 255, -255);
  setMotorSpeed(motorSpeed);
  
  Serial.print("angle:");
  Serial.print(servoAngle);
  Serial.print(" speed:");
  Serial.println(motorSpeed);
}

// 按键处理
void handleButton() {
  static bool lastButtonState = HIGH;
  bool currentState = digitalRead(MODE_SWITCH_PIN);
  
  if(lastButtonState == HIGH && currentState == LOW) {
    buttonPressTime = millis();
  } 
  else if(lastButtonState == LOW && currentState == HIGH) {
    if(millis() - buttonPressTime < longPressDuration) {
      OperationMode nextMode = static_cast<OperationMode>((currentMode + 1) % 3);
      switchMode(nextMode);
    }
  }
  else if(currentState == LOW && (millis() - buttonPressTime > longPressDuration)) {
    switchMode(MODE_OTA);
  }
  
  lastButtonState = currentState;
}

// 模式切换函数
// void switchMode(OperationMode newMode) {
//   if(currentMode == newMode) return;
  
//   Serial.print("Switching to mode: ");
//   switch(newMode) {
//     case MODE_MOBILE: Serial.println("Mobile"); break;
//     case MODE_JOYSTICK: Serial.println("Joystick"); break;
//     case MODE_OTA: Serial.println("OTA"); break;
//   }
  
//   cleanupCurrentMode();
//   currentMode = newMode;
  
//   switch(newMode) {
//     case MODE_MOBILE: initMobileMode(); break;
//     case MODE_JOYSTICK: initJoystickMode(); break;
//     case MODE_OTA: initOtaMode(); break;
//   }
  
//   // 更新LED指示
//   leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
//   FastLED.show();
// }


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

// 初始化OTA模式
void initOtaMode() {
  // 设置AP热点（不同的SSID）
  WiFi.softAP(otaSsid, password);
  Serial.print("OTA AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  // 设置OTA服务器
  otaServer.on("/", handleOtaRoot);
  otaServer.on("/update", HTTP_POST, 
    []() { otaServer.send(200, "text/plain", "OK"); },
    handleOtaUpdate
  );
  otaServer.begin();
  
  Serial.println("OTA mode activated");
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
    // if (!digitalRead(POWER_KEY_PIN)) {
    //   count++;
    //   if (count > 1500) {
    //     digitalWrite(POWER_PIN, LOW);
    //     leds[0] = CHSV(HUE_YELLOW, 255, 0);
    //     FastLED.show();
    //   }
    // } else {
    //   count = 0;
    // }
    
    // 模式切换按键检测
    // handleButton();

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