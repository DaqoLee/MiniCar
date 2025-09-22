

#if 0

#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "main.h"
#include "ShellFunc.h"
#include <TFT_eSPI.h>
#include <TFT_eWidget.h>           // Widget library
#include "Free_Fonts.h" // Include the header file attached to this sketch
#include <lvgl.h>

// 
#include "events_init.h"
#include "gui_guider.h"
#include "custom.h"

#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <FastLED.h>
#include <Preferences.h>

#define NUM_LEDS 1
CRGB leds[NUM_LEDS];
// 
#define JOY1_X 4
#define JOY1_Y 5
#define JOY2_X 2
#define JOY2_Y 1

#define MAX_JOY1_X 3400
#define MID_JOY1_X 1960
#define MIN_JOY1_X 630

#define MAX_JOY2_X 3050
#define MID_JOY2_X 1565
#define MIN_JOY2_X 100

#define MAX_JOY1_Y 2960
#define MID_JOY1_Y 1650
#define MIN_JOY1_Y 350

#define MAX_JOY2_Y 3420
#define MID_JOY2_Y 2070
#define MIN_JOY2_Y 680


#define JOY_MAX 100
#define JOY_MINI 0

#define MAX_VALUE 100
#define MIN_VALUE 0


// 定义最大设备数量
#define MAX_DEVICES 10

// void CAN_RX_Task(void *pvParameters);//void *pvParameters

// void task(void*p){
//     while(1){

//       vTaskDelay(1500);
//     }
// }
Preferences preferences;
// 模式定义
enum OperationMode {
  MODE_JOYSTICK,  // 摇杆控制模式
  MODE_OTA,        // OTA更新模式
  MODE_PAIR
};

volatile OperationMode currentMode = MODE_JOYSTICK;

typedef struct struct_pair {
  char type[16];    // 消息类型
  uint8_t data[32]; // 数据载荷
  int data_len;     // 数据长度
} struct_pair;


// 存储配对信息的数据结构
typedef struct {
  uint8_t mac[6];
  char name[32];
  unsigned long lastSeen;
} device_info_t;

device_info_t pairedDevices[MAX_DEVICES];

uint8_t pairedCount = 0;
uint8_t pairIndex = 0;
bool isPaired = false;

typedef struct struct_ack {
  uint8_t mode; // 确认的消息ID
  uint8_t battery;
} struct_ack;

struct_ack ackData;

TFT_eSPI tft = TFT_eSPI();
lv_display_t *display;
lv_ui guider_ui;


static lv_color_t buf1[TFT_WIDTH * 10];
lv_indev_t * indev_keypad;
lv_group_t * group ;
void keypad_init();
static void keypad_read(lv_indev_t * indev_drv, lv_indev_data_t * data);

// uint8_t receiverMac[] = {0xDC, 0x06, 0x75, 0xA9, 0x93, 0x20};
uint8_t receiverMac[] = {0x0C, 0x4E, 0xA0, 0x21, 0x29, 0x3C};

char joy1X_str[5];


typedef struct struct_message {
  int16_t joy1X;
  int16_t joy1Y;
  int16_t joy2X;
  int16_t joy2Y;
} struct_message;
struct_message txData;

// 电池参数
float batteryVoltage = 0.0;
float batteryPercentage = 0.0;
const float maxBatteryVoltage = 4.2;
const float minBatteryVoltage = 3.0;
const float voltageDividerRatio = 2.0;
unsigned long lastBatteryUpdate = 0;
const long batteryUpdateInterval = 2000;



void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushPixels((uint16_t *) px_map, w * h);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

