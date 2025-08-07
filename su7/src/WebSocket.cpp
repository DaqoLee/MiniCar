#include "WebSocket.h"




WebServer mobileServer(80);    // 手机遥控模式服务器
WebSocketsServer webSocket(81); // WebSocket服务器

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


