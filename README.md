

## 项目简介

本项目基于ESP32C3设计，板载锂电池充电、1路电机驱动、3路舵机驱动、1路RGB。可以使用手机或者自制的遥控器遥控，遥控器支持一键对频，支持OTA更新固件。



<img src="https://image.lceda.cn/oshwhub/pullImage/4a791ffd1c7e49deaf5416d81f539f77.jpg" alt="卡车3" width="500" />

## 项目功能

### 用户按键

| 按键 | 功能|指示灯|
| --- | --- |--- |
| 单击 | 开机 |绿
| 双击 | 模式切换 |手机：`蓝` 遥控：`绿` OTA： `红`
| 长按 | 关机 |灭
| 关机状态长按7s | 配对 |粉


<img src="https://image.lceda.cn/oshwhub/pullImage/56260d4165734aa299929a2c62d24469.jpg" alt="按键" width="500" />

### 遥控模式
#### 遥控器
上电默认使用遥控器遥控，遥控器开源链接。使用ESP-NOW通信，初次使用需要进行配对。


```
1、关机状态长按用户按键7s，RGB灯变为粉色时进入配对模式
2、遥控器进入菜单选择配对，进入配对模式
3、配对成功遥控会显示小车主控MAC地址后4位
```

遥控逻辑

| 操作 |动作  |
| --- | --- |
|  左边摇杆|前进后退  |
|  右边摇杆|方向  |
|  左边摇杆按下|车斗抬升  |
|  右边摇杆按下|车斗下降  |

#### 手机
切换手机遥控模式后LED为`蓝色`
- 手机连接以下热点
```
Ssid = "ESP32C3-Car"
password = "12345678"     
```

- 浏览器访问 [192.168.4.1](192.168.4.1) 就可以进入控制界面

<img src="https://image.lceda.cn/oshwhub/pullImage/0cc2f65452774d0e9d5767a882d93639.jpg" alt="遥控界面" width="500" />

#### OTA
- 电脑连接以下热点

```
Ssid = "ESP32C3-OTA"
password = "12345678"     
```

- 浏览器访问 [http://192.168.4.1:8080](http://192.168.4.1:8080)
- 页面里选择编译好的 `.bin` 固件文件上传，上传完成后重启

<img src="https://image.lceda.cn/oshwhub/pullImage/cd2d800ce104432d9c9da198f771a951.jpg" alt="OTA" width="500" />

## 硬件电路

### 开源链接

[硬件开源链接](https://oshwhub.com/daqolee/project_tqczetta?jlc_vid=FFRYAgUAElULUVVfElVYX1RRFABWX1ZSRFYMAQFfQ1gxVlNeQVhdV1RQQlFaUjtW)



## 组装教程

详细的组装教程已经上传，[下载附件PDF](https://oshwhub.com/daqolee/project_tqczetta?jlc_vid=FFRYAgUAElULUVVfElVYX1RRFABWX1ZSRFYMAQFfQ1gxVlNeQVhdV1RQQlFaUjtW)

<img src="https://image.lceda.cn/oshwhub/pullImage/cf2e5a20c5cc470386460392ae2d8233.png" alt="接线" width="500" />

## 实物图

<img src="https://image.lceda.cn/oshwhub/pullImage/27c261d9157a487ea6b293dfdb5f7ce8.jpg" alt="卡车0" width="500" />

### 

<img src="https://image.lceda.cn/oshwhub/pullImage/8200557261e742bdbe4721398f2e3fc1.jpg" alt="卡车1" width="500" />

### 

<img src="https://image.lceda.cn/oshwhub/pullImage/79ae140af2064501ab1becfca5f98ff8.jpg" alt="卡车2" width="500" />