static void slider_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj =  (lv_obj_t *)lv_event_get_target(e);

    /*Provide some extra space for the value*/
    if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_event_set_ext_draw_size(e, 50);
    }
    else if(code == LV_EVENT_DRAW_MAIN_END) {
        if(!lv_obj_has_state(obj, LV_STATE_PRESSED)) return;

        lv_area_t slider_area;
        lv_obj_get_coords(obj, &slider_area);
        lv_area_t indic_area = slider_area;
        lv_area_set_width(&indic_area, lv_area_get_width(&slider_area) * lv_slider_get_value(obj) / MAX_VALUE);
        indic_area.x1 += lv_area_get_width(&slider_area) * lv_slider_get_left_value(obj) / MAX_VALUE;
        char buf[16];
        lv_snprintf(buf, sizeof(buf), "%d - %d", (int)lv_slider_get_left_value(obj), (int)lv_slider_get_value(obj));

        lv_point_t label_size;
        lv_text_get_size(&label_size, buf, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, (lv_text_flag_t)0);
        lv_area_t label_area;
        label_area.x1 = 0;
        label_area.x2 = label_size.x - 1;
        label_area.y1 = 0;
        label_area.y2 = label_size.y - 1;

        lv_area_align(&indic_area, &label_area, LV_ALIGN_OUT_TOP_MID, 0, -10);

        lv_draw_label_dsc_t label_draw_dsc;
        lv_draw_label_dsc_init(&label_draw_dsc);
        label_draw_dsc.color = lv_color_hex3(0x888);
        label_draw_dsc.text = buf;
        label_draw_dsc.text_local = true;
        lv_layer_t * layer = lv_event_get_layer(e);
        lv_draw_label(layer, &label_draw_dsc, &label_area);
    }
}

static void slider1_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj =  (lv_obj_t *)lv_event_get_target(e);

    /*Provide some extra space for the value*/
    if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_event_set_ext_draw_size(e, 50);
    }
    else if(code == LV_EVENT_DRAW_MAIN_END) {
        if(!lv_obj_has_state(obj, LV_STATE_PRESSED)) return;

        lv_area_t slider_area;
        lv_obj_get_coords(obj, &slider_area);
        lv_area_t indic_area = slider_area;
        lv_area_set_width(&indic_area, lv_area_get_width(&slider_area) * lv_slider_get_value(obj) / MAX_VALUE);
        indic_area.x1 += lv_area_get_width(&slider_area) * lv_slider_get_left_value(obj) / MAX_VALUE;
        char buf[16];
        lv_snprintf(buf, sizeof(buf), "%d - %d", (int)lv_slider_get_left_value(obj), (int)lv_slider_get_value(obj));

        lv_point_t label_size;
        lv_text_get_size(&label_size, buf, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, (lv_text_flag_t)0);
        lv_area_t label_area;
        label_area.x1 = 0;
        label_area.x2 = label_size.x - 1;
        label_area.y1 = 0;
        label_area.y2 = label_size.y - 1;

        lv_area_align(&indic_area, &label_area, LV_ALIGN_OUT_TOP_MID, 0, -10);

        lv_draw_label_dsc_t label_draw_dsc;
        lv_draw_label_dsc_init(&label_draw_dsc);
        label_draw_dsc.color = lv_color_hex3(0x888);
        label_draw_dsc.text = buf;
        label_draw_dsc.text_local = true;
        lv_layer_t * layer = lv_event_get_layer(e);
        lv_draw_label(layer, &label_draw_dsc, &label_area);
    }
}

