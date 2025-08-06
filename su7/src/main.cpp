#if 0
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Servo16.h>
#include <FastLED.h>
#include <esp_now.h>

// 引脚定义
#define MOTOR_A_PWM 6   // 后轮电机PWM1
#define MOTOR_B_PWM 7   // 后轮电机PWM2
#define STEERING_PIN 3  // 舵机控制引脚
#define CAR_SLEEP_PIN 5 
#define POWER_KEY_PIN 1
#define BATTERY_PIN 0   // 电池电压检测引脚 (IO0)
#define POWER_PIN 10
#define RGB_PIN 4

#define NUM_LEDS 1

#define IS_REMOTE 1

CRGB leds[NUM_LEDS];
// WiFi设置
const char* ssid = "ESP32C3-Car";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// 舵机参数
const int servoMin = 1050;   // 0度对应的脉冲宽度(us)
const int servoMax = 1650;  // 180度对应的脉冲宽度(us)
const int servoCenter = 1350; // 中间位置(90)
int currentSteering = servoCenter;
Servo myservo; 
// 电机速度控制
int currentSpeed = 0;
const int maxSpeed = 255;

void PowerTask(void *pvParameters);
// 读取电池电压

// 电池参数
float batteryVoltage = 0.0;
float batteryPercentage = 0.0;
const float maxBatteryVoltage = 4.2; // 2S锂电池满电电压(8.4V)
const float minBatteryVoltage = 3.0; // 2S锂电池最低电压(6.0V)
const float voltageDividerRatio = 2.0; // 分压比 (两个100K电阻分压)
unsigned long lastBatteryUpdate = 0;
const long batteryUpdateInterval = 2000; // 每2秒更新一次电池信息



typedef struct struct_message {
  int16_t joy1X;
  int16_t joy1Y;
  int16_t joy2X;
  int16_t joy2Y;
} struct_message;
struct_message rxData;



void updateBatteryInfo() {
  // 读取ADC值（0-4095）
  int rawValue = analogRead(BATTERY_PIN);
  
  // 计算实际电压（分压比2:1，ADC参考电压3.3V）
  batteryVoltage = (((float)rawValue / 4095.0) * 3.3 * voltageDividerRatio) - 0.4;
  
  // 计算电量百分比（线性估算）
  batteryPercentage = map(constrain(batteryVoltage*100, minBatteryVoltage*100, maxBatteryVoltage*100), 
                          minBatteryVoltage*100, maxBatteryVoltage*100, 0, 100);
  
  // 广播电池信息给所有客户端
  String batteryInfo = "battery:" + String(batteryVoltage, 1) + "," + String(batteryPercentage, 0);
  webSocket.broadcastTXT(batteryInfo);
  
  Serial.printf("Battery: %.1fV (%d%%)\n", batteryVoltage, (int)batteryPercentage);
}
// WebSocket事件处理
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
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
      
      // 解析摇杆数据: "x,y" 格式
      char *token = strtok((char *)payload, ",");
      if(token != NULL) {
        int x = atoi(token);
        token = strtok(NULL, ",");
        if(token != NULL) {
          int y = atoi(token);
          
          // 控制舵机方向 (x�?: -100�?100)
          int steering = map(-x, -100, 100, servoMin, servoMax);
          steering = constrain(steering, servoMin, servoMax);
         	myservo.writeMicroseconds(steering);
          currentSteering = steering;
           Serial.printf("%d  steering :%d\n", x, steering);
          
          // 控制电机速度 (y�?: -100�?100)
          int speed = map(abs(y), 0, 100, 0, maxSpeed);
          speed = constrain(speed, 0, maxSpeed);
          
          if(y > 10) { // 前进
            ledcWrite(0, speed);  // 通道1用于电机A
            ledcWrite(1, 0);      // 通道2用于电机B
            currentSpeed = speed;
            webSocket.sendTXT(num, "Moving Forward");
          } 
          else if(y < -10) { // 后退
            ledcWrite(0, 0);
            ledcWrite(1, speed);
            currentSpeed = -speed;
            webSocket.sendTXT(num, "Moving Backward");
          } 
          else { // 停止
            ledcWrite(0, 0);
            ledcWrite(1, 0);
            currentSpeed = 0;
            webSocket.sendTXT(num, "Stopped");
          }
        }
      }
      break;
  }
}

#if 0
// 提供Web界面
void handleRoot() {
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
      .status-panel {
        display: flex;
        justify-content: space-between;
        margin-bottom: 25px;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 15px;
        padding: 15px;
      }
      .status-item {
        text-align: center;
        flex: 1;
      }
      .status-value {
        font-size: 24px;
        font-weight: bold;
        margin-top: 5px;
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
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>ESP32C3 SMART CAR</h1>
      <div class="subtitle">Joystick Control with WebSocket</div>
      
      <div class="connection-status" id="connectionStatus">Disconnected</div>
      
      <div class="status-panel">
        <div class="status-item">
          <div>Steering</div>
          <div class="status-value" id="steeringValue">90°</div>
        </div>
        <div class="status-item">
          <div>Speed</div>
          <div class="status-value" id="speedValue">0%</div>
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
        <p>🕹�? 2. Drag the joystick to control the car</p>
        <p>🔌 3. Connection Status: <span id="wsStatus">Disconnected</span></p>
        <p>⚠️ 4. Release joystick to stop the car</p>
      </div>
      
      <div class="footer">
        ESP32C3 Joystick Control | WebSocket Real-time | Made with ❤️
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
        };
        
        websocket.onerror = function(error) {
          console.error('WebSocket Error: ', error);
        };
      }
      
      // 发送控制命�?
      function sendCommand(x, y) {
        if (isConnected) {
          const command = `${x},${y}`;
          websocket.send(command);
          
          // 更新UI
          document.getElementById('steeringValue').textContent = 
            Math.round(mapRange(x, -200, 200, 0, 180)) + '°';
            
          document.getElementById('speedValue').textContent = 
            Math.round(Math.abs(y)) + '%';
            
          // 更新方向盘角�?
          const steeringAngle = mapRange(x, -200, 200, -180, 180);
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
        
        // 转换为百分比 (-100�?100)
        const xPercent = Math.round(mapRange(joystickPos.x, -centerX + 50, centerX - 50, -200, 200));
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
  
  server.send(200, "text/html", html);
}
#endif

#if 1
void handleRoot() {
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
      
      .direction-indicator {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        width: 60px;
        height: 60px;
        border-radius: 50%;
        background: rgba(255, 255, 255, 0.2);
        display: flex;
        justify-content: center;
        align-items: center;
        font-weight: bold;
        font-size: 18px;
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
      
      <div class="joystick-container" id="joystickContainer">
        <div class="joystick" id="joystick"></div>
        <div class="direction-indicator" id="directionIndicator">●</div>
      </div>
      
      <div class="instructions">
        <h3>📋 Instructions:</h3>
        <p>📶 1. Connect to WiFi: <strong>ESP32C3-Car</strong> (Password: 12345678)</p>
        <p>🕹️ 2. Drag the joystick to control the car</p>
        <p>🔋 3. Battery status updated every 2 seconds</p>
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
      const directionIndicator = document.getElementById('directionIndicator');
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
            
          // 更新方向指示器
          updateDirectionIndicator(x, y);
        }
      }
      
      // 更新方向指示器
      function updateDirectionIndicator(x, y) {
        let directionSymbol = "●"; // 默认停止
        
        if (y > 50) {
          directionSymbol = "↑"; // 前进
        } else if (y < -50) {
          directionSymbol = "↓"; // 后退
        } else if (x > 50) {
          directionSymbol = "→"; // 右转
        } else if (x < -50) {
          directionSymbol = "←"; // 左转
        } else if (y > 10) {
          directionSymbol = "↗"; // 前进+右转
        } else if (y < -10) {
          directionSymbol = "↘"; // 后退+右转
        } else if (x > 10 && y > 10) {
          directionSymbol = "↖"; // 前进+左转
        } else if (x < -10 && y < -10) {
          directionSymbol = "↙"; // 后退+左转
        }
        
        directionIndicator.textContent = directionSymbol;
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
        directionIndicator.textContent = "●"; // 重置方向指示器
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
  
  server.send(200, "text/html", html);
}

#endif


#if 0
void handleRoot() {
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
  
  server.send(200, "text/html", html);
}
#endif




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


void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  memcpy(&rxData, data, sizeof(rxData));
  
  
  int servoAngle = map(rxData.joy2Y, 0, 4095, servoMin, servoMax);
  // steeringServo.write(servoAngle);
  myservo.writeMicroseconds(servoAngle); 
  // 
  int motorSpeed = map(rxData.joy1X, 0, 4095, 255, -255);

  setMotorSpeed(motorSpeed);
  Serial.print("angle:");
  Serial.print(servoAngle);
  Serial.print("speed:");
  Serial.println(motorSpeed);
}


