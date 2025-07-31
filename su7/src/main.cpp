#if 0

#include <Servo16.h>
#include <WiFi.h>
#include <FastLED.h>

Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32

int pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33 
// Possible PWM GPIO pins on the ESP32-S2: 0(used by on-board button),1-17,18(used by on-board LED),19-21,26,33-42
// Possible PWM GPIO pins on the ESP32-S3: 0(used by on-board button),1-21,35-45,47,48(used by on-board LED)
// Possible PWM GPIO pins on the ESP32-C3: 0(used by on-board button),1-7,8(used by on-board LED),9-10,18-21
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
int servoPin = 17;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
int servoPin = 4;
#else
int servoPin = 18;
#endif


String ssid =     "Redmi_8B3F";
String password = "asdfghjkl";
 
#define NUM_LEDS 1

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 3
// #define CLOCK_PIN 13

// Define the array of leds
CRGB leds[NUM_LEDS];
void setup() {

	ledcSetup(0, 10000, 10);
  ledcAttachPin(6, 0);
  ledcWrite(0, 0);

	ledcSetup(1, 10000, 10);
  ledcAttachPin(7, 1);
  ledcWrite(1, 0);

	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 2, 1000, 2000); // attaches the servo on pin 18 to the servo object
	// // using default min/max of 1000us and 2000us
	// different servos may require different min/max settings
	// for an accurate 0 to 180 sweep
	
  Serial.begin(115200);
	FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 
	WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  while (!WiFi.isConnected())
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected");
  Serial.print("IP Address:");
  Serial.println(WiFi.localIP());

	// delay(200);
	// pinMode(10, OUTPUT);
	// digitalWrite(10,HIGH);
	myservo.writeMicroseconds(1350); 
}

void loop() {

	// for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
	// 	// in steps of 1 degree
	// 	myservo.write(pos);    // tell servo to go to position in variable 'pos'
	// 	delay(15);             // waits 15ms for the servo to reach the position
	// }
	// for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
	// 	myservo.write(pos);    // tell servo to go to position in variable 'pos'
	// 	delay(15);             // waits 15ms for the servo to reach the position
	// }

	ledcWrite(0, 500);
	ledcWrite(1, 0);
  myservo.writeMicroseconds(1400); 
	leds[0] = CRGB::Red;
  FastLED.show();
  delay(1000);
  // Now turn the LED off, then pause
	ledcWrite(0, 0);
	ledcWrite(1, 500);
	myservo.writeMicroseconds(1300);
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(1000);

	// Serial.println(WiFi.localIP());


  // ledcWrite(0, 200);
	// ledcWrite(1, 0);
	// delay(2000); 
	// ledcWrite(0, 300);
	// ledcWrite(1, 0);

	// delay(2000); 
	// ledcWrite(0, 0);
	// ledcWrite(1, 200);

	// delay(2000); 
	// ledcWrite(0, 0);
	// ledcWrite(1, 300);
	// delay(2000);

}

#endif

#if 0
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Servo16.h>
// 电机控制引脚定义
#define MOTOR_A_IN1 5
#define MOTOR_A_IN2 6
#define MOTOR_B_IN3 7
#define MOTOR_B_IN4 8
int servoPin = 4;
// WiFi设置
const char* ssid = "ESP32C3-Car";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
Servo myservo; 
// 控制小车移动的函数
void moveForward() {
	ledcWrite(0, 0);
	ledcWrite(1, 500);
}

void moveBackward() {
	ledcWrite(0, 500);
	ledcWrite(1, 0);
}

void turnLeft() {
	ledcWrite(0, 0);
	ledcWrite(1, 500);
	myservo.writeMicroseconds(1450); 
}

void turnRight() {
	ledcWrite(0, 0);
	ledcWrite(1, 500);
	myservo.writeMicroseconds(1250); 
}