void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    /*For example  ("my_..." functions needs to be implemented by you)
    int32_t x, y;
    bool touched = my_get_touch( &x, &y );

    if(!touched) {
        data->state = LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_PRESSED;

        data->point.x = x;
        data->point.y = y;
    }
     */
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}
void lvgl_user_init(void)
{
  lv_init();
  
  /*Set the touchscreen calibration data,
    the actual data for your display can be acquired using
    the Generic -> Touch_calibrate example from the TFT_eSPI library*/
  // uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
  // tft.setTouch( calData );

  lv_init();
  display = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
  lv_display_set_flush_cb(display, my_flush_cb);
  lv_display_set_buffers(display, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_tick_set_cb(my_tick);

  setup_ui(&guider_ui);
  events_init(&guider_ui);
  custom_init(&guider_ui);
  /*------------------
    * Keypad
    * -----------------*/

  /*Initialize your keypad or keyboard if you have*/
  keypad_init();

  /*Register a keypad input device*/
  indev_keypad = lv_indev_create();
  lv_indev_set_type(indev_keypad, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(indev_keypad, keypad_read);



      /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
    *add objects to the group with `lv_group_add_obj(group, obj)`
    *and assign this input device to group to navigate in it:
    *`lv_indev_set_group(indev_encoder, group);`*/

  lv_obj_t* tbs0 = lv_tabview_get_tab_btns(guider_ui.main_tabview_1);

  lv_obj_t* tbs1 = lv_obj_get_child(tbs0, 0);
  lv_obj_t* tbs2 = lv_obj_get_child(tbs0, 1);
  lv_obj_t* tbs3 = lv_obj_get_child(tbs0, 2);
  lv_group_focus_obj(tbs1);

  group = lv_group_create();

  lv_group_set_default(group);

  // lv_group_focus_obj(group);          //分组聚焦到对象
  lv_group_set_editing(group, true);   //编辑模式
  // lv_group_add_obj(group, tbs0);
  lv_group_add_obj(group, tbs1);
  lv_group_add_obj(group, tbs2);
  lv_group_add_obj(group, tbs3);
  // lv_group_add_obj(group, guider_ui.main_tabview_1);
  //  lv_group_add_obj(group, guider_ui.main_tabview_1_tab_1);
  //  lv_group_add_obj(group, guider_ui.main_tabview_1_tab_2);
  //  lv_group_add_obj(group, guider_ui.main_tabview_1_tab_3);
  // lv_group_add_obj(group, guider_ui.main_btn_1);
  // lv_group_add_obj(group, guider_ui.main_btn_2);
  lv_indev_set_group(indev_keypad, group);

// lv_color_t* buf1 = (lv_color_t*) heap_caps_malloc(240 * 240, MALLOC_CAP_SPIRAM);
// lv_color_t* buf2 = (lv_color_t*) heap_caps_malloc(240 * 240, MALLOC_CAP_SPIRAM);
//   lv_disp_draw_buf_init( &draw_buf, buf1, NULL, 240 * 240);

//   /*Initialize the display*/
//   static lv_disp_drv_t disp_drv;
//   lv_disp_drv_init( &disp_drv );
//   /*Change the following line to your display resolution*/
//   disp_drv.hor_res = 240;
//   disp_drv.ver_res = 240;
//   disp_drv.flush_cb = my_flush_cb;
//   disp_drv.full_refresh = 1;
//   disp_drv.draw_buf = &draw_buf;
//   lv_disp_drv_register(&disp_drv);

//   /*Initialize the (dummy) input device driver*/
//   static lv_indev_drv_t indev_drv;
//   lv_indev_drv_init(&indev_drv);
//   indev_drv.type = LV_INDEV_TYPE_POINTER;
//   indev_drv.read_cb = my_touchpad_read;
//   lv_indev_drv_register(&indev_drv);


}

// 保存配对设备信息到NVS
void savePairedDevices() {
  preferences.begin("esnow_multi", false);
  preferences.putUInt("paired_count", pairedCount);
  preferences.putUInt("paired_index", pairIndex);
  
  for (int i = 0; i < pairedCount; i++) {
    char macKey[20];
    sprintf(macKey, "mac_%d", i);
    preferences.putBytes(macKey, pairedDevices[i].mac, 6);
    
    char nameKey[20];
    sprintf(nameKey, "name_%d", i);
    preferences.putString(nameKey, pairedDevices[i].name);

    Serial.println(macKey);
    Serial.println(nameKey);
  }
  
  preferences.end();
  Serial.println("配对设备信息已保存");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {

  switch (currentMode)
  {
  case MODE_JOYSTICK:{

    memcpy(&ackData, incomingData, sizeof(ackData));
    Serial.printf("ACK mode: %d battery: %d\r\n",ackData.mode, ackData.battery);

    break;
  }  
  case MODE_PAIR:{


    struct_pair myData;
    memcpy(&myData, incomingData, sizeof(myData));
    
    // 处理配对响应
    if (strcmp(myData.type, "PAIRING_RESP") == 0) {
      // 检查是否已存在此设备
      bool exists = false;
      for (int i = 0; i < pairedCount; i++) {
        if (memcmp(pairedDevices[i].mac, mac, 6) == 0) {
          exists = true;
          break;
        }
      }
    // 如果不存在且未超过最大数量，则添加到配对列表
      if (!exists && pairedCount < MAX_DEVICES) {
        memcpy(pairedDevices[pairedCount].mac, mac, 6);
        
        // 从数据中提取设备名称（如果有）
        if (myData.data_len > 0) {
          strncpy(pairedDevices[pairedCount].name, (char*)myData.data, min(myData.data_len, 31));
          pairedDevices[pairedCount].name[31] = '\0';
        } else {
          // 如果没有名称，使用MAC地址后4位作为默认名称
          snprintf(pairedDevices[pairedCount].name, sizeof(pairedDevices[pairedCount].name), 
                    "Device_%02X%02X", mac[4], mac[5]);
        }
        
        pairedDevices[pairedCount].lastSeen = millis();
        /* 当前设备切换为新配对的设备 */
        pairIndex = pairedCount;
        pairedCount++;
        isPaired = true;
        Serial.print("新设备已添加: ");
        Serial.print(pairedDevices[pairedCount-1].name);
        Serial.print("，当前配对设备数: ");
        Serial.println(pairedCount);

        esp_now_peer_info_t peerInfo;
        memcpy(peerInfo.peer_addr, pairedDevices[pairIndex].mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;//老版本没有
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
          Serial.println("Error initializing ESP-NOW");
        // return;
        }
        
        // 保存配对信息
        savePairedDevices();

        currentMode = MODE_JOYSTICK;
      } else if (exists) {
        Serial.println("设备已存在，无需重复添加");
      } else {

        memcpy(pairedDevices[MAX_DEVICES - 1].mac, mac, 6);
        
        // 从数据中提取设备名称（如果有）
        if (myData.data_len > 0) {
          strncpy(pairedDevices[MAX_DEVICES - 1].name, (char*)myData.data, min(myData.data_len, 31));
          pairedDevices[MAX_DEVICES - 1].name[31] = '\0';
        } else {
          // 如果没有名称，使用MAC地址后4位作为默认名称
          snprintf(pairedDevices[MAX_DEVICES - 1].name, sizeof(pairedDevices[MAX_DEVICES - 1].name), 
                    "Device_%02X%02X", mac[4], mac[5]);
        }
        
        pairedDevices[MAX_DEVICES - 1].lastSeen = millis();
        /* 当前设备切换为新配对的设备 */
        pairIndex = MAX_DEVICES - 1;
        isPaired = true;
        Serial.println("已达到最大设备数量限制，覆盖最后一个设备");

        esp_now_peer_info_t peerInfo;
        memcpy(peerInfo.peer_addr, pairedDevices[pairIndex].mac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;//老版本没有
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
          Serial.println("Error initializing ESP-NOW");
        // return;
        }
        savePairedDevices();
        currentMode = MODE_JOYSTICK;

      }
    }      

    break;
  } 
  default:
    break;

  }
}


void listPairedDevices() {
  Serial.println("已配对设备列表:");
  Serial.println("索引\t设备名称\t\tMAC地址\t\t\t状态");
  Serial.println("----------------------------------------------------------------");
  
  for (int i = 0; i < pairedCount; i++) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             pairedDevices[i].mac[0], pairedDevices[i].mac[1], pairedDevices[i].mac[2], 
             pairedDevices[i].mac[3], pairedDevices[i].mac[4], pairedDevices[i].mac[5]);
    
    Serial.print(i);
    Serial.print("\t");
    Serial.print(pairedDevices[i].name);
    // 格式化输出对齐
    if (strlen(pairedDevices[i].name) < 8) Serial.print("\t");
    if (strlen(pairedDevices[i].name) < 16) Serial.print("\t");
    Serial.print("\t");
    Serial.print(macStr);
    Serial.print("\t");
    
    if (i == pairIndex) {
      Serial.println("[当前设备]");
    } else {
      Serial.println("");
    }
  }
  
  if (pairedCount == 0) {
    Serial.println("无已配对设备");
  }
}

void loadPairedDevices() {

  switch (currentMode)
  {
  case MODE_JOYSTICK:{

    preferences.begin("esnow_multi", true); // 只读模式打开
  /* 读取配对数 */
    pairedCount = preferences.getUInt("paired_count", 0);
    if (pairedCount)
    {
      /* 读取默认配对 */
      for (int i = 0; i < pairedCount; i++) {
          char macKey[20];
          sprintf(macKey, "mac_%d", i);
          preferences.getBytes(macKey, pairedDevices[i].mac, 6);
          
          char nameKey[20];
          sprintf(nameKey, "name_%d", i);
          pairedDevices[i].name[31] = '\0'; // 确保字符串终止
          preferences.getString(nameKey, pairedDevices[i].name, 32);
          
          pairedDevices[i].lastSeen = 0;
          Serial.println(macKey);
          Serial.println(nameKey);
      }
      pairIndex = preferences.getUInt("paired_index", 0);

      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, pairedDevices[pairIndex].mac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;//老版本没有
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
      // return;
      }
      isPaired = true;

    }
    else/* 如果没有配对信息就使用默认值 */
    {
      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, receiverMac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;//老版本没有
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
      // return;
      }
      isPaired = true;
    }
    
    preferences.end();
    break;
  }  

  case MODE_PAIR:{
    /* 首先添加一个广播对等设备，用于发送配对请求 */
    uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, broadcastMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;//老版本没有
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
    // return;
    }
    isPaired = false;
    break;
  }  
  default:
    break;
  }
}




