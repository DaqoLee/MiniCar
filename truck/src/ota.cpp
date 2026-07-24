#include "ota.h"

const char* otaSsid = "ESP32C3-OTA";     // OTA模式热点
const char* otaPassword = "12345678";   
// 两个独立的服务器
WebServer otaServer(8080);     // OTA模式服务器（使用不同端口）

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

// 初始化OTA模式
void initOtaMode() {
  // 设置AP热点（不同的SSID）
  WiFi.softAP(otaSsid, otaPassword);
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
    </style>
  </head>
  <body>
    <div class="container">
      <h1>POCKET CAR FIRMWARE UPDATE</h1>
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