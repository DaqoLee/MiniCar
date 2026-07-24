#pragma once

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Update.h>
#include <WiFi.h>



extern WebServer otaServer;     // OTA模式服务器（使用不同端口）



void handleOtaRoot();
void handleOtaUpdate();
void initOtaMode();