void setup() {


  Serial.begin(115200);
  delay(200);

  pinMode(PWOR_PIN, OUTPUT);
  digitalWrite(PWOR_PIN, HIGH);
  pinMode(BL, OUTPUT);
  digitalWrite(BL, HIGH);



  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect();
  // ???ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  /* 读取配对信息 */
  loadPairedDevices();
  /* 
  
  
  
  */
  listPairedDevices();

  //  savePairedDevices();

  // currentMode = MODE_PAIR;
  // loadPairedDevices();
 
  // esp_now_peer_info_t peerInfo;
  // memcpy(peerInfo.peer_addr, receiverMac, 6);
  // peerInfo.channel = 0;
  // peerInfo.encrypt = false;
  // peerInfo.ifidx = WIFI_IF_STA;//老版本没有
  // if (esp_now_add_peer(&peerInfo) != ESP_OK) {
  //   Serial.println("Error initializing ESP-NOW");
  //  // return;
  // }

  // esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  

  Serial.print("ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());

 
  // ??????
  pinMode(JOY1_X, INPUT);
  pinMode(JOY1_Y, INPUT);
  pinMode(JOY2_X, INPUT);
  pinMode(JOY2_Y, INPUT);

    // pinMode(SPI_CS1, OUTPUT);
    // digitalWrite(SPI_CS1, LOW);

    // pinMode(SPI_CS2, OUTPUT);
    // digitalWrite(SPI_CS2, LOW);

    // SPI.begin(2, -1, 1, -1);

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true); 

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH); 

  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_LEDS); 
  leds[0] = CHSV(HUE_BLUE, 255, 20);
  FastLED.show();

  analogReadResolution(12);
  lvgl_user_init();


  xTaskCreate(GuiTask,"GuiTask",4096*2 ,NULL,1,NULL);

}


