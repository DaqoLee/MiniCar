#include "DeviceView.h"
#include <cstdio>
#include <cstdlib>
using namespace Page;

#define ITEM_HEIGHT_MIN   80
#define ITEM_PAD          ((LV_VER_RES - ITEM_HEIGHT_MIN) / 2)

static lv_obj_t* g_root = nullptr;

void DeviceView::Create(lv_obj_t* root)
{
    g_root = root;
    lv_obj_set_style_pad_ver(root, ITEM_PAD, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        root,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER
    );
    Style_Init();
    ui.itemCount = 0;
    memset(ui.items, 0, sizeof(ui.items));
}

void DeviceView::Delete()
{
    // Delete all item containers
    for (int i = 0; i < ui.itemCount; i++) {
        if (ui.items[i].cont) {
            lv_obj_del(ui.items[i].cont);
        }
    }
    memset(ui.items, 0, sizeof(ui.items));
    ui.itemCount = 0;
    // Don't clear focus callback - other pages may need it
    Style_Reset();
}

void DeviceView::RebuildItems(int count)
{
    // Delete old items
    for (int i = 0; i < ui.itemCount; i++) {
        if (ui.items[i].cont) {
            lv_obj_del(ui.items[i].cont);
        }
    }
    memset(ui.items, 0, sizeof(ui.items));
    ui.itemCount = 0;
    
    if (count > MAX_DEVICE_ITEMS) count = MAX_DEVICE_ITEMS;
    if (count < 0) count = 0;
    
    // Create items directly on g_root
    for (int i = 0; i < count; i++) {
        if (i >= MAX_DEVICE_ITEMS) break;
        
        char idxStr[8] = {0};
        const char* img = "device";
        
        if (i < count - 2) {
            // Device entry
            snprintf(idxStr, sizeof(idxStr), "D%d", i + 1);
        } else if (i == count - 2) {
            // Pair entry
            snprintf(idxStr, sizeof(idxStr), "ADD");
            img = "pair";
        } else {
            // Back entry
            snprintf(idxStr, sizeof(idxStr), "BCK");
            img = "home";
        }
        
        Item_Create(
            &ui.items[ui.itemCount],
            g_root,
            idxStr,
            img,
            " "  // placeholder, updated later
        );
        ui.itemCount++;
    }
    
}

void DeviceView::GroupReset()
{
    lv_group_t* group = lv_group_get_default();
    if (!group) return;
    
    lv_group_set_wrap(group, true);
    lv_group_set_focus_cb(group, onFocus);
    
    lv_group_remove_all_objs(group);
    
    for (int i = ui.itemCount - 1; i >= 0; i--) {
        lv_group_add_obj(group, ui.items[i].icon);
    }
    
    if (ui.itemCount > 0) {
        lv_group_focus_obj(ui.items[0].icon);
    }
}

void DeviceView::SetScrollToY(lv_obj_t* obj, lv_coord_t y, lv_anim_enable_t en)
{
    lv_coord_t scroll_y = lv_obj_get_scroll_y(obj);
    lv_coord_t diff = -y + scroll_y;
    lv_obj_scroll_by(obj, 0, diff, en);
}

void DeviceView::onFocus(lv_group_t* g)
{
    lv_obj_t* icon = lv_group_get_focused(g);
    if (!icon) return;
    lv_obj_t* cont = lv_obj_get_parent(icon);
    if (!cont) return;
    lv_coord_t y = lv_obj_get_y(cont);
    lv_obj_scroll_to_y(lv_obj_get_parent(cont), y, LV_ANIM_ON);
}

void DeviceView::Style_Init()
{
    lv_style_init(&style.icon);
    lv_style_set_width(&style.icon, 220);
    lv_style_set_bg_color(&style.icon, lv_color_black());
    lv_style_set_bg_opa(&style.icon, LV_OPA_COVER);
    lv_style_set_text_font(&style.icon, ResourcePool::GetFont("bahnschrift_17"));
    lv_style_set_text_color(&style.icon, lv_color_white());

    lv_style_init(&style.focus);
    lv_style_set_width(&style.focus, 70);
    lv_style_set_border_side(&style.focus, LV_BORDER_SIDE_RIGHT);
    lv_style_set_border_width(&style.focus, 2);
    lv_style_set_border_color(&style.focus, lv_color_hex(0xff931e));

    static const lv_style_prop_t style_prop[] = {
        LV_STYLE_WIDTH,
        LV_STYLE_PROP_INV
    };
    static lv_style_transition_dsc_t trans;
    lv_style_transition_dsc_init(&trans, style_prop, lv_anim_path_overshoot, 200, 0, nullptr);
    lv_style_set_transition(&style.focus, &trans);
    lv_style_set_transition(&style.icon, &trans);

    lv_style_init(&style.info);
    lv_style_set_text_font(&style.info, ResourcePool::GetFont("bahnschrift_13"));
    lv_style_set_text_color(&style.info, lv_color_hex(0xcccccc));

    lv_style_init(&style.data);
    lv_style_set_text_font(&style.data, ResourcePool::GetFont("bahnschrift_13"));
    lv_style_set_text_color(&style.data, lv_color_white());
}

void DeviceView::Style_Reset()
{
    lv_style_reset(&style.icon);
    lv_style_reset(&style.info);
    lv_style_reset(&style.data);
    lv_style_reset(&style.focus);
}

void DeviceView::Item_Create(
    item_t* item,
    lv_obj_t* par,
    const char* name,
    const char* img_src,
    const char* infos
)
{
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(cont);
    lv_obj_set_width(cont, 220);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    item->cont = cont;

    /* icon */
    lv_obj_t* icon = lv_obj_create(cont);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(icon, &style.icon, 0);
    lv_obj_add_style(icon, &style.focus, LV_STATE_FOCUSED);
    lv_obj_set_style_align(icon, LV_ALIGN_LEFT_MID, 0);
    lv_obj_set_flex_flow(icon, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        icon,
        LV_FLEX_ALIGN_SPACE_AROUND,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t* img = lv_img_create(icon);
    lv_obj_enable_style_refresh(false);
    lv_img_set_src(img, ResourcePool::GetImage(img_src));

    lv_obj_t* label = lv_label_create(icon);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(label, name);
    item->icon = icon;

    /* infos */
    label = lv_label_create(cont);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(label, infos);
    lv_obj_add_style(label, &style.info, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 75, 0);
    item->labelInfo = label;

    /* datas */
    label = lv_label_create(cont);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(label, " ");
    lv_obj_add_style(label, &style.data, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 60, 0);
    item->labelData = label;

    lv_obj_move_foreground(icon);
    lv_obj_enable_style_refresh(true);

    lv_obj_update_layout(item->labelInfo);
    lv_coord_t height = lv_obj_get_height(item->labelInfo);
    height = LV_MAX(height, ITEM_HEIGHT_MIN);
    lv_obj_set_height(cont, height);
    lv_obj_set_height(icon, height);
}
