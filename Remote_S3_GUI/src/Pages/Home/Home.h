#ifndef __HOME_PRESENTER_H
#define __HOME_PRESENTER_H

#include "HomeView.h"
#include "HomeModel.h"

namespace Page
{

class Home : public PageBase
{
public:
    Home();
    virtual ~Home();

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
    typedef enum
    {
        RECORD_STATE_READY,
        RECORD_STATE_RUN,
        RECORD_STATE_PAUSE,
        RECORD_STATE_STOP
    } RecordState_t;

private:
    void Update();
    void AttachEvent(lv_obj_t* obj);
    static void onTimerUpdate(lv_timer_t* timer);
    static void onEvent(lv_event_t* event);
    void onBtnClicked(lv_obj_t* btn);
    void onRecord(bool longPress);
    void SetBtnRecImgSrc(const char* srcName);

private:
    HomeView View;
    HomeModel Model;
    lv_timer_t* timer;
    RecordState_t recState;
    lv_obj_t* lastFocus;
};

}

#endif
