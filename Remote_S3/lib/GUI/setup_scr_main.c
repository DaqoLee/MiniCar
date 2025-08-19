/*
* Copyright 2025 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_main(lv_ui *ui)
{
    //Write codes main
    ui->main = lv_obj_create(NULL);
    lv_obj_set_size(ui->main, 240, 240);
    lv_obj_set_scrollbar_mode(ui->main, LV_SCROLLBAR_MODE_OFF);

    //Write style for main, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes main_slider_2
    ui->main_slider_2 = lv_slider_create(ui->main);
    lv_obj_set_pos(ui->main_slider_2, 10, 177);
    lv_obj_set_size(ui->main_slider_2, 100, 6);
    lv_slider_set_range(ui->main_slider_2, -60, 60);
    lv_slider_set_mode(ui->main_slider_2, LV_SLIDER_MODE_SYMMETRICAL);
    lv_slider_set_value(ui->main_slider_2, -40, LV_ANIM_OFF);

    //Write style for main_slider_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_2, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_2, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_2, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->main_slider_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->main_slider_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for main_slider_2, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_2, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_2, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_2, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_2, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for main_slider_2, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_2, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_2, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_2, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_2, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes main_slider_1
    ui->main_slider_1 = lv_slider_create(ui->main);
    lv_obj_set_pos(ui->main_slider_1, 57, 130);
    lv_obj_set_size(ui->main_slider_1, 6, 100);
    lv_slider_set_range(ui->main_slider_1, -60, 60);
    lv_slider_set_mode(ui->main_slider_1, LV_SLIDER_MODE_SYMMETRICAL);
    lv_slider_set_value(ui->main_slider_1, -20, LV_ANIM_OFF);

    //Write style for main_slider_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_1, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_1, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->main_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->main_slider_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for main_slider_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_1, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for main_slider_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_1, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_1, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes main_slider_4
    ui->main_slider_4 = lv_slider_create(ui->main);
    lv_obj_set_pos(ui->main_slider_4, 130, 177);
    lv_obj_set_size(ui->main_slider_4, 100, 6);
    lv_slider_set_range(ui->main_slider_4, -60, 60);
    lv_slider_set_mode(ui->main_slider_4, LV_SLIDER_MODE_SYMMETRICAL);
    lv_slider_set_value(ui->main_slider_4, -40, LV_ANIM_OFF);

    //Write style for main_slider_4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_4, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_4, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_4, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_4, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->main_slider_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->main_slider_4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for main_slider_4, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_4, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_4, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_4, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_4, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for main_slider_4, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_4, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_4, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_4, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_4, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes main_slider_3
    ui->main_slider_3 = lv_slider_create(ui->main);
    lv_obj_set_pos(ui->main_slider_3, 177, 130);
    lv_obj_set_size(ui->main_slider_3, 6, 100);
    lv_slider_set_range(ui->main_slider_3, -60, 60);
    lv_slider_set_mode(ui->main_slider_3, LV_SLIDER_MODE_SYMMETRICAL);
    lv_slider_set_value(ui->main_slider_3, -20, LV_ANIM_OFF);

    //Write style for main_slider_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_3, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_3, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_3, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(ui->main_slider_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->main_slider_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for main_slider_3, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_3, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_3, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_3, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_3, 8, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write style for main_slider_3, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_slider_3, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_slider_3, lv_color_hex(0x2195f6), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_slider_3, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_slider_3, 8, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes main_led_1
    ui->main_led_1 = lv_led_create(ui->main);
    lv_obj_set_pos(ui->main_led_1, 4, 3);
    lv_obj_set_size(ui->main_led_1, 10, 10);
    lv_led_set_brightness(ui->main_led_1, 255);
    lv_led_set_color(ui->main_led_1, lv_color_hex(0x00a1b5));

    //Write codes main_bar_1
    ui->main_bar_1 = lv_bar_create(ui->main);
    lv_obj_set_pos(ui->main_bar_1, 200, 0);
    lv_obj_set_size(ui->main_bar_1, 40, 20);
    lv_obj_set_style_anim_duration(ui->main_bar_1, 1000, 0);
    lv_bar_set_mode(ui->main_bar_1, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(ui->main_bar_1, 0, 100);
    lv_bar_set_value(ui->main_bar_1, 80, LV_ANIM_OFF);

    //Write style for main_bar_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_bar_1, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_bar_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_bar_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_bar_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->main_bar_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for main_bar_1, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->main_bar_1, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->main_bar_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->main_bar_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->main_bar_1, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    //Write codes main_line_1
    ui->main_line_1 = lv_line_create(ui->main);
    lv_obj_set_pos(ui->main_line_1, 0, 120);
    lv_obj_set_size(ui->main_line_1, 240, 2);
    static lv_point_precise_t main_line_1[] = {{0, 0},{240, 0}};
    lv_line_set_points(ui->main_line_1, main_line_1, 2);

    //Write style for main_line_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->main_line_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->main_line_1, lv_color_hex(0x2F92DA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->main_line_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->main_line_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes main_line_2
    ui->main_line_2 = lv_line_create(ui->main);
    lv_obj_set_pos(ui->main_line_2, 120, 120);
    lv_obj_set_size(ui->main_line_2, 2, 120);
    static lv_point_precise_t main_line_2[] = {{0, 0},{0, 120}};
    lv_line_set_points(ui->main_line_2, main_line_2, 2);

    //Write style for main_line_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_line_width(ui->main_line_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->main_line_2, lv_color_hex(0x2F92DA), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->main_line_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_rounded(ui->main_line_2, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of main.


    //Update current screen layout.
    lv_obj_update_layout(ui->main);

}
