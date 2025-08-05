#if 1

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESP32PWM.h>
#include <Servo16.h>
#include <FastLED.h>
// 寮曡剼瀹氫箟
#define MOTOR_A_PWM 6   // 鍚庤疆鐢垫満PWM1
#define MOTOR_B_PWM 7   // 鍚庤疆鐢垫満PWM2
#define STEERING_PIN 3  // 鑸垫満鎺у埗寮曡剼
#define CAR_SLEEP_PIN 5 
#define POWER_KEY_PIN 1
#define BATTERY_PIN 0   // 鐢垫睜鐢靛帇妫€娴嬪紩鑴� (IO0)
#define POWER_PIN 10
#define RGB_PIN 4

#define NUM_LEDS 1
CRGB leds[NUM_LEDS];
// WiFi璁剧疆
const char* ssid = "ESP32C3-Car";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// 鑸垫満鍙傛暟
const int servoMin = 1050;   // 0搴﹀搴旂殑鑴夊啿瀹藉害(us)
const int servoMax = 1650;  // 180搴﹀搴旂殑鑴夊啿瀹藉害(us)
const int servoCenter = 1350; // 涓棿浣嶇疆(90锟�?)
int currentSteering = servoCenter;
Servo myservo; 
// 鐢垫満閫熷害鎺у埗
int currentSpeed = 0;
const int maxSpeed = 255;

void PowerTask(void *pvParameters);
// 璇诲彇鐢垫睜鐢靛帇

// 鐢垫睜鍙傛暟
float batteryVoltage = 0.0;
float batteryPercentage = 0.0;
const float maxBatteryVoltage = 4.2; // 2S閿傜數姹犳弧鐢电數鍘�(8.4V)
const float minBatteryVoltage = 3.0; // 2S閿傜數姹犳渶浣庣數鍘�(6.0V)
const float voltageDividerRatio = 2.0; // 鍒嗗帇姣� (涓や釜100K鐢甸樆鍒嗗帇)
unsigned long lastBatteryUpdate = 0;
const long batteryUpdateInterval = 2000; // 姣�2绉掓洿鏂颁竴娆＄數姹犱俊鎭�

void updateBatteryInfo() {
  // 璇诲彇ADC鍊硷紙0-4095锛�
  int rawValue = analogRead(BATTERY_PIN);
  
  // 璁＄畻瀹為檯鐢靛帇锛堝垎鍘嬫瘮2:1锛孉DC鍙傝€冪數鍘�3.3V锛�
  batteryVoltage = (((float)rawValue / 4095.0) * 3.3 * voltageDividerRatio) - 0.4;
  
  // 璁＄畻鐢甸噺鐧惧垎姣旓紙绾挎€т及绠楋級
  batteryPercentage = map(constrain(batteryVoltage*100, minBatteryVoltage*100, maxBatteryVoltage*100), 
                          minBatteryVoltage*100, maxBatteryVoltage*100, 0, 100);
  
  // 骞挎挱鐢垫睜淇℃伅缁欐墍鏈夊鎴风
  String batteryInfo = "battery:" + String(batteryVoltage, 1) + "," + String(batteryPercentage, 0);
  webSocket.broadcastTXT(batteryInfo);
  
  Serial.printf("Battery: %.1fV (%d%%)\n", batteryVoltage, (int)batteryPercentage);
}
// WebSocket浜嬩欢澶勭悊
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
      
      // 瑙ｆ瀽鎽囨潌鏁版嵁: "x,y" 鏍煎紡
      char *token = strtok((char *)payload, ",");
      if(token != NULL) {
        int x = atoi(token);
        token = strtok(NULL, ",");
        if(token != NULL) {
          int y = atoi(token);
          
          // 鎺у埗鑸垫満鏂瑰悜 (x锟�?: -100锟�?100)
          int steering = map(-x, -100, 100, servoMin, servoMax);
          steering = constrain(steering, servoMin, servoMax);
         	myservo.writeMicroseconds(steering);
          currentSteering = steering;
           Serial.printf("%d  steering :%d\n", x, steering);
          
          // 鎺у埗鐢垫満閫熷害 (y锟�?: -100锟�?100)
          int speed = map(abs(y), 0, 100, 0, maxSpeed);
          speed = constrain(speed, 0, maxSpeed);
          
          if(y > 10) { // 鍓嶈繘
            ledcWrite(0, speed);  // 閫氶亾1鐢ㄤ簬鐢垫満A
            ledcWrite(1, 0);      // 閫氶亾2鐢ㄤ簬鐢垫満B
            currentSpeed = speed;
            webSocket.sendTXT(num, "Moving Forward");
          } 
          else if(y < -10) { // 鍚庨€€
            ledcWrite(0, 0);
            ledcWrite(1, speed);
            currentSpeed = -speed;
            webSocket.sendTXT(num, "Moving Backward");
          } 
          else { // 鍋滄
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
// 鎻愪緵Web鐣岄潰
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
          <div class="status-value" id="steeringValue">90掳</div>
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
        <h3>馃搵 Instructions:</h3>
        <p>馃摱 1. Connect to WiFi: <strong>ESP32C3-Car</strong> (Password: 12345678)</p>
        <p>馃暪锟�? 2. Drag the joystick to control the car</p>
        <p>馃攲 3. Connection Status: <span id="wsStatus">Disconnected</span></p>
        <p>鈿狅笍 4. Release joystick to stop the car</p>
      </div>
      
      <div class="footer">
        ESP32C3 Joystick Control | WebSocket Real-time | Made with 鉂わ笍
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
      
      // 鍒濆鍖朩ebSocket杩炴帴
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
      
      // 鍙戦€佹帶鍒跺懡锟�?
      function sendCommand(x, y) {
        if (isConnected) {
          const command = `${x},${y}`;
          websocket.send(command);
          
          // 鏇存柊UI
          document.getElementById('steeringValue').textContent = 
            Math.round(mapRange(x, -200, 200, 0, 180)) + '掳';
            
          document.getElementById('speedValue').textContent = 
            Math.round(Math.abs(y)) + '%';
            
          // 鏇存柊鏂瑰悜鐩樿锟�?
          const steeringAngle = mapRange(x, -200, 200, -180, 180);
          steeringWheel.style.transform = `rotate(${steeringAngle}deg)`;
        }
      }
      
      // 鏄犲皠鑼冨洿鍑芥暟
      function mapRange(value, inMin, inMax, outMin, outMax) {
        return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
      }
      
      // 澶勭悊鎽囨潌鎷栧姩
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
        
        // 閲嶇疆UI
        document.getElementById('steeringValue').textContent = '90掳';
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
        
        // 闄愬埗鍦ㄥ渾褰㈣寖鍥村唴
        const distance = Math.min(Math.sqrt(x*x + y*y), centerX - 50);
        const angle = Math.atan2(y, x);
        
        joystickPos.x = Math.cos(angle) * distance;
        joystickPos.y = Math.sin(angle) * distance;
        
        // 鏇存柊鎽囨潌浣嶇疆
        joystick.style.transform = `translate(${joystickPos.x}px, ${joystickPos.y}px)`;
        
        // 杞崲涓虹櫨鍒嗘瘮 (-100锟�?100)
        const xPercent = Math.round(mapRange(joystickPos.x, -centerX + 50, centerX - 50, -200, 200));
        const yPercent = Math.round(mapRange(joystickPos.y, -centerY + 50, centerY - 50, -100, 100));
        
        sendCommand(xPercent, yPercent);
      }
      
      // 鍒濆鍖栦簨浠剁洃鍚櫒
      function initControls() {
        // 榧犳爣浜嬩欢
        joystick.addEventListener('mousedown', startDrag);
        document.addEventListener('mousemove', updateJoystick);
        document.addEventListener('mouseup', stopDrag);
        
        // 瑙︽懜浜嬩欢
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
      
      // 椤甸潰鍔犺浇鏃跺垵濮嬪寲
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
      
      <!-- 鐢垫睜鐢甸噺鏄剧ず -->
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
          <div class="status-value" id="steeringValue">90掳</div>
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
        <div class="direction-indicator" id="directionIndicator">鈼�</div>
      </div>
      
      <div class="instructions">
        <h3>馃搵 Instructions:</h3>
        <p>馃摱 1. Connect to WiFi: <strong>ESP32C3-Car</strong> (Password: 12345678)</p>
        <p>馃暪锔� 2. Drag the joystick to control the car</p>
        <p>馃攱 3. Battery status updated every 2 seconds</p>
        <p>鈿狅笍 4. Release joystick to stop the car</p>
        <p>馃攲 5. Connection Status: <span id="wsStatus">Disconnected</span></p>
      </div>
      
      <div class="footer">
        ESP32C3 Joystick Control | Battery Monitoring | Made with 鉂わ笍
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
      
      // 鍒濆鍖朩ebSocket杩炴帴
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
          
          // 妫€鏌ユ槸鍚︽槸鐢垫睜淇℃伅
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
      
      // 鏇存柊鐢垫睜UI
      function updateBatteryUI(voltage, percentage) {
        document.getElementById('batteryVoltage').textContent = voltage.toFixed(1) + 'V';
        document.getElementById('batteryPercentage').textContent = percentage.toFixed(0) + '%';
        document.getElementById('batteryValue').textContent = percentage.toFixed(0) + '%';
        
        const batteryFill = document.getElementById('batteryFill');
        batteryFill.style.width = percentage + '%';
        
        // 鏍规嵁鐢甸噺鏀瑰彉棰滆壊
        if(percentage > 60) {
          batteryFill.style.background = 'linear-gradient(90deg, #2ecc71, #2ecc71)';
        } else if(percentage > 30) {
          batteryFill.style.background = 'linear-gradient(90deg, #f39c12, #f39c12)';
        } else {
          batteryFill.style.background = 'linear-gradient(90deg, #e74c3c, #e74c3c)';
        }
      }
      
      // 鍙戦€佹帶鍒跺懡浠�
      function sendCommand(x, y) {
        if (isConnected) {
          const command = `${x},${y}`;
          websocket.send(command);
          
          // 鏇存柊UI
          document.getElementById('steeringValue').textContent = 
            Math.round(mapRange(x, -100, 100, 0, 180)) + '掳';
            
          document.getElementById('speedValue').textContent = 
            Math.round(Math.abs(y)) + '%';
            
          // 鏇存柊鏂瑰悜鎸囩ず鍣�
          updateDirectionIndicator(x, y);
        }
      }
      
      // 鏇存柊鏂瑰悜鎸囩ず鍣�
      function updateDirectionIndicator(x, y) {
        let directionSymbol = "鈼�"; // 榛樿鍋滄
        
        if (y > 50) {
          directionSymbol = "鈫�"; // 鍓嶈繘
        } else if (y < -50) {
          directionSymbol = "鈫�"; // 鍚庨€€
        } else if (x > 50) {
          directionSymbol = "鈫�"; // 鍙宠浆
        } else if (x < -50) {
          directionSymbol = "鈫�"; // 宸﹁浆
        } else if (y > 10) {
          directionSymbol = "鈫�"; // 鍓嶈繘+鍙宠浆
        } else if (y < -10) {
          directionSymbol = "鈫�"; // 鍚庨€€+鍙宠浆
        } else if (x > 10 && y > 10) {
          directionSymbol = "鈫�"; // 鍓嶈繘+宸﹁浆
        } else if (x < -10 && y < -10) {
          directionSymbol = "鈫�"; // 鍚庨€€+宸﹁浆
        }
        
        directionIndicator.textContent = directionSymbol;
      }
      
      // 鏄犲皠鑼冨洿鍑芥暟
      function mapRange(value, inMin, inMax, outMin, outMax) {
        return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
      }
      
      // 澶勭悊鎽囨潌鎷栧姩
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
        
        // 閲嶇疆UI
        document.getElementById('steeringValue').textContent = '90掳';
        document.getElementById('speedValue').textContent = '0%';
        directionIndicator.textContent = "鈼�"; // 閲嶇疆鏂瑰悜鎸囩ず鍣�
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
        
        // 闄愬埗鍦ㄥ渾褰㈣寖鍥村唴
        const distance = Math.min(Math.sqrt(x*x + y*y), centerX - 50);
        const angle = Math.atan2(y, x);
        
        joystickPos.x = Math.cos(angle) * distance;
        joystickPos.y = Math.sin(angle) * distance;
        
        // 鏇存柊鎽囨潌浣嶇疆
        joystick.style.transform = `translate(${joystickPos.x}px, ${joystickPos.y}px)`;
        
        // 杞崲涓虹櫨鍒嗘瘮 (-100鍒�100)
        const xPercent = Math.round(mapRange(joystickPos.x, -centerX + 50, centerX - 50, -100, 100));
        const yPercent = Math.round(mapRange(joystickPos.y, -centerY + 50, centerY - 50, -100, 100));
        
        sendCommand(xPercent, yPercent);
      }
      
      // 鍒濆鍖栦簨浠剁洃鍚櫒
      function initControls() {
        // 榧犳爣浜嬩欢
        joystick.addEventListener('mousedown', startDrag);
        document.addEventListener('mousemove', updateJoystick);
        document.addEventListener('mouseup', stopDrag);
        
        // 瑙︽懜浜嬩欢
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
      
      // 椤甸潰鍔犺浇鏃跺垵濮嬪寲
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
      
      <!-- 鐢垫睜鐢甸噺鏄剧ず -->
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
          <div class="status-value" id="steeringValue">90掳</div>
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
        <h3>馃搵 Instructions:</h3>
        <p>馃摱 1. Connect to WiFi: <strong>ESP32C3-Car</strong> (Password: 12345678)</p>
        <p>馃暪锔� 2. Drag the joystick to control the car</p>
        <p>馃攱 3. Battery status is updated every 2 seconds</p>
        <p>鈿狅笍 4. Release joystick to stop the car</p>
        <p>馃攲 5. Connection Status: <span id="wsStatus">Disconnected</span></p>
      </div>
      
      <div class="footer">
        ESP32C3 Joystick Control | Battery Monitoring | Made with 鉂わ笍
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
      
      // 鍒濆鍖朩ebSocket杩炴帴
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
          
          // 妫€鏌ユ槸鍚︽槸鐢垫睜淇℃伅
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
      
      // 鏇存柊鐢垫睜UI
      function updateBatteryUI(voltage, percentage) {
        document.getElementById('batteryVoltage').textContent = voltage.toFixed(1) + 'V';
        document.getElementById('batteryPercentage').textContent = percentage.toFixed(0) + '%';
        document.getElementById('batteryValue').textContent = percentage.toFixed(0) + '%';
        
        const batteryFill = document.getElementById('batteryFill');
        batteryFill.style.width = percentage + '%';
        
        // 鏍规嵁鐢甸噺鏀瑰彉棰滆壊
        if(percentage > 60) {
          batteryFill.style.background = 'linear-gradient(90deg, #2ecc71, #2ecc71)';
        } else if(percentage > 30) {
          batteryFill.style.background = 'linear-gradient(90deg, #f39c12, #f39c12)';
        } else {
          batteryFill.style.background = 'linear-gradient(90deg, #e74c3c, #e74c3c)';
        }
      }
      
      // 鍙戦€佹帶鍒跺懡浠�
      function sendCommand(x, y) {
        if (isConnected) {
          const command = `${x},${y}`;
          websocket.send(command);
          
          // 鏇存柊UI
          document.getElementById('steeringValue').textContent = 
            Math.round(mapRange(x, -100, 100, 0, 180)) + '掳';
            
          document.getElementById('speedValue').textContent = 
            Math.round(Math.abs(y)) + '%';
            
          // 鏇存柊鏂瑰悜鐩樿搴�
          const steeringAngle = mapRange(x, -100, 100, -180, 180);
          steeringWheel.style.transform = `rotate(${steeringAngle}deg)`;
        }
      }
      
      // 鏄犲皠鑼冨洿鍑芥暟
      function mapRange(value, inMin, inMax, outMin, outMax) {
        return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
      }
      
      // 澶勭悊鎽囨潌鎷栧姩
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
        
        // 閲嶇疆UI
        document.getElementById('steeringValue').textContent = '90掳';
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
        
        // 闄愬埗鍦ㄥ渾褰㈣寖鍥村唴
        const distance = Math.min(Math.sqrt(x*x + y*y), centerX - 50);
        const angle = Math.atan2(y, x);
        
        joystickPos.x = Math.cos(angle) * distance;
        joystickPos.y = Math.sin(angle) * distance;
        
        // 鏇存柊鎽囨潌浣嶇疆
        joystick.style.transform = `translate(${joystickPos.x}px, ${joystickPos.y}px)`;
        
        // 杞崲涓虹櫨鍒嗘瘮 (-100鍒�100)
        const xPercent = Math.round(mapRange(joystickPos.x, -centerX + 50, centerX - 50, -100, 100));
        const yPercent = Math.round(mapRange(joystickPos.y, -centerY + 50, centerY - 50, -100, 100));
        
        sendCommand(xPercent, yPercent);
      }
      
      // 鍒濆鍖栦簨浠剁洃鍚櫒
      function initControls() {
        // 榧犳爣浜嬩欢
        joystick.addEventListener('mousedown', startDrag);
        document.addEventListener('mousemove', updateJoystick);
        document.addEventListener('mouseup', stopDrag);
        
        // 瑙︽懜浜嬩欢
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
      
      // 椤甸潰鍔犺浇鏃跺垵濮嬪寲
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
  
  // 璁剧疆鑸垫満PWM
  // ledcSetup(0, 50, 16); // 閫氶亾0, 50Hz, 16浣嶅垎杈ㄧ巼
  // ledcAttachPin(STEERING_PIN, 0);
  // ledcWrite(0, servoCenter); // 鍒濆浣嶇疆灞呬腑

	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(STEERING_PIN, 2, 1000, 2000); // attaches the servo on pin 18 to the servo object
	myservo.writeMicroseconds(servoCenter); 
  
  // 璁剧疆鐢垫満PWM
  ledcSetup(0, 5000, 8); // 閫氶亾1, 5kHz, 8浣嶅垎杈ㄧ巼
  ledcAttachPin(MOTOR_A_PWM, 0);
  ledcSetup(1, 5000, 8); // 閫氶亾2, 5kHz, 8浣嶅垎杈ㄧ巼
  ledcAttachPin(MOTOR_B_PWM, 1);

  pinMode(CAR_SLEEP_PIN,OUTPUT);
  digitalWrite(CAR_SLEEP_PIN, HIGH);

  pinMode(POWER_KEY_PIN, INPUT);
  
  analogReadResolution(12); // 12浣嶅垎杈ㄧ巼 (0-4095)
  // pinMode(BATTERY_PIN, INPUT);
  updateBatteryInfo();

  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS); 
/*
    HUE_RED = 0,       ///< Red (0掳)
    HUE_ORANGE = 32,   ///< Orange (45掳)
    HUE_YELLOW = 64,   ///< Yellow (90掳)
    HUE_GREEN = 96,    ///< Green (135掳)
    HUE_AQUA = 128,    ///< Aqua (180掳)
    HUE_BLUE = 160,    ///< Blue (225掳)
    HUE_PURPLE = 192,  ///< Purple (270掳)
    HUE_PINK = 224     ///< Pink (315掳)
*/
  leds[0] = CHSV(HUE_YELLOW, 255, 30);
  FastLED.show();
  // 璁剧疆WiFi AP
  WiFi.softAP(ssid, password);
  
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // 璁剧疆鏈嶅姟鍣ㄨ矾锟�?
  server.on("/", handleRoot);
  
  // 鍚姩WebSocket鏈嶅姟锟�?
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // 鍚姩HTTP鏈嶅姟锟�?
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

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Servo16.h>
// #include <ESP32Servo.h>
const int servoCenter = 1350;
// 鐢垫満鍜岃埖鏈哄紩鑴氬畾涔�
#define MOTOR_PWM_A 6   // 鍚庤疆鐢垫満A PWM
#define MOTOR_PWM_B 7   // 鍚庤疆鐢垫満B PWM
#define STEERING_PIN 3  // 鍓嶈疆鑸垫満

// 鐢垫満PWM閰嶇疆
#define PWM_FREQ 5000
#define PWM_RES 8       // 8浣嶅垎杈ㄧ巼锛�0-255锛�

// 鑸垫満瀵硅薄
// Servo steeringServo;
Servo myservo; 
// 鏁版嵁缁撴瀯锛堝繀椤讳笌鍙戦€佺涓€鑷达級
typedef struct struct_message {
  int16_t joy1X;
  int16_t joy1Y;
  int16_t joy2X;
  int16_t joy2Y;
} struct_message;
struct_message rxData;

// 鐢垫満鎺у埗鍑芥暟
void setMotorSpeed(int speed) {
  if (speed > 0) {  // 鍓嶈繘
    ledcWrite(0, speed);
    ledcWrite(1, 0);
  } else if (speed < 0) {  // 鍚庨€€
    ledcWrite(0, 0);
    ledcWrite(1, -speed);
  } else {  // 鍋滄
    ledcWrite(0, 0);
    ledcWrite(1, 0);
  }
}

// 鎺ユ敹鏁版嵁鍥炶皟
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  memcpy(&rxData, data, sizeof(rxData));
  
  // 鎺у埗鑸垫満锛氭憞鏉�1X杞� -> 鑸垫満瑙掑害锛堟槧灏勫埌0-180掳锛�
  int servoAngle = map(rxData.joy2Y, 0, 4095, 1150, 1550);
  // steeringServo.write(servoAngle);
  myservo.writeMicroseconds(servoAngle); 
  // 鎺у埗鐢垫満锛氭憞鏉�1Y杞� -> 鐢垫満閫熷害锛堟槧灏勫埌-255~255锛�
  int motorSpeed = map(rxData.joy1X, 0, 4095, 255, -255);

  setMotorSpeed(motorSpeed);
  Serial.print("angle:");
  Serial.print(servoAngle);
  Serial.print("speed:");
  Serial.println(motorSpeed);
}

void setup() {

  delay(1000);
  pinMode(10,OUTPUT);
  digitalWrite(10,HIGH);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // 鍒濆鍖栫數鏈篜WM
  ledcSetup(0, PWM_FREQ, PWM_RES);  // 閫氶亾0 - 鐢垫満A
  ledcSetup(1, PWM_FREQ, PWM_RES);  // 閫氶亾1 - 鐢垫満B
  ledcAttachPin(MOTOR_PWM_A, 0);
  ledcAttachPin(MOTOR_PWM_B, 1);

  pinMode(5,OUTPUT);
  digitalWrite(5,HIGH);

  myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(STEERING_PIN, 2, 1000, 2000); // attaches the servo on pin 18 to the servo object
	myservo.writeMicroseconds(servoCenter); 
  

  // 鍒濆鍖栬埖鏈�
  // steeringServo.attach(STEERING_PIN);
  // steeringServo.write(90);  // 鍒濆灞呬腑浣嶇疆

  // 鍒濆鍖朎SP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW鍒濆鍖栧け璐�");
    return;
  }

  Serial.print("ESP32-C3 MAC Address: ");
  Serial.println(WiFi.macAddress());

  // 娉ㄥ唽鎺ユ敹鍥炶皟
  // esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
}

void loop() {
  // 涓诲惊鐜棤闇€鎿嶄綔锛屾暟鎹湪鍥炶皟涓鐞�
  // Serial.println("ESP32-C3 MAC Address: ");
  delay(100);
}