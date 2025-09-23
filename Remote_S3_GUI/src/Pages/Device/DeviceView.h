#ifndef __DEVICE_VIEW_H
#define __DEVICE_VIEW_H

#include "../Page.h"

namespace Page
{

class DeviceView
{

public:
    typedef struct
    {
        lv_obj_t* cont;
        lv_obj_t* lableValue;
        lv_obj_t* lableUnit;
    } SubInfo_t;

public:
    struct
    {
        struct
        {
            lv_obj_t* cont;
            // lv_obj_t* labelSpeed;
            // lv_obj_t* labelUint;
            lv_obj_t* labelStatus;
            lv_obj_t* batteryBarLeft;
            lv_obj_t* batteryBarRight;
            SubInfo_t labelInfoGrp[4];
        } topInfo;

          struct
        {
            lv_obj_t* cont;
            lv_obj_t* labelSpeed;
            lv_obj_t* labelUint;
        } middleInfo;      

        struct
        {
            lv_obj_t* cont;
            lv_obj_t* btnLeft;
            lv_obj_t* btnRight;
            SubInfo_t labelInfoGrp[4];
        } bottomInfo;

        struct
        {
            lv_obj_t* cont;
            lv_obj_t* btnMap;
            lv_obj_t* btnRec;
            lv_obj_t* btnMenu;
        } btnCont;

        lv_anim_timeline_t* anim_timeline;
    } ui;

    void Create(lv_obj_t* root);
    void Delete();
    void AppearAnimStart(bool reverse = false);

private:
    void TopInfo_Create(lv_obj_t* par);
    void BottomInfo_Create(lv_obj_t* par);
    void SubInfoGrp_Create(lv_obj_t* par, SubInfo_t* info, const char* unitText);
    void BtnCont_Create(lv_obj_t* par);
    lv_obj_t* Btn_Create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs);
    lv_obj_t* JoyBtn_Create(lv_obj_t* par, lv_coord_t x_ofs);
};

}

#endif // !__VIEW_H
