# MiniCar

## 项目简介

本项目基于ESP32C3设计，板载锂电池无线充电、1路电机驱动、1路舵机驱动、1路RGB。可以使用手机或者自制的遥控器遥控，遥控器支持一键对频，支持OTA更新固件。

<img src="https://image.lceda.cn/oshwhub/pullImage/46fe95f5ed8c4f709f3e180d9fa58f3c.jpg" alt="SU7实物图" width="500" />


## 项目功能

### 用户按键

| 按键 | 功能|RGB|
| --- | --- |--- |
| 单击 | 开机 |绿
| 双击 | 模式切换 |手机：`蓝` 遥控：`绿` OTA： `红`
| 长按 | 关机 |灭
| 关机状态长按7s | 配对 |粉


<img src="https://image.lceda.cn/oshwhub/pullImage/83388d2c5546420480fbfe259f17863c.jpg" alt="按键" width="500" />

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

[开源链接](https://oshwhub.com/daqolee/project_vfoxufca?jlc_vid=FFRYAgUAElULUVVfElVYX1RRFABWX1ZSRFYMAQFfQ1gxVlNeQVhdV1RQQlFaUjtW)

## 注意事项

`这一版不能直接通过下载板供电，必须要接电池才能供电。下载程序时必须长按开机按键保证芯片供电！！！`
下载完程序后就可以`OTA`模式更新固件
## 组装流程
# SU7静改动标准件整理清单
| 序号 | 名称 | 数量 | 备注 | 购买链接 |
| ---- | ---- | ---- | ---- | -------- |
| 1 | M1.2*4mm十字螺丝 | 3 | M1.2*4mm | [淘宝链接](https://detail.tmall.com/item.htm?id=600869955392&amp;mi_id=0000DcW-0zHTFnfN2QJvvA0B6DkzNdI41MBSFYUvPaXetSg&amp;skuId=5294951613139&amp;spm=tbpc.boughtlist.suborder_itempic.d600869955392.544f2e8d5i5Y1s) |
| 2 | M1.2*6mm十字螺丝 | 6 | M1.2*6mm | [淘宝链接](https://detail.tmall.com/item.htm?id=600869955392&amp;mi_id=0000DcW-0zHTFnfN2QJvvA0B6DkzNdI41MBSFYUvPaXetSg&amp;skuId=5294951613139&amp;spm=tbpc.boughtlist.suborder_itempic.d600869955392.544f2e8d5i5Y1s) |
| 3 | M1.2*8mm十字螺丝 | 2 | M1.2*8mm | [淘宝链接](https://detail.tmall.com/item.htm?id=600869955392&amp;mi_id=0000DcW-0zHTFnfN2QJvvA0B6DkzNdI41MBSFYUvPaXetSg&amp;skuId=5294951613139&amp;spm=tbpc.boughtlist.suborder_itempic.d600869955392.544f2e8d5i5Y1s) |
| 4 | M1.2*10mm十字螺丝 | 4 | M1.2*10mm | [淘宝链接](https://detail.tmall.com/item.htm?id=600869955392&amp;mi_id=0000DcW-0zHTFnfN2QJvvA0B6DkzNdI41MBSFYUvPaXetSg&amp;skuId=5294951613139&amp;spm=tbpc.boughtlist.suborder_itempic.d600869955392.544f2e8d5i5Y1s) |
| 5 | 空心杯电机 | 1 | 0612空心杯0.8mm轴 | [淘宝链接](https://detail.tmall.com/item.htm?id=813164267560&amp;mi_id=0000tFqPhsRInCpJKJOGa-hcBByoI8e4dtEaGVQqWqtWL-U&amp;skuId=5678001151853&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.544f2e8d5i5Y1s) |
| 6 | 黄铜齿轮1 | 1 | 模数0.2齿数15厚度1.2mm孔径0.78mm | [淘宝链接](https://item.taobao.com/item.htm?id=675331407574&amp;mi_id=0000xmxMzjfR6EusP6magRYzQ6bewlO5vm8UoS7XCbympNE&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.5b762e8dntoKon) |
| 7 | 黄铜齿轮2 | 1 | 模数0.2齿数40厚度2mm孔径2mm 紧配 | [淘宝链接](https://item.taobao.com/item.htm?id=676243317428&amp;mi_id=0000GZ2Z6D2PX_B6ogx61FrZ0JrJ3xzMhXjGj3Ne5xwlwlE&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.5b762e8dntoKon) |
| 8 | 黄铜齿轮3 | 1 | 模数0.3齿数12厚度4mm孔径1.98mm | [淘宝链接](https://item.taobao.com/item.htm?id=559892580057&amp;mi_id=0000WNy_MRjNAo1BCNG2GYFWo-d-CHtrsazclGI7tpo6jJk&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.5b762e8dntoKon) |
| 9 | 黄铜齿轮4 | 1 | 模数0.3齿数25厚度4mm孔径1.98mm | [淘宝链接](https://item.taobao.com/item.htm?id=35670047791&amp;mi_id=0000xLVGtCFVMNrB-8VnqYc6mG1ACtkdZTrE1jF65Io5gj8&amp;skuId=5567032128063&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.5b762e8dntoKon) |
| 10 | 传动轴 | 1 | 直径2mm长度14mm | [淘宝链接](https://detail.tmall.com/item.htm?abbucket=16&amp;id=587625731480&amp;rn=dc82174aedaee7a16c1243a01fc9ba97&amp;spm=a1z10.3-b-s.w4011-25408083092.60.306c137fvQU6EY) |
| 11 | 后轮轴 | 1 | 直径2mm长度24mm | [淘宝链接](https://detail.tmall.com/item.htm?abbucket=16&amp;id=587625731480&amp;rn=dc82174aedaee7a16c1243a01fc9ba97&amp;spm=a1z10.3-b-s.w4011-25408083092.60.306c137fvQU6EY) |
| 12 | 传动轴轴承 | 4 | 内径2mm外径4mm厚度2mm | [淘宝链接](https://item.taobao.com/item.htm?id=650753474200&amp;mi_id=0000pyEA3yx_BSgMLFDTsIUOOSVWEpsnUpeEPyOF7nm-n0E&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.5b762e8dntoKon) |
| 13 | 转向舵机 | 1 |  | [淘宝链接](https://item.taobao.com/item.htm?id=717560186295&amp;mi_id=0000aLkXJaiPDF2aJr451TD0I0P8Pr-lOKOf1pzqbqVMuGw&amp;skuId=5934252139621&amp;spm=tbpc.boughtlist.suborder_itempic.d717560186295.5b762e8dntoKon) |
| 14 | 转向槽光轴 | 1 | 直径1.5mm长度8mm | [淘宝链接](https://detail.tmall.com/item.htm?abbucket=16&amp;id=587625731480&amp;rn=dc82174aedaee7a16c1243a01fc9ba97&amp;spm=a1z10.3-b-s.w4011-25408083092.60.306c137fvQU6EY) |
| 15 | 转向轮轴承 | 2 | 内径4mm外径7mm厚度2mm | [淘宝链接](https://item.taobao.com/item.htm?id=650385687905&amp;mi_id=0000BQoOtxKgNJejPQhynr4gfMNELtgPsFKgNoPi2unbgm8&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.5b762e8dntoKon) |
| 16 | 电池 | 1 |  | 无 |
| 17 | 按钮 | 1 | 卧贴2*4mm | [立创商城](https://item.szlcsc.com/372554.html?fromZone=s_s__%2522C393942%2522&amp;spm=sc.gbn.xh1.zy.t&amp;lcsc_vid=ElALVlJRE1ZYBAEDRgBcVgFTFldaVF0CQwUPV1FTR1AxVlNeQlBaUlRVTlBWUTsOAxUeFF5JWBIBSRccGwIdBEoFGAxBAAgJFQACSQwSGg0%3D) |
| 18 | 无线充线圈 | 1 | 25mm圆形 | [淘宝链接](https://item.taobao.com/item.htm?id=795308532169&amp;mi_id=0000kNW4rJj54MHp7MACWG3QcgMdVkyoChhVE8-3-b5NP5Y&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.1af42e8dxag9Co) |

模型附件已经上传这里简单指示一下 

<img src="https://image.lceda.cn/oshwhub/pullImage/d536756af5d34626a5f08e81300b6bd3.png" alt="车架" width="500" />

<img src="https://image.lceda.cn/oshwhub/pullImage/4a4dd752387243ada8a4603e19909cd6.png" alt="动力" width="500" />

螺丝的型号与位置 

<img src="https://image.lceda.cn/oshwhub/pullImage/9e55d67839d74608bc93e1abb2f6bb50.png" alt="螺丝1" width="500" />

<img src="https://image.lceda.cn/oshwhub/pullImage/6767b019ee84430dac9a0851627bc1cc.png" alt="螺丝2" width="500" />

## 注意事项
- `轮毂部分需要光固化打印`，其他的普通FDM打印机就可以

<img src="https://image.lceda.cn/oshwhub/pullImage/d53d4c9eaa1c488bbd1143adde58c07a.jpg" alt="轮毂" width="500" />

- 车壳前轮眉部分需要打磨，不然转向会卡住轮胎


<img src="https://image.lceda.cn/oshwhub/pullImage/1bd47da0425c49ceb77c50f7f45c44f6.jpg" alt="打磨" width="500" />

## 实物图


<img src="https://image.lceda.cn/oshwhub/pullImage/d84f857327bb4dd291b654362fb1ed4c.jpg" alt="SU7底盘" width="500" />  
  
### 
<img src="https://image.lceda.cn/oshwhub/pullImage/befa6bb92c3149de9cec602ff5156876.jpg" alt="SU7实物图" width="500" />
