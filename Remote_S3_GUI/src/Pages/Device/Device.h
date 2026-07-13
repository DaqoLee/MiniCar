#ifndef __DEVICE_PRESENTER_H
#define __DEVICE_PRESENTER_H

#include "DeviceView.h"
#include "DeviceModel.h"

namespace Page
{

class Device : public PageBase
{
public:
    Device();
    virtual ~Device();

    virtual void onCustomAttrConfig();
    virtual void onViewLoad();
    virtual void onViewDidLoad();
    virtual void onViewWillAppear();
    virtual void onViewDidAppear();
    virtual void onViewWillDisappear();
    virtual void onViewDidDisappear();
    virtual void onViewUnload();
    virtual void onViewDidUnload();

private:
    void Update();
    void RebuildList();
    void AttachEvent(lv_obj_t* obj);
    static void onTimerUpdate(lv_timer_t* timer);
    static void onEvent(lv_event_t* event);
    void onBtnClicked(lv_obj_t* btn);
private:
    DeviceView View;
    DeviceModel Model;
    lv_timer_t* timer;
    lv_obj_t* lastFocus;
};

}

#endif