void setup() {

  pinMode(POWER_PIN,OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  Serial.begin(115200);
  
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(STEERING_PIN, 2, 1000, 2000); // attaches the servo on pin 18 to the servo object
	myservo.writeMicroseconds(servoCenter); 
  
  // 设置电机PWM
  ledcSetup(0, 5000, 8); // 通道1, 5kHz, 8位分辨率
  ledcAttachPin(MOTOR_A_PWM, 0);
  ledcSetup(1, 5000, 8); // 通道2, 5kHz, 8位分辨率
  ledcAttachPin(MOTOR_B_PWM, 1);

  pinMode(CAR_SLEEP_PIN,OUTPUT);
  digitalWrite(CAR_SLEEP_PIN, HIGH);

  pinMode(POWER_KEY_PIN, INPUT);
  
  analogReadResolution(12); // 12位分辨率 (0-4095)
  // pinMode(BATTERY_PIN, INPUT);
  updateBatteryInfo();

  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS); 
/*
    HUE_RED = 0,       ///< Red (0°)
    HUE_ORANGE = 32,   ///< Orange (45°)
    HUE_YELLOW = 64,   ///< Yellow (90°)
    HUE_GREEN = 96,    ///< Green (135°)
    HUE_AQUA = 128,    ///< Aqua (180°)
    HUE_BLUE = 160,    ///< Blue (225°)
    HUE_PURPLE = 192,  ///< Purple (270°)
    HUE_PINK = 224     ///< Pink (315°)
*/
  leds[0] = CHSV(HUE_GREEN, 255, 30);
  FastLED.show();

#if IS_REMOTE
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW");
    return;
  }

  Serial.print("ESP32-C3 MAC Address: ");
  Serial.println(WiFi.macAddress());
  // esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
#else

  // 设置WiFi AP
  WiFi.softAP(ssid, password);
  WiFi.setSleep(false);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  // 设置服务器路
  server.on("/", handleRoot);
  // 启动WebSocket服务
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.begin();
  Serial.println("HTTP and WebSocket servers started");
  // 启动HTTP服务
 #endif 

  xTaskCreate(PowerTask, "PowerTask", 4096, NULL, 1, NULL);
  
}

void PowerTask(void *pvParameters)//void *pvParameters
{
  static uint16_t count = 0;
  while (1)
  {
     if (!digitalRead(POWER_KEY_PIN))
    {
      count++;
      if (count > 1500)
      {
        digitalWrite(POWER_PIN, LOW);
        leds[0] = CHSV(HUE_YELLOW, 255, 0);
        FastLED.show();
      }
    
    }
    else
    {
      count = 0;
    }
    delay(1);

    unsigned long currentMillis = millis();
    if (currentMillis - lastBatteryUpdate >= batteryUpdateInterval) {
      updateBatteryInfo();

      if (batteryVoltage < 3.5f)
      {
         leds[0] = CHSV(HUE_RED, 255, 30);
         FastLED.show();
      }
      else
      {
        leds[0] = CHSV(HUE_GREEN, 255, 30);
        FastLED.show();
      }
      
      lastBatteryUpdate = currentMillis;
    }

  }
  
}

void loop() {

#if IS_REMOTE
  delay(100);
#else
  webSocket.loop();
  server.handleClient();
#endif

}

#endif

#if 0
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Servo16.h>
#include <FastLED.h>
#include <esp_now.h>
#include <ArduinoOTA.h>
#include "OneButton.h"


// 引脚定义
#define MOTOR_A_PWM 6   // 后轮电机PWM1
#define MOTOR_B_PWM 7   // 后轮电机PWM2
#define STEERING_PIN 3  // 舵机控制引脚
#define CAR_SLEEP_PIN 5 
#define POWER_KEY_PIN 1
#define BATTERY_PIN 0   // 电池电压检测引脚 (IO0)
#define POWER_PIN 10
#define RGB_PIN 4
#define MODE_SWITCH_PIN 1 // 模式切换按键

#define NUM_LEDS 1

// 模式定义
enum OperationMode {
  MODE_MOBILE,  // 手机遥控模式
  MODE_JOYSTICK, // 摇杆控制模式
  MODE_OTA       // 固件更新模式
};

volatile OperationMode currentMode = MODE_MOBILE; // 默认模式
CRGB leds[NUM_LEDS];

// WiFi设置
const char* ssid = "ESP32C3-Car";
const char* password = "12345678";
const char* otaSsid = "your_SSID";     // OTA模式使用的WiFi
const char* otaPassword = "your_PASSWORD";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// 舵机参数
const int servoMin = 1050;   // 0度对应的脉冲宽度(us)
const int servoMax = 1650;  // 180度对应的脉冲宽度(us)
const int servoCenter = 1350; // 中间位置(90)
int currentSteering = servoCenter;
Servo myservo; 

// 电机速度控制
int currentSpeed = 0;
const int maxSpeed = 255;

// 电池参数
float batteryVoltage = 0.0;
float batteryPercentage = 0.0;
const float maxBatteryVoltage = 4.2; // 2S锂电池满电电压(8.4V)
const float minBatteryVoltage = 3.0; // 2S锂电池最低电压(6.0V)
const float voltageDividerRatio = 2.0; // 分压比 (两个100K电阻分压)
unsigned long lastBatteryUpdate = 0;
const long batteryUpdateInterval = 2000; // 每2秒更新一次电池信息

// ESP-NOW 数据结构
typedef struct struct_message {
  int16_t joy1X;
  int16_t joy1Y;
  int16_t joy2X;
  int16_t joy2Y;
} struct_message;
struct_message rxData;

// 模式切换相关
unsigned long buttonPressTime = 0;
const long longPressDuration = 3000; // 长按3秒进入OTA模式
const int ledBrightness[] = {50, 150, 255}; // 不同模式的LED亮度

size_t otaTotalSize = 0;
size_t otaReceivedSize = 0;

// OTA状态
bool otaInProgress = false;
unsigned long otaStartTime = 0;

