

#if 1

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


#define MAX_VALUE 100
#define MIN_VALUE 0

// void CAN_RX_Task(void *pvParameters);//void *pvParameters

// void task(void*p){
//     while(1){

//       vTaskDelay(1500);
//     }
// }

TFT_eSPI tft = TFT_eSPI();
lv_display_t *display;

static lv_color_t buf1[TFT_WIDTH * 10];

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



void setup() {


    Serial.begin(115200);
    pinMode(BL, OUTPUT);
    digitalWrite(BL, HIGH);

    // pinMode(SPI_CS1, OUTPUT);
    // digitalWrite(SPI_CS1, LOW);

    // pinMode(SPI_CS2, OUTPUT);
    // digitalWrite(SPI_CS2, LOW);

    // SPI.begin(2, -1, 1, -1);

    tft.begin();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
    // tft.setFreeFont(FF18);
    // tft.setSwapBytes(true); // We need to swap the colour bytes (endianess)

    // tft.setCursor(15, 10);
    // tft.setTextFont(2);
    // tft.setTextSize(1);
    // tft.setTextColor(TFT_GREEN, TFT_BLACK);
    // tft.printf("ABCDEFGH");

    lv_init();
    display = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    lv_display_set_flush_cb(display, my_flush_cb);
    lv_display_set_buffers(display, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // LV_FONT_DECLARE(lv_font_montserrat_14)

    // lv_obj_t *btn = lv_button_create(lv_screen_active());
    // lv_obj_center(btn);
    // lv_obj_t *label = lv_label_create(btn);
    // lv_label_set_text(label, "ABCD");
    // lv_obj_center(label);
    // lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * slider;
    slider = lv_slider_create(lv_screen_active());
    lv_obj_center(slider);

    lv_slider_set_mode(slider, LV_SLIDER_MODE_RANGE);
    lv_slider_set_range(slider, MIN_VALUE, MAX_VALUE);
    lv_slider_set_value(slider, 70, LV_ANIM_OFF);
    lv_slider_set_left_value(slider, 20, LV_ANIM_OFF);
    lv_obj_set_size(slider, 5, 50);
   // lv_bar_set_orientation(slider,LV_BAR_ORIENTATION_VERTICAL);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(slider);


    lv_obj_t * slider1;
    slider1 = lv_slider_create(lv_screen_active());
    lv_obj_center(slider1);

    lv_slider_set_mode(slider1, LV_SLIDER_MODE_RANGE);
    lv_slider_set_range(slider1, MIN_VALUE, MAX_VALUE);
    lv_slider_set_value(slider1, 70, LV_ANIM_OFF);
    lv_slider_set_left_value(slider1, 20, LV_ANIM_OFF);
    lv_obj_set_size(slider1, 100, 10);
   // lv_bar_set_orientation(slider,LV_BAR_ORIENTATION_VERTICAL);
    lv_obj_add_event_cb(slider1, slider1_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(slider1);


    Serial.print("inited");

    // xTaskCreatePinnedToCore(CAN_RX_Task, "CAN", 8192, NULL, 1, NULL, 0);
    // xTaskCreate(task,"stepper",4096,NULL,10,NULL);

}


float speed = 0;
float roll, pitch, yaw;

float gyroX;
float gyroY;
float gyroZ;

void refresh() {
    lv_timer_handler(); // ??LVGL??
    vTaskDelay(5);           // ?????
}
void loop() {
 

    // tft.setCursor(15, 10);
    // tft.setTextFont(2);
    // tft.setTextSize(1);
    // tft.setTextColor(TFT_GREEN, TFT_BLACK);
    // tft.printf("A");
refresh();

}




#endif
