#ifndef __DEVICE_VIEW_H
#define __DEVICE_VIEW_H

#include "../Page.h"

namespace Page
{

class DeviceView
{
public:
    void Create(lv_obj_t* root);
    void Delete();

public:
    typedef struct
    {
        lv_obj_t* cont;
        lv_obj_t* icon;
        lv_obj_t* labelInfo;
        lv_obj_t* labelData;
    } item_t;

#define MAX_DEVICE_ITEMS 12

    struct
    {
        item_t items[MAX_DEVICE_ITEMS];
        int itemCount;
    } ui;

public:
    void RebuildItems(int count);
    void GroupReset();
    void SetScrollToY(lv_obj_t* obj, lv_coord_t y, lv_anim_enable_t en);
    static void onFocus(lv_group_t* g);

private:
    struct
    {
        lv_style_t icon;
        lv_style_t focus;
        lv_style_t info;
        lv_style_t data;
    } style;

private:
    void Group_Init();
    void Style_Init();
    void Style_Reset();
    void Item_Create(
        item_t* item,
        lv_obj_t* par,
        const char* name,
        const char* img_src,
        const char* infos
    );
};

}

#endif