OneButton button;
// 函数声明
void PowerTask(void *pvParameters);
void updateBatteryInfo();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void handleRoot();
void setMotorSpeed(int speed);
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);
void handleButton();
void switchMode(OperationMode newMode);
void initMobileMode();
void initJoystickMode();
void initOTAMode();
void cleanupCurrentMode();
void processOTACommand(uint8_t num, String command);
void processOTAData(uint8_t num, uint8_t * payload, size_t length);
void stopAllMotors();
// 更新电池信息
void updateBatteryInfo() {
  int rawValue = analogRead(BATTERY_PIN);
  batteryVoltage = (((float)rawValue / 4095.0) * 3.3 * voltageDividerRatio) - 0.4;
  
  batteryPercentage = map(constrain(batteryVoltage*100, minBatteryVoltage*100, maxBatteryVoltage*100), 
                          minBatteryVoltage*100, maxBatteryVoltage*100, 0, 100);
  
  String batteryInfo = "battery:" + String(batteryVoltage, 1) + "," + String(batteryPercentage, 0);
  webSocket.broadcastTXT(batteryInfo);
  
  // Serial.printf("Battery: %.1fV (%d%%)\n", batteryVoltage, (int)batteryPercentage);
}

// WebSocket事件处理
// void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
//   if(currentMode != MODE_MOBILE) return;
  
//   switch(type) {
//     case WStype_DISCONNECTED:
//       Serial.printf("[%u] Disconnected!\n", num);
//       break;
//     case WStype_CONNECTED:
//       {
//         IPAddress ip = webSocket.remoteIP(num);
//         Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
//         webSocket.sendTXT(num, "Connected");
//       }
//       break;
//     case WStype_TEXT:
//       Serial.printf("[%u] Received: %s\n", num, payload);
      
//       char *token = strtok((char *)payload, ",");
//       if(token != NULL) {
//         int x = atoi(token);
//         token = strtok(NULL, ",");
//         if(token != NULL) {
//           int y = atoi(token);
          
//           int steering = map(-x, -100, 100, servoMin, servoMax);
//           steering = constrain(steering, servoMin, servoMax);
//           myservo.writeMicroseconds(steering);
//           currentSteering = steering;
//           Serial.printf("%d  steering :%d\n", x, steering);
          
//           int speed = map(abs(y), 0, 100, 0, maxSpeed);
//           speed = constrain(speed, 0, maxSpeed);
          
//           if(y > 10) { // 前进
//             ledcWrite(0, speed);
//             ledcWrite(1, 0);
//             currentSpeed = speed;
//           } 
//           else if(y < -10) { // 后退
//             ledcWrite(0, 0);
//             ledcWrite(1, speed);
//             currentSpeed = -speed;
//           } 
//           else { // 停止
//             ledcWrite(0, 0);
//             ledcWrite(1, 0);
//             currentSpeed = 0;
//           }
//         }
//       }
//       break;
//   }
// }


// WebSocket事件处理
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (otaInProgress) {
    // OTA进行中，优先处理OTA数据
    if (type == WStype_BIN) {
      processOTAData(num, payload, length);
    }
    return;
  }
  
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
        webSocket.sendTXT(num, "mode:" + String(currentMode == MODE_MOBILE ? "Mobile" : 
                                              currentMode == MODE_JOYSTICK ? "Joystick" : "OTA"));
      }
      break;
    case WStype_TEXT:
      {
        String command = String((char*)payload);
        if (command.startsWith("ota:")) {
          processOTACommand(num, command);
        } else {
          // 处理控制命令
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
        }
      }
      break;
    case WStype_BIN:
      // 在非OTA模式下忽略二进制数据
      break;
  }
}

// 处理OTA命令
void processOTACommand(uint8_t num, String command) {
  if (command == "ota:start") {
    // 开始OTA更新
    otaInProgress = true;
    otaStartTime = millis();
    otaTotalSize = 0;
    otaReceivedSize = 0;
    
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      webSocket.sendTXT(num, "ota:error:begin_failed");
      otaInProgress = false;
      return;
    }
    
    webSocket.sendTXT(num, "ota:started");
    Serial.println("OTA update started");
    
  } else if (command == "ota:end") {
    // 结束OTA更新
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\nRebooting...\n", otaReceivedSize);
      webSocket.broadcastTXT("ota:complete");
      
      // 发送完成消息后延迟重启
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      webSocket.broadcastTXT("ota:error:end_failed");
      otaInProgress = false;
    }
  }
}

// 处理OTA数据
void processOTAData(uint8_t num, uint8_t * payload, size_t length) {
  if (!otaInProgress) return;
  
  // 写入接收到的数据
  if (Update.write(payload, length) != length) {
    Update.printError(Serial);
    webSocket.sendTXT(num, "ota:error:write_failed");
    Update.abort();
    otaInProgress = false;
    return;
  }
  
  otaReceivedSize += length;
  
  // 每接收1KB或完成时更新进度
  if (otaReceivedSize % 1024 == 0 || length < 1024) {
    int progress = (otaTotalSize > 0) ? (otaReceivedSize * 100 / otaTotalSize) : 0;
    webSocket.sendTXT(num, "ota:progress:" + String(progress));
    Serial.printf("OTA Progress: %d%% (%d/%d bytes)\n", progress, otaReceivedSize, otaTotalSize);
  }
}


#if 0
// Web界面
void handleRoot() {
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
      
      .direction-indicator {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        width: 60px;
        height: 60px;
        border-radius: 50%;
        background: rgba(255, 255, 255, 0.2);
        display: flex;
        justify-content: center;
        align-items: center;
        font-weight: bold;
        font-size: 18px;
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
      
      <div class="joystick-container" id="joystickContainer">
        <div class="joystick" id="joystick"></div>
        <div class="direction-indicator" id="directionIndicator">●</div>
      </div>
      
      <div class="instructions">
        <h3>📋 Instructions:</h3>
        <p>📶 1. Connect to WiFi: <strong>ESP32C3-Car</strong> (Password: 12345678)</p>
        <p>🕹️ 2. Drag the joystick to control the car</p>
        <p>🔋 3. Battery status updated every 2 seconds</p>
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
      const directionIndicator = document.getElementById('directionIndicator');
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
            
          // 更新方向指示器
          updateDirectionIndicator(x, y);
        }
      }
      
      // 更新方向指示器
      function updateDirectionIndicator(x, y) {
        let directionSymbol = "●"; // 默认停止
        
        if (y > 50) {
          directionSymbol = "↑"; // 前进
        } else if (y < -50) {
          directionSymbol = "↓"; // 后退
        } else if (x > 50) {
          directionSymbol = "→"; // 右转
        } else if (x < -50) {
          directionSymbol = "←"; // 左转
        } else if (y > 10) {
          directionSymbol = "↗"; // 前进+右转
        } else if (y < -10) {
          directionSymbol = "↘"; // 后退+右转
        } else if (x > 10 && y > 10) {
          directionSymbol = "↖"; // 前进+左转
        } else if (x < -10 && y < -10) {
          directionSymbol = "↙"; // 后退+左转
        }
        
        directionIndicator.textContent = directionSymbol;
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
        directionIndicator.textContent = "●"; // 重置方向指示器
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
  
  server.send(200, "text/html", html);
}
#endif


