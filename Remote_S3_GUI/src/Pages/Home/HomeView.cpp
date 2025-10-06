#include "HomeView.h"
#include <stdarg.h>
#include <stdio.h>

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

using namespace Page;

void HomeView::Create(lv_obj_t* root)
{
   
    TopInfo_Create(root);
    BottomInfo_Create(root);
    // BtnCont_Create(root);

    ui.anim_timeline = lv_anim_timeline_create();

#define ANIM_DEF(start_time, obj, attr, start, end) \
    {start_time, obj, LV_ANIM_EXEC(attr), start, end, 500, lv_anim_path_ease_out, true}

#define ANIM_OPA_DEF(start_time, obj) \
    ANIM_DEF(start_time, obj, opa_scale, LV_OPA_TRANSP, LV_OPA_COVER)

    lv_coord_t y_tar_top = lv_obj_get_y(ui.topInfo.cont);
    lv_coord_t y_tar_bottom = lv_obj_get_y(ui.bottomInfo.cont);
    // lv_coord_t h_tar_btn = lv_obj_get_height(ui.btnCont.btnRec);

    lv_anim_timeline_wrapper_t wrapper[] =
    {
        ANIM_DEF(0, ui.topInfo.cont, y, -100, y_tar_top),

        ANIM_DEF(200, ui.bottomInfo.cont, y, LV_VER_RES + lv_obj_get_height(ui.bottomInfo.cont), y_tar_bottom),
        ANIM_OPA_DEF(200, ui.bottomInfo.cont),

        // ANIM_DEF(500, ui.btnCont.btnMap, height, 0, h_tar_btn),
        // ANIM_DEF(600, ui.btnCont.btnRec, height, 0, h_tar_btn),
        // ANIM_DEF(700, ui.btnCont.btnMenu, height, 0, h_tar_btn),
        LV_ANIM_TIMELINE_WRAPPER_END
    };
    lv_anim_timeline_add_wrapper(ui.anim_timeline, wrapper);
}

void HomeView::Delete()
{
    if(ui.anim_timeline)
    {
        lv_anim_timeline_del(ui.anim_timeline);
        ui.anim_timeline = nullptr;
    }
}

void HomeView::TopInfo_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), 0);
    lv_obj_set_size(cont, LV_HOR_RES, 140);
    lv_obj_set_style_radius(cont, 10, 0);
    // lv_obj_set_y(cont, -40);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_set_flex_align(
        cont,
        LV_FLEX_ALIGN_SPACE_EVENLY,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_END
    );
    lv_obj_set_y(cont, -40);
    ui.topInfo.cont = cont;

    for (int i = 0; i < ARRAY_SIZE(ui.topInfo.barInfoGrp); i++)
    {
        BarInfoGrp_Create(
            cont,
            &(ui.topInfo.barInfoGrp[i])
        );
    }
}

void HomeView::BottomInfo_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);

    
    // lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x111111), 0);//lv_color_hex(0x111111)
    lv_obj_set_size(cont, LV_HOR_RES, 90);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // lv_obj_set_style_border_color(cont,  lv_color_hex(0xff931e), 0);//lv_color_hex(0xff931e)
    // lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_FULL, 0);
    // lv_obj_set_style_border_width(cont, 1, 0);
    // lv_obj_set_style_border_post(cont, true, 0);
     lv_obj_set_style_radius(cont, 15, 0);
    // lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 120);

    // lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);

    // lv_obj_set_flex_align(
    //     cont,
    //     LV_FLEX_ALIGN_SPACE_EVENLY,
    //     LV_FLEX_ALIGN_CENTER,
    //     LV_FLEX_ALIGN_CENTER
    // );

    ui.bottomInfo.cont = cont;

    ui.bottomInfo.btnLeft = JoyBtn_Create(cont, -70);
    ui.bottomInfo.btnRight = JoyBtn_Create(cont, 70);

}

