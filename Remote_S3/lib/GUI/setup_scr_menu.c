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



void setup_scr_menu(lv_ui *ui)
{
    //Write codes menu
    ui->menu = lv_obj_create(NULL);
    lv_obj_set_size(ui->menu, 240, 240);
    lv_obj_set_scrollbar_mode(ui->menu, LV_SCROLLBAR_MODE_OFF);

    //Write style for menu, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->menu, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->menu, lv_color_hex(0x098D6B), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->menu, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes menu_menu_1
    ui->menu_menu_1 = lv_menu_create(ui->menu);
    lv_obj_set_pos(ui->menu_menu_1, 0, 0);
    lv_obj_set_size(ui->menu_menu_1, 240, 240);
    lv_obj_set_scrollbar_mode(ui->menu_menu_1, LV_SCROLLBAR_MODE_OFF);

    //Create sidebar page for menu menu_menu_1
    ui->menu_menu_1_sidebar_page = lv_menu_page_create(ui->menu_menu_1, "menu");
    lv_menu_set_sidebar_page(ui->menu_menu_1, ui->menu_menu_1_sidebar_page);
    lv_obj_set_scrollbar_mode(ui->menu_menu_1_sidebar_page, LV_SCROLLBAR_MODE_OFF);

    //Create subpage for menu_menu_1
    lv_obj_t * menu_menu_1_subpage_1 = lv_menu_page_create(ui->menu_menu_1, NULL);
    ui->menu_menu_1_subpage_1_cont = lv_menu_cont_create(menu_menu_1_subpage_1);
    lv_obj_set_layout(ui->menu_menu_1_subpage_1_cont, LV_LAYOUT_NONE);
    ui->menu_menu_1_cont_1 = lv_menu_cont_create(ui->menu_menu_1_sidebar_page);
    ui->menu_menu_1_label_1 = lv_label_create(ui->menu_menu_1_cont_1);
    lv_label_set_text(ui->menu_menu_1_label_1, "model");
    lv_obj_set_size(ui->menu_menu_1_label_1, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->menu_menu_1_label_1, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(menu_menu_1_subpage_1, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->menu_menu_1, ui->menu_menu_1_cont_1, menu_menu_1_subpage_1);

    //Create subpage for menu_menu_1
    lv_obj_t * menu_menu_1_subpage_2 = lv_menu_page_create(ui->menu_menu_1, NULL);
    ui->menu_menu_1_subpage_2_cont = lv_menu_cont_create(menu_menu_1_subpage_2);
    lv_obj_set_layout(ui->menu_menu_1_subpage_2_cont, LV_LAYOUT_NONE);
    ui->menu_menu_1_cont_2 = lv_menu_cont_create(ui->menu_menu_1_sidebar_page);
    ui->menu_menu_1_label_2 = lv_label_create(ui->menu_menu_1_cont_2);
    lv_label_set_text(ui->menu_menu_1_label_2, "calibrate");
    lv_obj_set_size(ui->menu_menu_1_label_2, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->menu_menu_1_label_2, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(menu_menu_1_subpage_2, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->menu_menu_1, ui->menu_menu_1_cont_2, menu_menu_1_subpage_2);

    //Create subpage for menu_menu_1
    lv_obj_t * menu_menu_1_subpage_3 = lv_menu_page_create(ui->menu_menu_1, NULL);
    ui->menu_menu_1_subpage_3_cont = lv_menu_cont_create(menu_menu_1_subpage_3);
    lv_obj_set_layout(ui->menu_menu_1_subpage_3_cont, LV_LAYOUT_NONE);
    ui->menu_menu_1_cont_3 = lv_menu_cont_create(ui->menu_menu_1_sidebar_page);
    ui->menu_menu_1_label_3 = lv_label_create(ui->menu_menu_1_cont_3);
    lv_label_set_text(ui->menu_menu_1_label_3, "game");
    lv_obj_set_size(ui->menu_menu_1_label_3, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->menu_menu_1_label_3, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(menu_menu_1_subpage_3, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->menu_menu_1, ui->menu_menu_1_cont_3, menu_menu_1_subpage_3);

    //Create subpage for menu_menu_1
    lv_obj_t * menu_menu_1_subpage_4 = lv_menu_page_create(ui->menu_menu_1, NULL);
    ui->menu_menu_1_subpage_4_cont = lv_menu_cont_create(menu_menu_1_subpage_4);
    lv_obj_set_layout(ui->menu_menu_1_subpage_4_cont, LV_LAYOUT_NONE);
    ui->menu_menu_1_cont_4 = lv_menu_cont_create(ui->menu_menu_1_sidebar_page);
    ui->menu_menu_1_label_4 = lv_label_create(ui->menu_menu_1_cont_4);
    lv_label_set_text(ui->menu_menu_1_label_4, "exit");
    lv_obj_set_size(ui->menu_menu_1_label_4, LV_PCT(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(ui->menu_menu_1_label_4, LV_LABEL_LONG_CLIP);
    lv_obj_set_scrollbar_mode(menu_menu_1_subpage_4, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_load_page_event(ui->menu_menu_1, ui->menu_menu_1_cont_4, menu_menu_1_subpage_4);

    //Write style for menu_menu_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->menu_menu_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->menu_menu_1, lv_color_hex(0xeaeaea), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->menu_menu_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->menu_menu_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->menu_menu_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for menu_menu_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_margin_hor(ui->menu_menu_1_sidebar_page, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_margin_ver(ui->menu_menu_1_sidebar_page, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->menu_menu_1_sidebar_page, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->menu_menu_1_sidebar_page, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->menu_menu_1_sidebar_page, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->menu_menu_1_sidebar_page, lv_color_hex(0xf6f6f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->menu_menu_1_sidebar_page, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_menu_menu_1_extra_option_btns_main_default
    static lv_style_t style_menu_menu_1_extra_option_btns_main_default;
    ui_init_style(&style_menu_menu_1_extra_option_btns_main_default);

    lv_style_set_text_color(&style_menu_menu_1_extra_option_btns_main_default, lv_color_hex(0x151212));
    lv_style_set_text_font(&style_menu_menu_1_extra_option_btns_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_menu_menu_1_extra_option_btns_main_default, 255);
    lv_style_set_text_align(&style_menu_menu_1_extra_option_btns_main_default, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_top(&style_menu_menu_1_extra_option_btns_main_default, 10);
    lv_style_set_pad_bottom(&style_menu_menu_1_extra_option_btns_main_default, 10);
    lv_obj_add_style(ui->menu_menu_1_cont_4, &style_menu_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->menu_menu_1_cont_3, &style_menu_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->menu_menu_1_cont_2, &style_menu_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->menu_menu_1_cont_1, &style_menu_menu_1_extra_option_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_CHECKED for &style_menu_menu_1_extra_option_btns_main_checked
    static lv_style_t style_menu_menu_1_extra_option_btns_main_checked;
    ui_init_style(&style_menu_menu_1_extra_option_btns_main_checked);

    lv_style_set_text_color(&style_menu_menu_1_extra_option_btns_main_checked, lv_color_hex(0x9ab700));
    lv_style_set_text_font(&style_menu_menu_1_extra_option_btns_main_checked, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_menu_menu_1_extra_option_btns_main_checked, 255);
    lv_style_set_text_align(&style_menu_menu_1_extra_option_btns_main_checked, LV_TEXT_ALIGN_CENTER);
    lv_style_set_border_width(&style_menu_menu_1_extra_option_btns_main_checked, 0);
    lv_style_set_radius(&style_menu_menu_1_extra_option_btns_main_checked, 5);
    lv_style_set_bg_opa(&style_menu_menu_1_extra_option_btns_main_checked, 60);
    lv_style_set_bg_color(&style_menu_menu_1_extra_option_btns_main_checked, lv_color_hex(0x19a5ff));
    lv_style_set_bg_grad_dir(&style_menu_menu_1_extra_option_btns_main_checked, LV_GRAD_DIR_NONE);
    lv_obj_add_style(ui->menu_menu_1_cont_4, &style_menu_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->menu_menu_1_cont_3, &style_menu_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->menu_menu_1_cont_2, &style_menu_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);
    lv_obj_add_style(ui->menu_menu_1_cont_1, &style_menu_menu_1_extra_option_btns_main_checked, LV_PART_MAIN|LV_STATE_CHECKED);

    //Write style state: LV_STATE_DEFAULT for &style_menu_menu_1_extra_main_title_main_default
    static lv_style_t style_menu_menu_1_extra_main_title_main_default;
    ui_init_style(&style_menu_menu_1_extra_main_title_main_default);

    lv_style_set_text_color(&style_menu_menu_1_extra_main_title_main_default, lv_color_hex(0x41485a));
    lv_style_set_text_font(&style_menu_menu_1_extra_main_title_main_default, &lv_font_montserratMedium_14);
    lv_style_set_text_opa(&style_menu_menu_1_extra_main_title_main_default, 255);
    lv_style_set_text_align(&style_menu_menu_1_extra_main_title_main_default, LV_TEXT_ALIGN_CENTER);
    lv_style_set_bg_opa(&style_menu_menu_1_extra_main_title_main_default, 0);
    lv_style_set_pad_hor(&style_menu_menu_1_extra_main_title_main_default, 5);
    lv_style_set_pad_ver(&style_menu_menu_1_extra_main_title_main_default, 5);
    lv_menu_t * menu_menu_1_menu= (lv_menu_t *)ui->menu_menu_1;
    // lv_obj_t * menu_menu_1_title = menu_menu_1_menu->sidebar_header_title;
    // lv_obj_set_size(menu_menu_1_title, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(lv_menu_get_sidebar_header(ui->menu_menu_1), &style_menu_menu_1_extra_main_title_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);









    //The custom code of menu.


    //Update current screen layout.
    lv_obj_update_layout(ui->menu);

}