float speed = 0;
float roll, pitch, yaw;

float gyroX;
float gyroY;
float gyroZ;

// void refresh() {
//     lv_timer_handler(); // ??LVGL??
//     vTaskDelay(5);           // ?????
// }

int value = 0;
int16_t joy1Y = 0;
int16_t joy2Y = 0;
int16_t joy1X = 0;
int16_t joy2X = 0;
int16_t sendSuccessCount = 0;
void loop() {
 

    // tft.setCursor(15, 10);
    // tft.setTextFont(2);
    // tft.setTextSize(1);
    // tft.setTextColor(TFT_GREEN, TFT_BLACK);
    // tft.printf("A");

  joy1Y = analogRead(JOY1_Y);
  joy2Y = analogRead(JOY2_Y);
  joy1X = analogRead(JOY1_X);
  joy2X = analogRead(JOY2_X);

  txData.joy1X = map(joy1X, MIN_JOY1_X, MAX_JOY1_X, 0, 4095);
  txData.joy1Y = map(joy1Y, MIN_JOY1_Y, MAX_JOY1_Y, 4095, 0);
  txData.joy2X = map(joy2X, MIN_JOY2_X, MAX_JOY2_X, 4095, 0);
  txData.joy2Y = map(joy2Y, MIN_JOY2_Y, MAX_JOY2_Y, 4095, 0);

  // int rawValue = analogRead(BATTERY_PIN);
  // batteryVoltage = (((float)rawValue / 4095.0) * 3.3 * voltageDividerRatio) - 0.4;
  
  // batteryPercentage = map(constrain(batteryVoltage*100, minBatteryVoltage*100, maxBatteryVoltage*100), 
  //                         minBatteryVoltage*100, maxBatteryVoltage*100, 0, 100);
  // // Serial.printf("X1: %d, Y1: %d, X2: %d, Y2: %d\r\n",txData.joy1X,txData.joy1Y,txData.joy2X,txData.joy2Y );
  // Serial.println(batteryPercentage);

  
   if (!isPaired) {
    // 如果还未配对，则每隔5秒发送一次配对请求
    Serial.println("Sending pairing request...");

    struct_pair myRequest;
    strcpy(myRequest.type, "PAIRING");
    // 广播MAC地址已经在setup中添加
    uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_err_t result = esp_now_send(broadcastMac, (uint8_t *) &myRequest, sizeof(myRequest));

    if (result == ESP_OK) {
      Serial.println("Pairing request sent.");
    } else {
      Serial.println("Error sending pairing request.");
    }
    delay(5000);
  } else {

    esp_err_t result = esp_now_send(pairedDevices[pairIndex].mac, (uint8_t*)&txData, sizeof(txData));
      
    if (result == ESP_OK) {

      sendSuccessCount = sendSuccessCount >= 200 ? 200 : sendSuccessCount+1; 
      // Serial.println(sendSuccessCount);
    } else {

      sendSuccessCount =  sendSuccessCount <= -50 ? -50 : sendSuccessCount-10; ;
      // Serial.println(sendSuccessCount);
      }
    //  lv_event_send();
    //  
    delay(5);

  }



  

// lv_slider_set_value(guider_ui.main_slider_2, value, LV_ANIM_OFF);
// refresh();
// Serial.println(value);

}