#if 0
// 主网页
// 主网页
void handleRoot() {
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
      .direction-indicator {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        width: 60px;
        height: 60px;
        border-radius: 50%;
        background: rgba(255, 255, 255, 0.2);
        display: flex;
        justify-content: center;
        align-items: center;
        font-weight: bold;
        font-size: 18px;
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
      .mode-indicator {
        display: flex;
        justify-content: center;
        gap: 10px;
        margin-bottom: 20px;
      }
      .mode-dot {
        width: 15px;
        height: 15px;
        border-radius: 50%;
        background-color: #555;
      }
      .mode-dot.active {
        background-color: #2ecc71;
      }
      .ota-section {
        background: rgba(255, 255, 255, 0.1);
        border-radius: 10px;
        padding: 15px;
        margin-top: 20px;
      }
      .ota-button {
        background: #3498db;
        color: white;
        border: none;
        padding: 10px 20px;
        border-radius: 5px;
        cursor: pointer;
        font-size: 16px;
        font-weight: bold;
        margin-top: 15px;
        transition: background 0.3s;
      }
      .ota-button:hover {
        background: #2980b9;
      }
      .progress-container {
        width: 100%;
        height: 20px;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 10px;
        margin-top: 15px;
        overflow: hidden;
      }
      .progress-bar {
        height: 100%;
        background: linear-gradient(90deg, #2ecc71, #3498db);
        width: 0%;
        transition: width 0.3s;
      }
      .ota-status {
        margin-top: 10px;
        font-weight: bold;
      }
      .hidden {
        display: none;
      }
      .success {
        color: #2ecc71;
      }
      .error {
        color: #e74c3c;
      }
      .warning {
        color: #f39c12;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>ESP32C3 SMART CAR</h1>
      <div class="subtitle" id="subtitle">Remote Control Mode</div>
      
      <div class="connection-status" id="connectionStatus">Disconnected</div>
      
      <!-- 模式指示器 -->
      <div class="mode-indicator">
        <div class="mode-dot" id="modeMobile"></div>
        <div class="mode-dot" id="modeJoystick"></div>
        <div class="mode-dot" id="modeOta"></div>
      </div>
      
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
          <div class="status-label">Mode</div>
          <div class="status-value" id="modeValue">Mobile</div>
        </div>
      </div>
      
      <div class="joystick-container" id="joystickContainer">
        <div class="joystick" id="joystick"></div>
        <div class="direction-indicator" id="directionIndicator">●</div>
      </div>
      
      <div class="instructions">
        <h3>📋 Instructions:</h3>
        <p>📶 1. Connect to WiFi: <strong>ESP32C3-MiniCar</strong> (Password: 12345678)</p>
        <p>🕹️ 2. Drag the joystick to control the car</p>
        <p>🔋 3. Battery status updated every 2 seconds</p>
        <p>⚠️ 4. Release joystick to stop the car</p>
        <p>🔌 5. Connection Status: <span id="wsStatus">Disconnected</span></p>
        <p>🔘 6. Press MODE button to switch modes</p>
      </div>
      
      <div class="ota-section" id="otaSection">
        <h3>📱 OTA Firmware Update</h3>
        <p>Upload a new firmware (.bin file) to your device:</p>
        
        <div id="otaForm">
          <input type="file" id="firmwareFile" accept=".bin">
          <button class="ota-button" id="uploadButton">Upload Firmware</button>
          <div class="warning" id="fileWarning">Please select a .bin file first!</div>
        </div>
        
        <div class="progress-container hidden" id="progressContainer">
          <div class="progress-bar" id="progressBar"></div>
        </div>
        
        <div class="ota-status" id="otaStatus"></div>
      </div>
      
      <div class="footer">
        ESP32C3 Remote Car | Reliable OTA Update | Made with ❤️
      </div>
    </div>
    
    <script>
      let websocket;
      let isConnected = false;
      const joystick = document.getElementById('joystick');
      const joystickContainer = document.getElementById('joystickContainer');
      const directionIndicator = document.getElementById('directionIndicator');
      let joystickPos = { x: 0, y: 0 };
      let isDragging = false;
      let firmwareFile = null;
      let otaActive = false;
      
      // DOM元素
      const fileInput = document.getElementById('firmwareFile');
      const uploadButton = document.getElementById('uploadButton');
      const progressBar = document.getElementById('progressBar');
      const progressContainer = document.getElementById('progressContainer');
      const otaStatus = document.getElementById('otaStatus');
      const fileWarning = document.getElementById('fileWarning');
      
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
          // 检查模式更新
          else if(event.data.startsWith("mode:")) {
            const modeName = event.data.substring(5);
            updateModeUI(modeName);
          }
          // OTA开始
          else if(event.data === "ota:started") {
            otaStatus.textContent = "OTA update started...";
            otaStatus.className = "ota-status warning";
          }
          // OTA进度
          else if(event.data.startsWith("ota:progress:")) {
            const progress = parseInt(event.data.substring(13));
            updateProgressBar(progress);
          }
          // OTA完成
          else if(event.data === "ota:complete") {
            progressBar.style.width = "100%";
            otaStatus.textContent = "Update complete! Device is rebooting...";
            otaStatus.className = "ota-status success";
            
            // 显示重新连接提示
            setTimeout(() => {
              otaStatus.innerHTML = "Reconnect to the device in 10 seconds...";
              countdownReconnect(10);
            }, 2000);
          }
          // OTA错误
          else if(event.data.startsWith("ota:error:")) {
            const error = event.data.substring(10);
            otaStatus.textContent = "Error: " + error;
            otaStatus.className = "ota-status error";
            resetOTAForm();
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
      
      // 更新模式UI
      function updateModeUI(modeName) {
        document.getElementById('modeValue').textContent = modeName;
        document.getElementById('subtitle').textContent = modeName + " Mode";
        
        // 重置所有模式指示器
        document.getElementById('modeMobile').className = 'mode-dot';
        document.getElementById('modeJoystick').className = 'mode-dot';
        document.getElementById('modeOta').className = 'mode-dot';
        
        // 激活当前模式
        if(modeName === "Mobile") {
          document.getElementById('modeMobile').className = 'mode-dot active';
          document.getElementById('joystickContainer').style.display = 'block';
        }
        else if(modeName === "Joystick") {
          document.getElementById('modeJoystick').className = 'mode-dot active';
          document.getElementById('joystickContainer').style.display = 'none';
        }
        else if(modeName === "OTA") {
          document.getElementById('modeOta').className = 'mode-dot active';
          document.getElementById('joystickContainer').style.display = 'none';
        }
      }
      
      // 更新OTA进度条
      function updateProgressBar(progress) {
        progressBar.style.width = progress + '%';
        otaStatus.textContent = `Uploading: ${progress}%`;
        otaStatus.className = "ota-status warning";
      }
      
      // 倒计时重新连接
      function countdownReconnect(seconds) {
        if (seconds <= 0) {
          window.location.reload();
          return;
        }
        
        otaStatus.textContent = `Reconnect to the device in ${seconds} seconds...`;
        setTimeout(() => countdownReconnect(seconds - 1), 1000);
      }
      
      // 重置OTA表单
      function resetOTAForm() {
        fileInput.value = "";
        firmwareFile = null;
        progressBar.style.width = "0%";
        progressContainer.classList.add("hidden");
        uploadButton.disabled = false;
        otaActive = false;
      }
      
      // 发送控制命令
      function sendCommand(x, y) {
        if (isConnected && !otaActive) {
          const command = `${x},${y}`;
          websocket.send(command);
          
          document.getElementById('steeringValue').textContent = 
            Math.round(mapRange(x, -100, 100, 0, 180)) + '°';
            
          document.getElementById('speedValue').textContent = 
            Math.round(Math.abs(y)) + '%';
            
          updateDirectionIndicator(x, y);
        }
      }
      
      // 更新方向指示器
      function updateDirectionIndicator(x, y) {
        let directionSymbol = "●";
        
        if (y > 50) directionSymbol = "↑";
        else if (y < -50) directionSymbol = "↓";
        else if (x > 50) directionSymbol = "→";
        else if (x < -50) directionSymbol = "←";
        else if (y > 10) directionSymbol = "↗";
        else if (y < -10) directionSymbol = "↘";
        else if (x > 10 && y > 10) directionSymbol = "↖";
        else if (x < -10 && y < -10) directionSymbol = "↙";
        
        directionIndicator.textContent = directionSymbol;
      }
      
      // 映射范围函数
      function mapRange(value, inMin, inMax, outMin, outMax) {
        return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
      }
      
      // 处理摇杆拖动
      function startDrag(e) {
        if (otaActive) return;
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
        directionIndicator.textContent = "●";
      }
      
      function updateJoystick(e) {
        if (!isDragging || otaActive) return;
        
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
        
        const distance = Math.min(Math.sqrt(x*x + y*y), centerX - 50);
        const angle = Math.atan2(y, x);
        
        joystickPos.x = Math.cos(angle) * distance;
        joystickPos.y = Math.sin(angle) * distance;
        
        joystick.style.transform = `translate(${joystickPos.x}px, ${joystickPos.y}px)`;
        
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
        
        // OTA文件上传
        fileInput.addEventListener('change', function(e) {
          firmwareFile = e.target.files[0];
          if (firmwareFile) {
            if (firmwareFile.name.endsWith('.bin')) {
              fileWarning.style.display = 'none';
            } else {
              fileWarning.style.display = 'block';
              fileWarning.textContent = "Please select a valid .bin file!";
              firmwareFile = null;
            }
          }
        });
        
        uploadButton.addEventListener('click', function() {
          if (!firmwareFile) {
            fileWarning.style.display = 'block';
            return;
          }
          
          if (!firmwareFile.name.endsWith('.bin')) {
            fileWarning.style.display = 'block';
            fileWarning.textContent = "Please select a valid .bin file!";
            return;
          }
          
          otaActive = true;
          fileWarning.style.display = 'none';
          uploadButton.disabled = true;
          progressContainer.classList.remove("hidden");
          otaStatus.textContent = "Preparing OTA update...";
          otaStatus.className = "ota-status warning";
          
          // 启动OTA更新
          websocket.send("ota:start");
          
          // 读取并发送文件
          const reader = new FileReader();
          reader.onload = function(e) {
            const arrayBuffer = e.target.result;
            const firmwareData = new Uint8Array(arrayBuffer);
            
            // 分块发送固件数据 (1KB chunks)
            const chunkSize = 1024;
            for (let offset = 0; offset < firmwareData.length; offset += chunkSize) {
              const end = Math.min(offset + chunkSize, firmwareData.length);
              const chunk = firmwareData.subarray(offset, end);
              
              websocket.send(chunk);
              
              // 更新进度
              const progress = Math.round((offset / firmwareData.length) * 100);
              progressBar.style.width = progress + '%';
              otaStatus.textContent = `Uploading: ${progress}%`;
            }
            
            // 发送结束信号
            websocket.send("ota:end");
            otaStatus.textContent = "Finalizing update...";
          };
          
          reader.readAsArrayBuffer(firmwareFile);
        });
      }
      
      // 页面加载时初始化
      window.onload = function() {
        initWebSocket();
        initControls();
        updateModeUI("Mobile");
        fileWarning.style.display = 'none';
      };
    </script>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}
#endif
// OTA处理
void handleOTA() {
  server.send(200, "text/html", "OTA page");
}

// 固件上传处理
void handleUpdate() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    otaInProgress = true;
    otaStartTime = millis();
    Serial.printf("Update: %s\n", upload.filename.c_str());
    
    // 初始化更新
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      server.send(500, "text/plain", "UPDATE_BEGIN_FAILED");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // 写入接收到的数据
    if (Update.write(upload.buf, upload.currentSize)) {
      // 计算并发送进度
      int progress = (upload.totalSize > 0) ? (upload.totalSize * 100 / (upload.totalSize + 1)) : 0;
      webSocket.broadcastTXT("ota_progress:" + String(progress));
      Serial.printf("Progress: %d%%\n", progress);
    } else {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    // 完成更新
    if (Update.end(true)) {
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      webSocket.broadcastTXT("ota_complete");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      webSocket.broadcastTXT("ota_error:UPDATE_END_FAILED");
    }
  }
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
      // 短按：循环切换模式
      OperationMode nextMode = static_cast<OperationMode>((currentMode + 1) % 2);
      switchMode(nextMode);
    }
  }
  else if(currentState == LOW && (millis() - buttonPressTime > longPressDuration)) {
    // 长按：直接进入OTA模式
    switchMode(MODE_OTA);

    // digitalWrite(POWER_PIN, LOW);
    // leds[0] = CHSV(HUE_YELLOW, 255, 0);
    // FastLED.show();
}
  
  lastButtonState = currentState;
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
   case MODE_OTA: initOTAMode(); break;
  }
  
  // 更新LED指示
 // leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
  FastLED.show();
}