void HomeView::BarInfoGrp_Create(lv_obj_t* par, BarInfo_t* info)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 100, 100);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        cont,
        LV_FLEX_ALIGN_SPACE_AROUND,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t* bar = lv_bar_create(cont);

    // lv_obj_set_pos(bar1, 57, 5);
    lv_obj_set_size(bar, 8, 90);
    // lv_obj_set_style_anim_duration(bar, 1000, 0);
    lv_bar_set_mode(bar, LV_BAR_MODE_SYMMETRICAL);
    lv_bar_set_range(bar, -100, 100);
    lv_bar_set_value(bar, 50, LV_ANIM_OFF);
 
    //Write style for main_bar_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(bar, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x26B08C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for main_bar_2, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x26B08C), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);


    info->bar[0] = bar;

    bar = lv_bar_create(cont);
    // lv_obj_set_pos(bar1, 57, 5);
    lv_obj_set_size(bar, 8, 90);
    // lv_obj_set_style_anim_duration(bar, 1000, 0);
    lv_bar_set_mode(bar, LV_BAR_MODE_SYMMETRICAL);
    lv_bar_set_range(bar, -100, 100);
    lv_bar_set_value(bar, 50, LV_ANIM_OFF);
 
    //Write style for main_bar_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(bar, 60, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x26B08C), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bar, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for main_bar_2, Part: LV_PART_INDICATOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x26B08C), LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 10, LV_PART_INDICATOR|LV_STATE_DEFAULT);

    info->bar[1] = bar;
    info->cont = cont;
}

lv_obj_t* HomeView::JoyBtn_Create(lv_obj_t* par, lv_coord_t x_ofs)
{
    lv_obj_t* obj = lv_obj_create(par);
    // lv_obj_set_pos(obj, 45, 45);
    lv_obj_set_size(obj, 30, 30);
    // obj_label = lv_label_create(obj);
    // lv_label_set_text(obj_label, "");
    // lv_label_set_long_mode(obj_label, LV_LABEL_LONG_WRAP);
    // lv_obj_align(obj_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, LV_STATE_DEFAULT);
    // lv_obj_set_width(obj_label, LV_PCT(100));
    lv_obj_align(obj, LV_ALIGN_CENTER, x_ofs, 0);
    //Write style for main_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff931e), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 15, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    return obj;
}

void HomeView::SubInfoGrp_Create(lv_obj_t* par, SubInfo_t* info, const char* unitText)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 100, 55);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        cont,
        LV_FLEX_ALIGN_SPACE_AROUND,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t* label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_32"), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    info->lableValue = label;

    // label = lv_label_create(cont);
    // lv_obj_set_style_text_font(label, ResourcePool::GetFont("bahnschrift_13"), 0);
    // lv_obj_set_style_text_color(label, lv_color_hex(0xb3b3b3), 0);
    // lv_label_set_text(label, unitText);
    // info->lableUnit = label;

    info->cont = cont;
}

void HomeView::BtnCont_Create(lv_obj_t* par)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, 40);
    lv_obj_align_to(cont, ui.topInfo.cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);

    /*lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_place(
        cont,
        LV_FLEX_PLACE_SPACE_AROUND,
        LV_FLEX_PLACE_CENTER,
        LV_FLEX_PLACE_CENTER
    );*/

    ui.btnCont.cont = cont;

    ui.btnCont.btnMap = Btn_Create(cont, ResourcePool::GetImage("locate"), -80);
    ui.btnCont.btnRec = Btn_Create(cont, ResourcePool::GetImage("start"), 0);
    ui.btnCont.btnMenu = Btn_Create(cont, ResourcePool::GetImage("menu"), 80);
}

lv_obj_t* HomeView::Btn_Create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs)
{
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 40, 31);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(obj, LV_ALIGN_CENTER, x_ofs, 0);
    lv_obj_set_style_bg_img_src(obj, img_src, 0);

    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_width(obj, 45, LV_STATE_PRESSED);
    lv_obj_set_style_height(obj, 25, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x666666), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xbbbbbb), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff931e), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(obj, 9, 0);

    static lv_style_transition_dsc_t tran;
    static const lv_style_prop_t prop[] = { LV_STYLE_WIDTH, LV_STYLE_HEIGHT, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(
        &tran,
        prop,
        lv_anim_path_ease_out,
        200,
        0,
        nullptr
    );
    lv_obj_set_style_transition(obj, &tran, LV_STATE_PRESSED);
    lv_obj_set_style_transition(obj, &tran, LV_STATE_FOCUSED);

    lv_obj_update_layout(obj);

    return obj;
}

void HomeView::AppearAnimStart(bool reverse)
{
    lv_anim_timeline_set_reverse(ui.anim_timeline, reverse);
    lv_anim_timeline_start(ui.anim_timeline);
}