void stopCar() {
	ledcWrite(0, 0);
	ledcWrite(1, 0);
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
      Serial.printf("[%u] Received text: %s\n", num, payload);
      
      if (strcmp((char*)payload, "forward") == 0) {
        moveForward();
        webSocket.sendTXT(num, "Moving Forward");
      } else if (strcmp((char*)payload, "backward") == 0) {
        moveBackward();
        webSocket.sendTXT(num, "Moving Backward");
      } else if (strcmp((char*)payload, "left") == 0) {
        turnLeft();
        webSocket.sendTXT(num, "Turning Left");
      } else if (strcmp((char*)payload, "right") == 0) {
        turnRight();
        webSocket.sendTXT(num, "Turning Right");
      } else if (strcmp((char*)payload, "stop") == 0) {
        stopCar();
        webSocket.sendTXT(num, "Stopped");
      }
      break;
  }
}

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
      body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d);
        text-align: center;
        padding: 20px;
        min-height: 100vh;
        margin: 0;
        color: white;
        overflow: hidden;
      }
      .container {
        max-width: 500px;
        margin: 20px auto;
        background: rgba(0, 0, 0, 0.7);
        padding: 25px;
        border-radius: 20px;
        box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
        backdrop-filter: blur(10px);
      }
      h1 {
        color: #ffffff;
        margin-bottom: 10px;
        text-shadow: 0 0 10px rgba(255, 255, 255, 0.5);
      }
      .subtitle {
        color: #cccccc;
        margin-bottom: 30px;
      }
      .control-panel {
        display: grid;
        grid-template-columns: 1fr 1fr 1fr;
        grid-gap: 15px;
        margin: 30px 0;
      }
      .btn {
        padding: 20px;
        border: none;
        border-radius: 15px;
        font-size: 18px;
        font-weight: bold;
        cursor: pointer;
        transition: all 0.2s;
        color: white;
        box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
        display: flex;
        justify-content: center;
        align-items: center;
        flex-direction: column;
      }
      .btn i {
        font-size: 28px;
        margin-bottom: 5px;
      }
      .btn:active {
        transform: scale(0.95);
        box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
      }
      .forward {
        grid-column: 2;
        background: linear-gradient(145deg, #3498db, #2980b9);
      }
      .backward {
        grid-column: 2;
        background: linear-gradient(145deg, #3498db, #2980b9);
      }
      .left {
        grid-column: 1;
        background: linear-gradient(145deg, #e74c3c, #c0392b);
      }
      .right {
        grid-column: 3;
        background: linear-gradient(145deg, #e74c3c, #c0392b);
      }
      .stop {
        grid-column: 1 / span 3;
        background: linear-gradient(145deg, #2c3e50, #1a2530);
        margin-top: 15px;
        padding: 15px;
      }
      .status {
        margin: 25px 0;
        padding: 15px;
        border-radius: 10px;
        background: rgba(255, 255, 255, 0.1);
        font-size: 18px;
        min-height: 25px;
      }
      .instructions {
        margin-top: 25px;
        text-align: left;
        background: rgba(255, 255, 255, 0.1);
        padding: 20px;
        border-radius: 15px;
        font-size: 14px;
      }
      .connection-status {
        position: absolute;
        top: 15px;
        right: 15px;
        padding: 8px 15px;
        border-radius: 20px;
        font-weight: bold;
        background: #e74c3c;
      }
      .connected {
        background: #2ecc71;
      }
      .speed-control {
        margin: 20px 0;
      }
      .speed-slider {
        width: 100%;
        height: 25px;
        -webkit-appearance: none;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 10px;
        outline: none;
      }
      .speed-slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        width: 35px;
        height: 35px;
        border-radius: 50%;
        background: #3498db;
        cursor: pointer;
        box-shadow: 0 0 10px rgba(52, 152, 219, 0.5);
      }
      .footer {
        margin-top: 20px;
        font-size: 12px;
        color: #aaaaaa;
      }
      @media (max-width: 500px) {
        .container {
          margin: 10px;
          padding: 15px;
        }
        .btn {
          padding: 15px;
          font-size: 16px;
        }
      }
    </style>
  </head>
  <body>
    <div class="connection-status" id="connectionStatus">Disconnected</div>
    
    <div class="container">
      <h1>ESP32C3 SMART CAR</h1>
      <div class="subtitle">WebSocket Control Panel</div>
      
      <div class="control-panel">
        <button class="btn forward" ontouchstart="sendCommand('forward')" ontouchend="sendCommand('stop')">
          ▲ FORWARD
        </button>
        
        <button class="btn left" ontouchstart="sendCommand('left')" ontouchend="sendCommand('stop')">
          ◀ LEFT
        </button>
        
        <button class="btn backward" ontouchstart="sendCommand('backward')" ontouchend="sendCommand('stop')">
          ▼ BACKWARD
        </button>
        
        <button class="btn right" ontouchstart="sendCommand('right')" ontouchend="sendCommand('stop')">
          ▶ RIGHT
        </button>
        
        <button class="btn stop" onclick="sendCommand('stop')">
          ⬛ EMERGENCY STOP
        </button>
      </div>
      
      <div class="speed-control">
        <h3>Speed Control</h3>
        <input type="range" min="1" max="100" value="80" class="speed-slider" id="speedSlider" onchange="updateSpeed()">
        <div>Current Speed: <span id="speedValue">80</span>%</div>
      </div>
      
      <div class="status" id="status">Ready to connect...</div>
      
      <div class="instructions">
        <h3>📋 Instructions:</h3>
        <p>📶 1. Connect to WiFi: <strong>ESP32C3-Car</strong> (Password: 12345678)</p>
        <p>📱 2. Touch and hold direction buttons for movement</p>
        <p>🔌 3. Connection Status: <span id="wsStatus">Disconnected</span></p>
      </div>
      
      <div class="footer">
        ESP32C3 WebSocket Control | Real-time Communication | Made with ❤️
      </div>
    </div>
    
    <script>
      let websocket;
      let isConnected = false;
      
      function initWebSocket() {
        const ip = window.location.hostname;
        websocket = new WebSocket('ws://' + ip + ':81/');
        
        websocket.onopen = function() {
          console.log('WebSocket connected');
          isConnected = true;
          document.getElementById('connectionStatus').textContent = 'Connected';
          document.getElementById('connectionStatus').className = 'connection-status connected';
          document.getElementById('wsStatus').textContent = 'Connected';
          document.getElementById('status').textContent = 'Connected to car controller';
        };
        
        websocket.onclose = function() {
          console.log('WebSocket disconnected');
          isConnected = false;
          document.getElementById('connectionStatus').textContent = 'Disconnected';
          document.getElementById('connectionStatus').className = 'connection-status';
          document.getElementById('wsStatus').textContent = 'Disconnected';
          document.getElementById('status').textContent = 'Connection lost. Reconnecting...';
          setTimeout(initWebSocket, 2000);
        };
        
        websocket.onmessage = function(event) {
          console.log('Received: ' + event.data);
          document.getElementById('status').textContent = event.data;
        };
        
        websocket.onerror = function(error) {
          console.error('WebSocket Error: ', error);
        };
      }
      
      function sendCommand(cmd) {
        if (isConnected) {
          console.log('Sending command: ' + cmd);
          websocket.send(cmd);
        } else {
          document.getElementById('status').textContent = 'Not connected. Trying to reconnect...';
          initWebSocket();
        }
      }
      
      function updateSpeed() {
        const slider = document.getElementById('speedSlider');
        const value = slider.value;
        document.getElementById('speedValue').textContent = value;
        // 在实际应用中，可以将速度值发送给ESP32
        // sendCommand('speed:' + value);
      }
      
      // 页面加载时初始化WebSocket连接
      window.onload = initWebSocket;
    </script>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}

void setup() {
  // 初始化电机控制引脚
  // pinMode(MOTOR_A_IN1, OUTPUT);
  // pinMode(MOTOR_A_IN2, OUTPUT);
  // pinMode(MOTOR_B_IN3, OUTPUT);
  // pinMode(MOTOR_B_IN4, OUTPUT);
  // stopCar(); // 确保小车初始状态为停止

	ledcSetup(0, 10000, 10);
  ledcAttachPin(MOTOR_A_IN2, 0);
  ledcWrite(0, 0);

	ledcSetup(1, 10000, 10);
  ledcAttachPin(MOTOR_B_IN3, 1);
  ledcWrite(1, 0);
  
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 2, 1000, 2000); // attaches the servo on pin 18 to the servo object
	myservo.writeMicroseconds(1350); 
  // 启动串口通信
  Serial.begin(115200);
  
  // 设置ESP32为AP模式
  WiFi.softAP(ssid, password);
  
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // 设置服务器路由
  server.on("/", handleRoot);
  
  // 启动WebSocket服务器
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // 启动HTTP服务器
  server.begin();
  Serial.println("HTTP and WebSocket servers started");
}

void loop() {
  webSocket.loop();
  server.handleClient();
}

#endif

#if 1
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESP32PWM.h>
#include <Servo16.h>
// 引脚定义
#define MOTOR_A_PWM 6   // 后轮电机PWM1
#define MOTOR_B_PWM 7   // 后轮电机PWM2
#define STEERING_PIN 4  // 舵机控制引脚

// WiFi设置
const char* ssid = "ESP32C3-Car";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// 舵机参数
const int servoMin = 1150;   // 0度对应的脉冲宽度(us)
const int servoMax = 1550;  // 180度对应的脉冲宽度(us)
const int servoCenter = 1350; // 中间位置(90度)
int currentSteering = servoCenter;
Servo myservo; 
// 电机速度控制
int currentSpeed = 0;
const int maxSpeed = 255;

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
          
          // 控制舵机方向 (x值: -100到100)
          int steering = map(-x, -200, 200, servoMin, servoMax);
          steering = constrain(steering, servoMin, servoMax);
         	myservo.writeMicroseconds(steering);
          currentSteering = steering;
          
          // 控制电机速度 (y值: -100到100)
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
        <p>🕹️ 2. Drag the joystick to control the car</p>
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
      
      // 发送控制命令
      function sendCommand(x, y) {
        if (isConnected) {
          const command = `${x},${y}`;
          websocket.send(command);
          
          // 更新UI
          document.getElementById('steeringValue').textContent = 
            Math.round(mapRange(x, -200, 200, 0, 180)) + '°';
            
          document.getElementById('speedValue').textContent = 
            Math.round(Math.abs(y)) + '%';
            
          // 更新方向盘角度
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
        
        // 转换为百分比 (-100到100)
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

void setup() {
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
  
  // 设置WiFi AP
  WiFi.softAP(ssid, password);
  
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  // 设置服务器路由
  server.on("/", handleRoot);
  
  // 启动WebSocket服务器
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // 启动HTTP服务器
  server.begin();
  Serial.println("HTTP and WebSocket servers started");
}

void loop() {
  webSocket.loop();
  server.handleClient();
}

#endif