// 停止所有电机
void stopAllMotors() {
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  currentSpeed = 0;
}


// 初始化手机模式
void initMobileMode() {
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", handleRoot);

  server.on("/ota", handleOTA);
  server.on("/update", HTTP_POST, []() {
  server.send(200, "text/plain", "OK");
  }, handleUpdate);
  

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.begin();
  
  Serial.println("Mobile mode activated");

  
}

// 初始化摇杆模式
void initJoystickMode() {

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW");
    return;
  }

  Serial.print("ESP32-C3 MAC Address: ");
  Serial.println(WiFi.macAddress());
  // esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("Joystick mode activated");
}


// 初始化OTA模式 (AP模式)
void initOTAMode() {
  // 确保AP模式已启动
  // WiFi.softAP(ssid, password);
  // Serial.print("AP IP for OTA: ");
  // Serial.println(WiFi.softAPIP());
  
  // // 设置OTA参数
  // ArduinoOTA
  //   .onStart([]() {
  //     String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
  //     Serial.println("Start updating " + type);
  //     // 停止所有活动
  //     stopAllMotors();
  //   })
  //   .onEnd([]() {
  //     Serial.println("\nEnd");
  //   })
  //   .onProgress([](unsigned int progress, unsigned int total) {
  //     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  //   })
  //   .onError([](ota_error_t error) {
  //     Serial.printf("Error[%u]: ", error);
  //     if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
  //     else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
  //     else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
  //     else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  //     else if (error == OTA_END_ERROR) Serial.println("End Failed");
  //   });

  // // 设置OTA密码
  // ArduinoOTA.setPassword(password);
  
  // ArduinoOTA.begin();
  
  // Serial.println("OTA Ready (AP Mode)");

 WiFi.softAP(ssid, password);
 Serial.print("AP IP for OTA: ");
 Serial.println(WiFi.softAPIP());
  
  Serial.println("OTA mode activated");
}

// // 初始化OTA模式
// void initOTAMode() {
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(otaSsid, otaPassword);
  
//   while(WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\nConnected to WiFi");
  
//   ArduinoOTA
//     .onStart([]() {
//       String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
//       Serial.println("Start updating " + type);
//     })
//     .onEnd([]() {
//       Serial.println("\nEnd");
//     })
//     .onProgress([](unsigned int progress, unsigned int total) {
//       Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
//     })
//     .onError([](ota_error_t error) {
//       Serial.printf("Error[%u]: ", error);
//     });
  
//   ArduinoOTA.begin();
//   Serial.println("OTA Ready");
//   Serial.println("IP: " + WiFi.localIP().toString());
// }

