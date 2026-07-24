#include "StartUpView.h"
#include "Version.h"
#include "Arduino.h"
using namespace Page;

#define COLOR_ORANGE    lv_color_hex(0xff931e)

void StartupView::Create(lv_obj_t* root)
{
    lv_obj_t* cont = lv_obj_create(root);
    lv_obj_remove_style_all(cont);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, 200, 100);
    lv_obj_set_style_border_color(cont, COLOR_ORANGE, 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(cont, 3, 0);
    lv_obj_set_style_border_post(cont, true, 0);
    lv_obj_center(cont);
    ui.cont = cont;


        /* icon */
    lv_obj_t* icon = lv_obj_create(cont);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_align(icon, LV_ALIGN_LEFT_MID, 0);

    lv_obj_set_flex_flow(icon, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        icon,
        LV_FLEX_ALIGN_SPACE_EVENLY,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_center(icon);

    
    const char* img_src = "logo";
    lv_obj_t* img = lv_img_create(icon);
    lv_obj_enable_style_refresh(false);
    lv_img_set_src(img, ResourcePool::GetImage(img_src));

    lv_obj_t* label = lv_label_create(icon);
    lv_obj_set_style_text_font(label, ResourcePool::GetFont("agencyb_17"), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "POCKET-remote");

    ui.icon = icon;

    ui.anim_timeline = lv_anim_timeline_create();

#define ANIM_DEF(start_time, obj, attr, start, end) \
     {start_time, obj, LV_ANIM_EXEC(attr), start, end, 500, lv_anim_path_ease_out, true}

    lv_anim_timeline_wrapper_t wrapper[] =
    {
        ANIM_DEF(0, ui.cont, width, 0, lv_obj_get_style_width(ui.cont, 0)),
        ANIM_DEF(500, ui.icon, y, lv_obj_get_style_height(ui.cont, 0), lv_obj_get_y( ui.icon)),
        LV_ANIM_TIMELINE_WRAPPER_END
    };

    lv_anim_timeline_add_wrapper(ui.anim_timeline, wrapper);
}

void StartupView::Delete()
{
    if(ui.anim_timeline)
    {
        lv_anim_timeline_del(ui.anim_timeline);
        ui.anim_timeline = nullptr;
    }
}
