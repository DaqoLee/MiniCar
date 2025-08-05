#if 1
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESP32PWM.h>
#include <Servo16.h>
#include <FastLED.h>
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
CRGB leds[NUM_LEDS];
// WiFi设置
const char* ssid = "ESP32C3-Car";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// 舵机参数
const int servoMin = 1050;   // 0度对应的脉冲宽度(us)
const int servoMax = 1650;  // 180度对应的脉冲宽度(us)
const int servoCenter = 1350; // 中间位置(90�?)
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
void setup() {

  pinMode(POWER_PIN,OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  Serial.begin(115200);
  
  // 设置舵机PWM
  // ledcSetup(0, 50, 16); // 通道0, 50Hz, 16位分辨率
  // ledcAttachPin(STEERING_PIN, 0);
  // ledcWrite(0, servoCenter); // 初始位置居中

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
  leds[0] = CHSV(HUE_YELLOW, 255, 30);
  FastLED.show();
  // 设置WiFi AP
  WiFi.softAP(ssid, password);
  
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // 设置服务器路�?
  server.on("/", handleRoot);
  
  // 启动WebSocket服务�?
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // 启动HTTP服务�?
  server.begin();
  Serial.println("HTTP and WebSocket servers started");

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
      lastBatteryUpdate = currentMillis;
    }

  }
  
}

void loop() {
  webSocket.loop();
  server.handleClient();


 
}

#endif