// 清理当前模式资源
void cleanupCurrentMode() {
  switch(currentMode) {
    case MODE_MOBILE:
      webSocket.close();
      server.stop();
      WiFi.softAPdisconnect(true);
      break;
      
    case MODE_JOYSTICK:
      esp_now_deinit();
      WiFi.mode(WIFI_OFF);
      break;
      
    case MODE_OTA:
      // ArduinoOTA.end();
      // WiFi.disconnect();
      break;
  }
  WiFi.mode(WIFI_OFF);
}

void PowerTask(void *pvParameters) {
  static uint16_t count = 0;
  while (1) {
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
    
    // 按键检测
    handleButton();
    
    // 电池检测
    unsigned long currentMillis = millis();
    if (currentMillis - lastBatteryUpdate >= batteryUpdateInterval) {
      updateBatteryInfo();
      
      // 低电量指示（仅在非OTA模式下）
      // if(currentMode != MODE_OTA) {
      //   if (batteryVoltage < 3.5f) {
      //     leds[0] = CHSV(HUE_RED, 255, ledBrightness[currentMode]);
      //   } else {
      //     leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
      //   }
      //   FastLED.show();
      // }
      
      lastBatteryUpdate = currentMillis;
    }
    
    delay(1);
  }
}


void doubleClick()
{
  Serial.println("x2");

  // ledState = !ledState; // reverse the LED
  // digitalWrite(PIN_LED, ledState);
} // doubleClick


void setup() {
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  Serial.begin(115200);
  
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP); // 模式切换按键
  // pinMode(POWER_KEY_PIN, INPUT);
  
  // button.setup(POWER_KEY_PIN, INPUT_PULLUP, true);

  // link the doubleclick function to be called on a doubleclick event.
  // button.attachDoubleClick(doubleClick);
  
  myservo.setPeriodHertz(50);
  myservo.attach(STEERING_PIN, 2, 1000, 2000);
  myservo.writeMicroseconds(servoCenter); 
  
  // 设置电机PWM
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_A_PWM, 0);
  ledcSetup(1, 5000, 8);
  ledcAttachPin(MOTOR_B_PWM, 1);

  pinMode(CAR_SLEEP_PIN, OUTPUT);
  digitalWrite(CAR_SLEEP_PIN, HIGH);


  analogReadResolution(12);
  updateBatteryInfo();

  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS); 
  leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
  FastLED.show();

  // 根据初始模式初始化
  switchMode(currentMode);
  
  // 创建电源管理任务
  xTaskCreate(PowerTask, "PowerTask", 4096, NULL, 1, NULL);
}

void loop() {
  switch(currentMode) {
    case MODE_MOBILE:
      webSocket.loop();
      server.handleClient();
      break;
      
    case MODE_JOYSTICK:
      // ESP-NOW模式无需额外处理
      delay(10);
      break;
      
    case MODE_OTA:
      // ArduinoOTA.handle();
      // 5分钟无操作自动退出OTA模式
      static unsigned long otaStartTime = millis();
      if(millis() - otaStartTime > 300000) { // 5分钟
        switchMode(MODE_MOBILE);
      }
      break;
  }
}
#endif


#if 0
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Servo16.h>
#include <FastLED.h>
#include <esp_now.h>
#include <ArduinoOTA.h>
#include <Update.h>

// 引脚定义
#define MOTOR_A_PWM 6
#define MOTOR_B_PWM 7
#define STEERING_PIN 3
#define CAR_SLEEP_PIN 5
#define POWER_KEY_PIN 1
#define BATTERY_PIN 0
#define POWER_PIN 10
#define RGB_PIN 4
#define MODE_SWITCH_PIN 9

#define NUM_LEDS 1

// 模式定义
enum OperationMode {
  MODE_MOBILE,
  MODE_JOYSTICK,
  MODE_OTA
};

volatile OperationMode currentMode = MODE_MOBILE;
CRGB leds[NUM_LEDS];

// WiFi设置
const char* apSsid = "ESP32C3-Car";
const char* apPassword = "12345678";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

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
const int ledBrightness[] = {50, 150, 255};

// OTA状态
bool otaInProgress = false;
unsigned long otaStartTime = 0;

// 函数声明
void PowerTask(void *pvParameters);
void updateBatteryInfo();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void handleRoot();
void handleOTA();
void handleUpdate();
void handleUpdateDone();
void handleUpdateError();
void setMotorSpeed(int speed);
void stopAllMotors();
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len);
void handleButton();
void switchMode(OperationMode newMode);
void initMobileMode();
void initJoystickMode();
void initOTAMode();
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

// WebSocket事件处理
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
        webSocket.sendTXT(num, "mode:" + String(currentMode == MODE_MOBILE ? "Mobile" : 
                                              currentMode == MODE_JOYSTICK ? "Joystick" : "OTA"));
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

