#include "Pair.h"
using namespace Page;

Pair::Pair()
    : recState(RECORD_STATE_READY)
    , lastFocus(nullptr)
{
}

Pair::~Pair()
{
}

void Pair::onCustomAttrConfig()
{
    SetCustomLoadAnimType(PageManager::LOAD_ANIM_NONE);
}

void Pair::onViewLoad()
{
    Model.Init();
    View.Create(_root);

    AttachEvent(_root);
    AttachEvent(View.ui.bottomInfo.cont);
    // AttachEvent(View.ui.btnCont.btnRec);
    // AttachEvent(View.ui.btnCont.btnMenu);
}

void Pair::onViewDidLoad()
{

}

void Pair::onViewWillAppear()
{
    lv_indev_wait_release(lv_indev_get_act());
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);

    lv_group_set_wrap(group, true);

    lv_group_add_obj(group, View.ui.bottomInfo.cont);
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

void Pair::onViewDidAppear()
{
    timer = lv_timer_create(onTimerUpdate, 100, this);
}

void Pair::onViewWillDisappear()
{
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    lastFocus = lv_group_get_focused(group);
    // lv_group_remove_obj(View.ui.bottomInfo.cont);
    lv_group_remove_all_objs(group);
    lv_timer_del(timer);
    // View.AppearAnimStart(true);
}

void Pair::onViewDidDisappear()
{
}

void Pair::onViewUnload()
{
    View.Delete();
    Model.Deinit();
   
}

void Pair::onViewDidUnload()
{

}

void Pair::AttachEvent(lv_obj_t* obj)
{
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void Pair::Update()
{

    char name[32] = {0};
    Model.GetDeviceInfo(name);
   // for (int i = 0; i < 4; i++)
   if (name[0])
    {
        lv_label_set_text(
        View.ui.topInfo.labelInfoGrp[0].lableValue,name);
    }
}

void Pair::onTimerUpdate(lv_timer_t* timer)
{
    Pair* instance = (Pair*)timer->user_data;

    instance->Update();
}

void Pair::onBtnClicked(lv_obj_t* btn)
{
    // if (btn == View.ui.btnCont.btnMap)
    // {
    //     _Manager->Push("Pages/LiveMap");
    // }
    // else if (btn == View.ui.btnCont.btnMenu || btn == _root)
    {
        _Manager->Pop();
    }
}

void Pair::onRecord(bool longPress)
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

void Pair::SetBtnRecImgSrc(const char* srcName)
{
    lv_obj_set_style_bg_img_src(View.ui.btnCont.btnRec, ResourcePool::GetImage(srcName), 0);
}

void Pair::onEvent(lv_event_t* event)
{
    Pair* instance = (Pair*)lv_event_get_user_data(event);
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

    if (obj == instance->View.ui.bottomInfo.cont && code == LV_EVENT_SHORT_CLICKED)
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
