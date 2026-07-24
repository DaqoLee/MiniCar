# RemoteS3 — ESP32-S3 遥控器

基于 ESP32-S3-PICO-1-N8R8 的开源遥控器，用于遥控车（或其他模型）的无线控制。配合 [MiniCar](https://github.com/DaqoLee/MiniCar) 车端项目使用。

## 特性

- **ESP32-S3** 主控，8MB Flash + 8MB PSRAM
- **1.3 寸 240×240 ST7789** 圆形 LCD 屏幕
- **LVGL v8.4** 图形界面，多页面切换（遥控、设备配对、校准、拨盘控制、系统信息）
- **ESP-NOW** 通信，低延迟、无需路由器
- 双摇杆模拟输入
- 三按键导航（上/下/确认），配合按键组聚焦实现全界面键盘操作
- 设备配对 / 管理，NVS 持久化保存已配对设备
- 电池电压检测与电量显示
- 连接状态检测与指示

## 硬件

| 组件 | 说明 |
| --- | --- |
| 主控 | ESP32-S3-PICO-1-N8R8 |
| 屏幕 | 1.3" 240×240 ST7789 SPI |
| 摇杆 | 双轴摇杆 ×2 |
| 按键 | KEY_R1 / KEY_R2 / KEY_R3（上/下/确认） |
| 背光 / 电源 | TFT_BL (GPIO38) / POWER_PIN (GPIO44) |

## 快速开始

### 环境

- [PlatformIO](https://platformio.org/)
- Arduino 框架

### 编译与烧录

```bash
# 安装依赖（PlatformIO 会自动拉取 lib_deps）
pio pkg install

# 编译
pio run

# 烧录
pio run --target upload

# 串口监视器
pio device monitor
```

### 引脚配置

在 `platformio.ini` 中以编译宏定义：

| 引脚 | 功能 | GPIO |
| --- | --- | --- |
| TFT_SCLK | SPI 时钟 | 41 |
| TFT_MOSI | SPI 数据 | 40 |
| TFT_CS | SPI 片选 | 42 |
| TFT_DC | 数据/命令 | 43 |
| TFT_RST | 复位 | 39 |
| TFT_BL | 背光 | 38 |
| POWER_PIN | 电源保持 | 44 |
| KEY_R1 | 按键 1（上） | 45 |
| KEY_R2 | 按键 2（下） | 46 |
| KEY_R3 | 按键 3（确认） | 0 |
| BATTERY_PIN | 电池检测 ADC | 10 |
| RGB_PIN | WS2812 LED 数据 | 15 |

## 项目结构

```
Remote_S3_GUI/
├── platformio.ini       # 平台配置与编译宏
├── boards/               # 板级定义 JSON
├── include/
│   ├── Version.h         # 版本号、作者、构建时间
│   ├── lv_conf.h         # LVGL 配置
│   └── main.h            # 引脚定义
└── src/
    ├── main.cpp          # 入口：初始化、LVGL 显示驱动、FreeRTOS 任务
    ├── App.cpp/h         # 应用初始化、页面管理器
    ├── ShellFunc.cpp     # 串口 shell 辅助
    ├── HAL/              # 硬件抽象层
    ├── Pages/            # 页面
    │   ├── StartUp/      # 开机动画
    │   ├── Pair/         # ESP-NOW 设备配对
    │   ├── Device/       # 已配对设备列表与切换
    │   ├── Dialplate/    # 遥控主界面（摇杆仪表盘）
    │   ├── Calibrate/    # 摇杆校准
    │   └── SystemInfos/  # 系统信息
    └── Resource/         # 字体、图片等资源
```

## 固件版本

| 项 | 值 |
| --- | --- |
| 名称 | POCKET |
| 软件版本 | v2.7 |
| 硬件版本 | v1.0 |
| 作者 | DaqoLee |

## 许可

本项目基于 MIT 协议开源。部分底层代码来自 [_VIFEXTech](https://github.com/FASTSHIFT/X-TRACK) 的 X-TRACK 项目（MIT 协议）。