// 主网页
void handleRoot() {
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
      .direction-indicator {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        width: 60px;
        height: 60px;
        border-radius: 50%;
        background: rgba(255, 255, 255, 0.2);
        display: flex;
        justify-content: center;
        align-items: center;
        font-weight: bold;
        font-size: 18px;
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
      .mode-indicator {
        display: flex;
        justify-content: center;
        gap: 10px;
        margin-bottom: 20px;
      }
      .mode-dot {
        width: 15px;
        height: 15px;
        border-radius: 50%;
        background-color: #555;
      }
      .mode-dot.active {
        background-color: #2ecc71;
      }
      .ota-section {
        background: rgba(255, 255, 255, 0.1);
        border-radius: 10px;
        padding: 15px;
        margin-top: 20px;
      }
      .ota-button {
        background: #3498db;
        color: white;
        border: none;
        padding: 10px 20px;
        border-radius: 5px;
        cursor: pointer;
        font-size: 16px;
        font-weight: bold;
        margin-top: 15px;
        transition: background 0.3s;
      }
      .ota-button:hover {
        background: #2980b9;
      }
      .progress-container {
        width: 100%;
        height: 20px;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 10px;
        margin-top: 15px;
        overflow: hidden;
      }
      .progress-bar {
        height: 100%;
        background: linear-gradient(90deg, #2ecc71, #3498db);
        width: 0%;
        transition: width 0.3s;
      }
      .ota-status {
        margin-top: 10px;
        font-weight: bold;
      }
      .hidden {
        display: none;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>ESP32C3 SMART CAR</h1>
      <div class="subtitle" id="subtitle">Remote Control Mode</div>
      
      <div class="connection-status" id="connectionStatus">Disconnected</div>
      
      <!-- 模式指示器 -->
      <div class="mode-indicator">
        <div class="mode-dot" id="modeMobile"></div>
        <div class="mode-dot" id="modeJoystick"></div>
        <div class="mode-dot" id="modeOta"></div>
      </div>
      
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
          <div class="status-label">Mode</div>
          <div class="status-value" id="modeValue">Mobile</div>
        </div>
      </div>
      
      <div class="joystick-container" id="joystickContainer">
        <div class="joystick" id="joystick"></div>
        <div class="direction-indicator" id="directionIndicator">●</div>
      </div>
      
      <div class="instructions">
        <h3>📋 Instructions:</h3>
        <p>📶 1. Connect to WiFi: <strong>ESP32C3-Car</strong> (Password: 12345678)</p>
        <p>🕹️ 2. Drag the joystick to control the car</p>
        <p>🔋 3. Battery status updated every 2 seconds</p>
        <p>⚠️ 4. Release joystick to stop the car</p>
        <p>🔌 5. Connection Status: <span id="wsStatus">Disconnected</span></p>
        <p>🔘 6. Press MODE button to switch modes</p>
      </div>
      
      <div class="ota-section" id="otaSection">
        <h3>📱 OTA Firmware Update</h3>
        <p>Upload a new firmware (.bin file) to your device:</p>
        
        <div id="otaForm">
          <input type="file" id="firmwareFile" accept=".bin">
          <button class="ota-button" id="uploadButton">Upload Firmware</button>
        </div>
        
        <div class="progress-container" id="progressContainer">
          <div class="progress-bar" id="progressBar"></div>
        </div>
        
        <div class="ota-status" id="otaStatus"></div>
      </div>
      
      <div class="footer">
        ESP32C3 Remote Car | Web OTA Update | Made with ❤️
      </div>
    </div>
    
    <script>
      let websocket;
      let isConnected = false;
      const joystick = document.getElementById('joystick');
      const joystickContainer = document.getElementById('joystickContainer');
      const directionIndicator = document.getElementById('directionIndicator');
      let joystickPos = { x: 0, y: 0 };
      let isDragging = false;
      let firmwareFile = null;
      
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
          // 检查模式更新
          else if(event.data.startsWith("mode:")) {
            const modeName = event.data.substring(5);
            updateModeUI(modeName);
          }
          // OTA进度更新
          else if(event.data.startsWith("ota_progress:")) {
            const progress = parseInt(event.data.substring(13));
            updateProgressBar(progress);
          }
          // OTA完成
          else if(event.data === "ota_complete") {
            document.getElementById('otaStatus').textContent = "Update complete! Rebooting...";
            document.getElementById('otaStatus').style.color = "#2ecc71";
            setTimeout(() => {
              window.location.reload();
            }, 3000);
          }
          // OTA错误
          else if(event.data.startsWith("ota_error:")) {
            const error = event.data.substring(10);
            document.getElementById('otaStatus').textContent = "Error: " + error;
            document.getElementById('otaStatus').style.color = "#e74c3c";
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
      
      // 更新模式UI
      function updateModeUI(modeName) {
        document.getElementById('modeValue').textContent = modeName;
        document.getElementById('subtitle').textContent = modeName + " Mode";
        
        // 重置所有模式指示器
        document.getElementById('modeMobile').className = 'mode-dot';
        document.getElementById('modeJoystick').className = 'mode-dot';
        document.getElementById('modeOta').className = 'mode-dot';
        
        // 激活当前模式
        if(modeName === "Mobile") {
          document.getElementById('modeMobile').className = 'mode-dot active';
          document.getElementById('joystickContainer').style.display = 'block';
        }
        else if(modeName === "Joystick") {
          document.getElementById('modeJoystick').className = 'mode-dot active';
          document.getElementById('joystickContainer').style.display = 'none';
        }
        else if(modeName === "OTA") {
          document.getElementById('modeOta').className = 'mode-dot active';
          document.getElementById('joystickContainer').style.display = 'none';
        }
      }
      
      // 更新OTA进度条
      function updateProgressBar(progress) {
        const progressBar = document.getElementById('progressBar');
        progressBar.style.width = progress + '%';
        document.getElementById('otaStatus').textContent = `Updating: ${progress}%`;
        document.getElementById('otaStatus').style.color = "#3498db";
      }
      
      // 发送控制命令
      function sendCommand(x, y) {
        if (isConnected) {
          const command = `${x},${y}`;
          websocket.send(command);
          
          document.getElementById('steeringValue').textContent = 
            Math.round(mapRange(x, -100, 100, 0, 180)) + '°';
            
          document.getElementById('speedValue').textContent = 
            Math.round(Math.abs(y)) + '%';
            
          updateDirectionIndicator(x, y);
        }
      }
      
      // 更新方向指示器
      function updateDirectionIndicator(x, y) {
        let directionSymbol = "●";
        
        if (y > 50) directionSymbol = "↑";
        else if (y < -50) directionSymbol = "↓";
        else if (x > 50) directionSymbol = "→";
        else if (x < -50) directionSymbol = "←";
        else if (y > 10) directionSymbol = "↗";
        else if (y < -10) directionSymbol = "↘";
        else if (x > 10 && y > 10) directionSymbol = "↖";
        else if (x < -10 && y < -10) directionSymbol = "↙";
        
        directionIndicator.textContent = directionSymbol;
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
        directionIndicator.textContent = "●";
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
        
        const distance = Math.min(Math.sqrt(x*x + y*y), centerX - 50);
        const angle = Math.atan2(y, x);
        
        joystickPos.x = Math.cos(angle) * distance;
        joystickPos.y = Math.sin(angle) * distance;
        
        joystick.style.transform = `translate(${joystickPos.x}px, ${joystickPos.y}px)`;
        
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
        
        // OTA文件上传
        document.getElementById('firmwareFile').addEventListener('change', function(e) {
          firmwareFile = e.target.files[0];
        });
        
        document.getElementById('uploadButton').addEventListener('click', function() {
          if (!firmwareFile) {
            alert("Please select a firmware file first!");
            return;
          }
          
          if (!firmwareFile.name.endsWith('.bin')) {
            alert("Please select a valid .bin file!");
            return;
          }
          
          // 显示进度条
          document.getElementById('progressContainer').style.display = 'block';
          document.getElementById('otaStatus').textContent = "Starting update...";
          document.getElementById('otaStatus').style.color = "#3498db";
          
          // 读取并发送文件
          const reader = new FileReader();
          reader.onload = function(e) {
            const arrayBuffer = e.target.result;
            const firmwareData = new Uint8Array(arrayBuffer);
            
            // 通过WebSocket发送固件数据
            websocket.send("ota_start:" + firmwareData.length);
            
            // 分块发送固件数据
            const chunkSize = 1024;
            for (let offset = 0; offset < firmwareData.length; offset += chunkSize) {
              const chunk = firmwareData.slice(offset, offset + chunkSize);
              websocket.send(chunk);
              
              // 更新进度
              const progress = Math.round((offset / firmwareData.length) * 100);
              document.getElementById('progressBar').style.width = progress + '%';
              document.getElementById('otaStatus').textContent = `Uploading: ${progress}%`;
            }
            
            // 发送结束信号
            websocket.send("ota_end");
          };
          
          reader.readAsArrayBuffer(firmwareFile);
        });
      }
      
      // 页面加载时初始化
      window.onload = function() {
        initWebSocket();
        initControls();
        updateModeUI("Mobile");
      };
    </script>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}

// OTA处理
void handleOTA() {
  server.send(200, "text/html", "OTA page");
}

// 固件上传处理
void handleUpdate() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    otaInProgress = true;
    otaStartTime = millis();
    Serial.printf("Update: %s\n", upload.filename.c_str());
    
    // 初始化更新
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      server.send(500, "text/plain", "UPDATE_BEGIN_FAILED");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // 写入接收到的数据
    if (Update.write(upload.buf, upload.currentSize)) {
      // 计算并发送进度
      int progress = (upload.totalSize > 0) ? (upload.totalSize * 100 / (upload.totalSize + 1)) : 0;
      webSocket.broadcastTXT("ota_progress:" + String(progress));
      Serial.printf("Progress: %d%%\n", progress);
    } else {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    // 完成更新
    if (Update.end(true)) {
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      webSocket.broadcastTXT("ota_complete");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      webSocket.broadcastTXT("ota_error:UPDATE_END_FAILED");
    }
  }
}

// 设置电机速度
void setMotorSpeed(int speed) {
  if (speed > 0) {  
    ledcWrite(0, speed);
    ledcWrite(1, 0);
    currentSpeed = speed;
  } else if (speed < 0) {  
    ledcWrite(0, 0);
    ledcWrite(1, -speed);
    currentSpeed = -speed;
  } else {  
    stopAllMotors();
  }
}

// 停止所有电机
void stopAllMotors() {
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  currentSpeed = 0;
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
void switchMode(OperationMode newMode) {
  if(currentMode == newMode) return;
  
  stopAllMotors();
  
  Serial.print("Switching to mode: ");
  String modeName = "";
  switch(newMode) {
    case MODE_MOBILE: 
      Serial.println("Mobile");
      modeName = "Mobile";
      break;
    case MODE_JOYSTICK: 
      Serial.println("Joystick");
      modeName = "Joystick";
      break;
    case MODE_OTA: 
      Serial.println("OTA");
      modeName = "OTA";
      break;
  }
  
  cleanupCurrentMode();
  currentMode = newMode;
  
  switch(newMode) {
    case MODE_MOBILE: initMobileMode(); break;
    case MODE_JOYSTICK: initJoystickMode(); break;
    case MODE_OTA: initOTAMode(); break;
  }
  
  leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
  FastLED.show();
  
  if(currentMode == MODE_MOBILE) {
    webSocket.broadcastTXT("mode:" + modeName);
  }
}

// 初始化手机模式
void initMobileMode() {
  WiFi.softAP(apSsid, apPassword);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", handleRoot);
  server.on("/ota", handleOTA);
  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
  }, handleUpdate);
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.begin();
  
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
void initOTAMode() {
  WiFi.softAP(apSsid, apPassword);
  Serial.print("AP IP for OTA: ");
  Serial.println(WiFi.softAPIP());
  
  Serial.println("OTA mode activated");
}

// 清理当前模式资源
void cleanupCurrentMode() {
  switch(currentMode) {
    case MODE_MOBILE:
      webSocket.close();
      server.stop();
      WiFi.softAPdisconnect(true);
      break;
      
    case MODE_JOYSTICK:
      esp_now_deinit();
      WiFi.mode(WIFI_OFF);
      break;
      
    case MODE_OTA:
      // 不需要特殊清理
      break;
  }
}

// 电源管理任务
void PowerTask(void *pvParameters) {
  static uint16_t count = 0;
  while (1) {
    if (!digitalRead(POWER_KEY_PIN)) {
      count++;
      if (count > 1500) {
        digitalWrite(POWER_PIN, LOW);
        leds[0] = CHSV(HUE_YELLOW, 255, 0);
        FastLED.show();
      }
    } else {
      count = 0;
    }
    
    handleButton();
    
    unsigned long currentMillis = millis();
    if (currentMillis - lastBatteryUpdate >= batteryUpdateInterval) {
      updateBatteryInfo();
      
      if(currentMode != MODE_OTA) {
        if (batteryVoltage < 3.5f) {
          leds[0] = CHSV(HUE_RED, 255, ledBrightness[currentMode]);
        } else {
          leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
        }
        FastLED.show();
      }
      
      lastBatteryUpdate = currentMillis;
    }
    
    delay(1);
  }
}

void setup() {
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  Serial.begin(115200);
  
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  
  myservo.setPeriodHertz(50);
  myservo.attach(STEERING_PIN, 2, 1000, 2000);
  myservo.writeMicroseconds(servoCenter); 
  
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_A_PWM, 0);
  ledcSetup(1, 5000, 8);
  ledcAttachPin(MOTOR_B_PWM, 1);

  pinMode(CAR_SLEEP_PIN, OUTPUT);
  digitalWrite(CAR_SLEEP_PIN, HIGH);

  pinMode(POWER_KEY_PIN, INPUT);
  
  analogReadResolution(12);
  updateBatteryInfo();

  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS); 
  leds[0] = CHSV(HUE_BLUE, 255, ledBrightness[currentMode]);
  FastLED.show();

  switchMode(currentMode);
  
  xTaskCreate(PowerTask, "PowerTask", 4096, NULL, 1, NULL);
}

void loop() {
  switch(currentMode) {
    case MODE_MOBILE:
      webSocket.loop();
      server.handleClient();
      break;
      
    case MODE_JOYSTICK:
      delay(10);
      break;
      
    case MODE_OTA:
      // OTA模式无需额外处理
      delay(100);
      break;
  }
  
  // 处理OTA超时
  if (otaInProgress && (millis() - otaStartTime > 120000)) { // 2分钟超时
    otaInProgress = false;
    webSocket.broadcastTXT("ota_error:TIMEOUT");
    Serial.println("OTA update timed out");
  }
}
#endif


#if 0

#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)
int ledState = LOW;  // ledState used to set the LED


#define MOTOR_A_PWM 6
#define MOTOR_B_PWM 7
#define STEERING_PIN 3
#define CAR_SLEEP_PIN 5
#define POWER_KEY_PIN 1
#define BATTERY_PIN 0
#define POWER_PIN 10
#define RGB_PIN 4
#define MODE_SWITCH_PIN 9


AsyncWebServer serverOta(80);

const char update_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Firmware Update</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 50px;
        }
        .upload-container {
            max-width: 400px;
            margin: 0 auto;
            text-align: center;
        }
        .upload-btn {
            margin-top: 20px;
            padding: 10px 20px;
            background-color: #007bff;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        .upload-btn:hover {
            background-color: #0056b3;
        }
    </style>
