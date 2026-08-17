
## 项目简介
本项目基于`ESP32S3`设计，板载一块1.3英寸LCD，移植了[X-TRACK](https://github.com/FASTSHIFT/X-TRACK.git)的UI框架，可以显示基本的遥控信息，配合开源的小车项目使用。使用ESP-NOW通信，目前已经实现`摇杆校准`、`一键对频`、`电量回传`，最多支持保存10个设备，可以在菜单中切换。


 <img src="https://image.lceda.cn/oshwhub/pullImage/1be66a2987b7455baaeedaad7dd560b1.jpg" width="500" /> 

## 硬件说明

原本硬件支持游戏机开发，但是目前只实现了遥控器。只是使用遥控功能只需要焊接下图的元件，红框部分是摇杆的FPC连接座，需要焊接。

 <img src="https://image.lceda.cn/oshwhub/pullImage/7ecfd5de1e074f8086ade9f16e7579bd.jpg" width="500" /> 

原理图里也做了标注 

 <img src="https://image.lceda.cn/oshwhub/pullImage/3ce301fb521a4fe1863af6e7d20c4d99.jpg" width="500" /> 
 
## 硬件电路
### 开源链接

[开源链接](https://oshwhub.com/daqolee/project_lusvtvwk?jlc_vid=FFRYAgUAElULUVVfElVYX1RRFABWX1ZSRFYMAQFfQ1gxVlNeQVhdV1RQQlFaUjtW)
## 组装流程

### 标准件清单
| 序号 | 名称 | 数量 | 备注 | 购买链接 |
| ---- | ---- | ---- | ---- | -------- |
| 1 | 1.3英寸LCD | 1 | 插接 | [淘宝链接](https://item.taobao.com/item.htm?id=737088494920&amp;mi_id=0000SP0UDB7zt5-BP33OS4YAxJG0pcGT3--oYHjwSHCnKZY&amp;skuId=5262987987522&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.7f342e8dRjhpTf) |
| 2 | 锂电池 | 1 | 500mAh | [淘宝链接](https://item.taobao.com/item.htm?id=553314372759&amp;mi_id=0000IAimHCfos30btUx4C3yM-xzlmwSLL8HMrMy7ehHBWTs&amp;skuId=6174834350362&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.7f342e8dRjhpTf) |
| 3 | Switch摇杆 | 2 | 一代 | [淘宝链接](https://detail.tmall.com/item.htm?id=701787851629&amp;mi_id=0000yY4K06E_3_O8t8yk1wSIDITpcmemOUUoOpkc6UW09nQ&amp;skuId=6106806740364&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.7f342e8dRjhpTf) |
| 4 | 贴片螺母 | 4 | M1.6*￠3*1.0+￠2*0.8 | [淘宝链接](https://item.taobao.com/item.htm?id=673872431344&amp;mi_id=0000tqDVRoKvuCd7znQLk1RIfwSYT_42qnnTS0NkJTIOGZg&amp;spm=tbpc.boughtlist.suborder_itemtitle.1.7f342e8dRjhpTf) |
| 5 | M1.6螺丝 | 4 | M1.6*3mm | [淘宝链接](https://detail.tmall.com/item.htm?id=600869955392&amp;mi_id=0000DcW-0zHTFnfN2QJvvA0B6DkzNdI41MBSFYUvPaXetSg&amp;skuId=5294951613139&amp;spm=tbpc.boughtlist.suborder_itempic.d600869955392.544f2e8d5i5Y1s) |
| 6 | M2十字螺丝 | 4 | M2*6mm | [淘宝链接](https://detail.tmall.com/item.htm?id=600869955392&amp;mi_id=0000DcW-0zHTFnfN2QJvvA0B6DkzNdI41MBSFYUvPaXetSg&amp;skuId=5294951613139&amp;spm=tbpc.boughtlist.suborder_itempic.d600869955392.544f2e8d5i5Y1s) |

外壳模型已经上传[附件](https://oshwhub.com/daqolee/project_lusvtvwk?jlc_vid=FFRYAgUAElULUVVfElVYX1RRFABWX1ZSRFYMAQFfQ1gxVlNeQVhdV1RQQlFaUjtW)

 <img src="https://image.lceda.cn/oshwhub/pullImage/d4a73a62db8c4fc1adf325b0eca69154.jpg" width="500" /> 