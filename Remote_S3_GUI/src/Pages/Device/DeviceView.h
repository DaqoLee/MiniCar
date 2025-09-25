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

    struct
    {
        item_t home;
        item_t calibrate;
        item_t pair;
        item_t device;
        item_t battery;
        item_t system;
    } ui;

public:
    void SetSport(
        float trip,
        const char* time,
        float maxSpd
    );
    void SetGPS(
        float lat,
        float lng,
        float alt,
        const char* utc,
        float course,
        float speed
    );
    void SetMAG(
        float dir,
        int x,
        int y,
        int z
    );
    void SetIMU(
        int step,
        const char* info
    );
    void SetRTC(
        const char* dateTime
    );
    void SetBattery(
        int usage,
        float voltage,
        const char* state
    );
    void SetSystem(
        const char* firmVer,
        const char* authorName,
        const char* lvglVer,
        const char* bootTime,
        const char* compilerName,
        const char* bulidTime
    );

    void SetScrollToY(lv_obj_t* obj, lv_coord_t y, lv_anim_enable_t en);
    void SetScrollToX(lv_obj_t* obj, lv_coord_t x, lv_anim_enable_t en);
    static void onFocus(lv_group_t* e);

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

#endif // !__VIEW_H