</head>
<body>
    <div class="upload-container">
        <h1>ESP32 固件更新123</h1>
        <input type="file" id="firmwareFile" accept=".bin">
        <button class="upload-btn" onclick="uploadFirmware()">上传固件</button>
        <p id="status"></p>
    </div>

    <script>
        function uploadFirmware() {
            const fileInput = document.getElementById('firmwareFile');
            const status = document.getElementById('status');

            if (fileInput.files.length === 0) {
                status.textContent = "Please select a firmware file.";
                return;
            }

            const file = fileInput.files[0];
            const formData = new FormData();
            formData.append("firmware", file);

            status.textContent = "上传中...";

            fetch("/upload-firmware", {
                method: "POST",
                body: formData
            })
            .then(response => {
                if (response.ok) {
                    status.textContent = "上传成功!";
                } else {
                    status.textContent = "上传失败.";
                }
            })
            .catch(error => {
                status.textContent = "Error: " + error.message;
            });
        }
    </script>
</body>
</html>
)rawliteral";

/* setup function */
void setup(void)
{
    // pinMode(led,  OUTPUT);
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    Serial.begin(115200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("esp32");
    Serial.println(WiFi.softAPIP());

    serverOta.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {//web服务接收
        request->send(200, "text/html", update_html);
    });
    serverOta.on("/upload-firmware", HTTP_POST, [](AsyncWebServerRequest *request){        //OTA升级
        request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if(!index){
        Serial.printf("Update Start: %s\n", filename.c_str());
        Update.begin(UPDATE_SIZE_UNKNOWN);
        }
        if(Update.write(data, len) != len){
        Update.printError(Serial);
        }
        if(final){
        if(Update.end(true)){
            Serial.printf("Update Success: %uB\n", index+len);
        } else {
            Update.printError(Serial);
        }
        }
    });

    serverOta.begin();
}

void loop(void)
{
    // serverOta.handleClient();
    delay(1);

    //loop to blink without delay
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        // save the last time you blinked the LED
        previousMillis = currentMillis;
        // if the LED is off turn it on and vice-versa:
        ledState = not(ledState);
        // set the LED with the ledState of the variable:
        // digitalWrite(led,  ledState);
    }

}

#endif




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