// 电源管理任务
void GuiTask(void *pvParameters) {
  static int count = 0;
  static int btn_x = 0;
  static int btn_y = 0;
  while (1) {
    // lv_bar_set_value(guider_ui.main_bar_2, txData.joy1X/100, LV_ANIM_OFF);
    // lv_obj_send_event(guider_ui.main_label_14, LV_EVENT_VALUE_CHANGED, &txData.joy1X);
    //"%d" 里面不能有空格
    lv_label_set_text_fmt(guider_ui.main_label_14, "%d",  txData.joy1X);
    lv_label_set_text_fmt(guider_ui.main_label_13, "%d",  txData.joy1Y);

    lv_label_set_text_fmt(guider_ui.main_label_9, "%d",  txData.joy2X);
    lv_label_set_text_fmt(guider_ui.main_label_10, "%d",  txData.joy2Y);

    count = map(txData.joy1X, MID_JOY1_X, MAX_JOY1_X, 0, 100);
    lv_bar_set_value(guider_ui.main_bar_2, count, LV_ANIM_OFF);

    count = map(txData.joy1X, MIN_JOY1_X, MID_JOY1_X, 0, -100);
    lv_bar_set_value(guider_ui.main_bar_3, count, LV_ANIM_OFF);
    
    count = map(txData.joy1Y, MAX_JOY1_Y, MID_JOY1_Y, 0, -100);
    lv_bar_set_value(guider_ui.main_bar_4, count, LV_ANIM_OFF);

    count = map(txData.joy1Y, MID_JOY1_Y, MIN_JOY1_Y, 0, 100);
    lv_bar_set_value(guider_ui.main_bar_1, count, LV_ANIM_OFF);

    btn_x = map(txData.joy1X, MIN_JOY1_X, MAX_JOY1_X, 90, 0);
    btn_y = map(txData.joy1Y, MIN_JOY1_Y, MAX_JOY1_Y, 90, 0);
    lv_obj_set_pos(guider_ui.main_btn_1, btn_y, btn_x);
//
    count = map(txData.joy2X, MID_JOY2_X, MAX_JOY2_X, 0, 100);
    lv_bar_set_value(guider_ui.main_bar_7, count, LV_ANIM_OFF);

    count = map(txData.joy2X, MIN_JOY2_X, MID_JOY2_X, 0, -100);
    lv_bar_set_value(guider_ui.main_bar_6, count, LV_ANIM_OFF);

    count = map(txData.joy2Y, MID_JOY2_Y, MAX_JOY2_Y, 0, 100);
    lv_bar_set_value(guider_ui.main_bar_8, count, LV_ANIM_OFF);

    count = map(txData.joy2Y, MIN_JOY2_Y, MID_JOY2_Y, 0, -100);
    lv_bar_set_value(guider_ui.main_bar_5, count, LV_ANIM_OFF);

    

    btn_x = map(txData.joy2X, MIN_JOY2_X, MAX_JOY2_X, 90, 0);
    btn_y = map(txData.joy2Y, MIN_JOY2_Y, MAX_JOY2_Y, 0, 90);
    lv_obj_set_pos(guider_ui.main_btn_2, btn_y, btn_x);

    lv_bar_set_value(guider_ui.main_bar_9, ackData.battery, LV_ANIM_OFF);

    if (sendSuccessCount > 0)
    {
      lv_obj_set_style_text_color(guider_ui.main_label_6, lv_color_hex(0x26B08C), LV_PART_MAIN|LV_STATE_DEFAULT);
      lv_label_set_text(guider_ui.main_label_6,"Connected");
    }
    else
    {      
      lv_obj_set_style_text_color(guider_ui.main_label_6, lv_color_hex(0xff5b74), LV_PART_MAIN|LV_STATE_DEFAULT);
      lv_label_set_text(guider_ui.main_label_6,"Not connected");
    }
    

    lv_timer_handler();
    vTaskDelay(5);  
    
}
}



