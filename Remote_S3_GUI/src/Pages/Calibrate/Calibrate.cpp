#include "Calibrate.h"
using namespace Page;

Calibrate::Calibrate()
    : recState(RECORD_STATE_READY)
    , lastFocus(nullptr)
{
}

Calibrate::~Calibrate()
{
}

void Calibrate::onCustomAttrConfig()
{
    SetCustomLoadAnimType(PageManager::LOAD_ANIM_NONE);
}

void Calibrate::onViewLoad()
{
    Model.Init();
    View.Create(_root);

    AttachEvent(_root);
    AttachEvent(View.ui.bottomInfo.cont);
    AttachEvent(View.ui.bottomInfo.btnLeft);
    AttachEvent(View.ui.bottomInfo.btnRight);
}

void Calibrate::onViewDidLoad()
{

}

void Calibrate::onViewWillAppear()
{
    lv_indev_wait_release(lv_indev_get_act());
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);

    lv_group_set_wrap(group, true);

    lv_group_add_obj(group, View.ui.bottomInfo.cont);
    lv_group_add_obj(group, View.ui.bottomInfo.btnLeft);
    lv_group_add_obj(group, View.ui.bottomInfo.btnRight);
    lv_group_focus_obj(View.ui.bottomInfo.cont);

    // if (lastFocus)
    // {
    //     lv_group_focus_obj(lastFocus);
    // }
    // else
    // {
    //     lv_group_focus_obj(View.ui.btnCont.btnRec);
    // }

    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_TRANSP);

    Update();

    View.AppearAnimStart();
}

void Calibrate::onViewDidAppear()
{
    timer = lv_timer_create(onTimerUpdate, 100, this);
}

void Calibrate::onViewWillDisappear()
{
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    lastFocus = lv_group_get_focused(group);
    // lv_group_remove_obj(View.ui.bottomInfo.cont);
    lv_group_remove_all_objs(group);
    lv_timer_del(timer);
    // View.AppearAnimStart(true);
}

void Calibrate::onViewDidDisappear()
{
}

void Calibrate::onViewUnload()
{
    View.Delete();
    Model.Deinit();
   
}

void Calibrate::onViewDidUnload()
{

}

void Calibrate::AttachEvent(lv_obj_t* obj)
{
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void Calibrate::Update()
{

    static uint16_t data[12] = {0}
    ;
    Model.GetCalibrateInfo(data);
    for (int i = 0; i < 12; i++)
    {
        lv_label_set_text_fmt(
        View.ui.topInfo.labelInfoGrp[i].lableValue,
        "%d",data[i]);   
    }

    
}

void Calibrate::onTimerUpdate(lv_timer_t* timer)
{
    Calibrate* instance = (Calibrate*)timer->user_data;

    instance->Update();
}

void Calibrate::onBtnClicked(lv_obj_t* btn)
{
    // if (btn == View.ui.btnCont.btnMap)
    // {
    //     _Manager->Push("Pages/LiveMap");
    // }
    if (btn == View.ui.bottomInfo.cont)
    {
        _Manager->Pop();
    }
    else if (btn == View.ui.bottomInfo.btnLeft)
    {
        LV_LOG_INFO("btnLeft");
        Model.SetRemoteMode(DataProc::OperationMode_t::MODE_CALIBRATE,DataProc::CalibrateMode_t::READ_MAX_MIN);
    }
    else if (btn == View.ui.bottomInfo.btnRight)
    {
        LV_LOG_INFO("btnRight");
        Model.SetRemoteMode(DataProc::OperationMode_t::MODE_CALIBRATE,DataProc::CalibrateMode_t::SAVE);
    }
    else
    {
        LV_LOG_INFO("btn");
    }
}

void Calibrate::onRecord(bool longPress)
{
    switch (recState)
    {
    case RECORD_STATE_READY:
        if (longPress)
        {
            if (!Model.GetGPSReady())
            {
                LV_LOG_WARN("GPS has not ready, can't start record");
                Model.PlayMusic("Error");
                return;
            }

            Model.PlayMusic("Connect");
            Model.RecorderCommand(Model.REC_START);
            SetBtnRecImgSrc("pause");
            recState = RECORD_STATE_RUN;
        }
        break;
    case RECORD_STATE_RUN:
        if (!longPress)
        {
            Model.PlayMusic("UnstableConnect");
            Model.RecorderCommand(Model.REC_PAUSE);
            SetBtnRecImgSrc("start");
            recState = RECORD_STATE_PAUSE;
        }
        break;
    case RECORD_STATE_PAUSE:
        if (longPress)
        {
            Model.PlayMusic("NoOperationWarning");
            SetBtnRecImgSrc("stop");
            Model.RecorderCommand(Model.REC_READY_STOP);
            recState = RECORD_STATE_STOP;
        }
        else
        {
            Model.PlayMusic("Connect");
            Model.RecorderCommand(Model.REC_CONTINUE);
            SetBtnRecImgSrc("pause");
            recState = RECORD_STATE_RUN;
        }
        break;
    case RECORD_STATE_STOP:
        if (longPress)
        {
            Model.PlayMusic("Disconnect");
            Model.RecorderCommand(Model.REC_STOP);
            SetBtnRecImgSrc("start");
            recState = RECORD_STATE_READY;
        }
        else
        {
            Model.PlayMusic("Connect");
            Model.RecorderCommand(Model.REC_CONTINUE);
            SetBtnRecImgSrc("pause");
            recState = RECORD_STATE_RUN;
        }
        break;
    default:
        break;
    }
}

void Calibrate::SetBtnRecImgSrc(const char* srcName)
{
    lv_obj_set_style_bg_img_src(View.ui.btnCont.btnRec, ResourcePool::GetImage(srcName), 0);
}

void Calibrate::onEvent(lv_event_t* event)
{
    Calibrate* instance = (Calibrate*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    // if (obj == instance->_root)
    // {
    //     if (code == LV_EVENT_PRESSED)
    //     {
    //         instance->onBtnClicked(obj);
    //     }
    // }

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        instance->onBtnClicked(obj);  // 仅该场景执行Pop
    }

    // if (obj == instance->View.ui.btnCont.btnRec)
    // {
    //     if (code == LV_EVENT_SHORT_CLICKED)
    //     {
    //         instance->onRecord(false);
    //     }
    //     else if (code == LV_EVENT_LONG_PRESSED)
    //     {
    //         instance->onRecord(true);
    //     }
    // }

   
}
