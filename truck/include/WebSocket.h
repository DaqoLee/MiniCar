#pragma once

#include <WebServer.h>
#include <WebSocketsServer.h>

extern WebServer mobileServer;    // 手机遥控模式服务器
extern WebSocketsServer webSocket; // WebSocket服务器


void handleMobileRoot();
void initMobileMode();