void keypad_init()
{
  pinMode(KEY_R1_PIN,INPUT_PULLUP);
  pinMode(KEY_R2_PIN,INPUT_PULLUP);
  pinMode(KEY_R3_PIN,INPUT_PULLUP);
}


// int read_key(void)
// {
//     if(digitalRead(KEY_R1_PIN)==0)
//     {
//         return 2;
//     }
//     else if (digitalRead(KEY_R3_PIN)==0)
//     {
//         return 1;  
//     }
//      else if (digitalRead(KEY_R2_PIN)==0)
//     {
//         return 5; 
//     }
//     else
//     {
//         return -1;
//     }
// }

static uint32_t keypad_get_key(void)
{
    if(digitalRead(KEY_R1_PIN)==0)
    {
        return 2;
    }
    else if (digitalRead(KEY_R3_PIN)==0)
    {
        return 5;  
    }
     else if (digitalRead(KEY_R2_PIN)==0)
    {
        return 1; 
    }
    else
    {
        return 0;
    }

    return 0;
}

static void keypad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    /*Get the current x and y coordinates*/
    // mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the a key is pressed and save the pressed key*/
    uint32_t act_key = keypad_get_key();
    // Serial.println(act_key);
    if(act_key != 0) {
        data->state = LV_INDEV_STATE_PRESSED;

        /*Translate the keys to LVGL control characters according to your key definitions*/
        switch(act_key) {
            case 1:
                act_key = LV_KEY_NEXT;
                break;
            case 2:
                act_key = LV_KEY_PREV;
                break;
            case 3:
                act_key = LV_KEY_LEFT;
                break;
            case 4:
                act_key = LV_KEY_RIGHT;
                break;
            case 5:
                act_key = LV_KEY_ENTER;
                break;
        }

        last_key = act_key;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->key = last_key;
}

#endif

#if 1
/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>
#include <TFT_eSPI.h>
#include "App.h"
#include "Pages/StartUp/StartUpView.h"
#include "Version.h"
#include "Resource/ResourcePool.h"
#include "main.h"
#include "HAL/HAL.h"

#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <FastLED.h>
#include <Preferences.h>
/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 *You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
 as the examples and demos are now part of the main LVGL library. */

/*Change to your screen resolution*/

static const uint16_t screenWidth  = 240;
static const uint16_t screenHeight = 240;
#define SCREEN_BUFFER_SIZE ((screenWidth * screenHeight)/2)

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */

void lv_port_indev_init(void);

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp_drv );
}

static void disp_wait_cb(lv_disp_drv_t* disp_drv)
{
   // __wfi();
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_drv, lv_indev_data_t * data )
{
    uint16_t touchX, touchY;

    // bool touched = tft.getTouch( &touchX, &touchY, 600 );

    // if( !touched )
    // {
    //     data->state = LV_INDEV_STATE_REL;
    // }
    // else
    // {
    //     data->state = LV_INDEV_STATE_PR;

    //     /*Set the coordinates*/
    //     data->point.x = touchX;
    //     data->point.y = touchY;

    //     Serial.print( "Data x " );
    //     Serial.println( touchX );

    //     Serial.print( "Data y " );
    //     Serial.println( touchY );
    // }
}
lv_anim_timeline_t* anim_timeline;
lv_obj_t* ucont;
lv_obj_t* labelLogo;

// 电源管理任务
void PowerTask(void *pvParameters) {
  static uint16_t count = 0;
  while (1) {
    HAL::HAL_Update();
    delay(5);
  }
}

void setup()
{
  Serial.begin( 115200 ); /* prepare for possible serial debug */

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println( LVGL_Arduino );
  Serial.println( "I am LVGL_Arduino" );

  Serial.begin(115200);

  lv_init();
   
  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);

  static lv_color_t lv_disp_buf1[SCREEN_BUFFER_SIZE];
  static lv_color_t lv_disp_buf2[SCREEN_BUFFER_SIZE];

  static lv_disp_draw_buf_t disp_buf;
  lv_disp_draw_buf_init(&disp_buf, lv_disp_buf1, lv_disp_buf2, SCREEN_BUFFER_SIZE);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.wait_cb = disp_wait_cb;
  disp_drv.draw_buf = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  HAL::HAL_Init();
  lv_port_indev_init();
  App_Init();

  xTaskCreate(PowerTask, "PowerTask", 4096, NULL, 1, NULL);

  Serial.println( "Setup done" );
}

void loop()
{
   
  lv_timer_handler(); /* let the GUI do its work */
  delay( 5 );
}

void keypad_init()
{
  pinMode(KEY_R1_PIN,INPUT_PULLUP);
  pinMode(KEY_R2_PIN,INPUT_PULLUP);
  pinMode(KEY_R3_PIN,INPUT_PULLUP);
}


static uint32_t keypad_get_key(void)
{
    if(digitalRead(KEY_R1_PIN)==0)
    {
        return 2;
    }
    else if (digitalRead(KEY_R3_PIN)==0)
    {
        return 5;  
    }
     else if (digitalRead(KEY_R2_PIN)==0)
    {
        return 1; 
    }
    else
    {
        return 0;
    }

    return 0;
}

static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    /*Get the current x and y coordinates*/
    // mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the a key is pressed and save the pressed key*/
    uint32_t act_key = keypad_get_key();
    // Serial.println(act_key);
    if(act_key != 0) {
        data->state = LV_INDEV_STATE_PRESSED;

        /*Translate the keys to LVGL control characters according to your key definitions*/
        switch(act_key) {
            case 1:
                act_key = LV_KEY_NEXT;
                break;
            case 2:
                act_key = LV_KEY_PREV;
                break;
            case 3:
                act_key = LV_KEY_LEFT;
                break;
            case 4:
                act_key = LV_KEY_RIGHT;
                break;
            case 5:
                act_key = LV_KEY_ENTER;
                break;
        }

        last_key = act_key;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->key = last_key;
}

lv_group_t* group;
lv_indev_drv_t indev_drv;

void lv_port_indev_init(void)
{

    /*------------------
     * Encoder
     * -----------------*/

    /*Initialize your encoder if you have*/
    keypad_init();

    /*Register a encoder input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    lv_indev_t* indev = lv_indev_drv_register(&indev_drv);
    
    group = lv_group_create();
    lv_indev_set_group(indev, group);
    lv_group_set_default(group);

    /* Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     * add objects to the group with `lv_group_add_obj(group, obj)`
     * and assign this input device to group to navigate in it:
     * `lv_indev_set_group(indev_encoder, group);` */
}


#endif
