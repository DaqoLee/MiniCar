#include "Device.h"
#include "Version.h"
#include "Utils/PageManager/PM_Log.h"
using namespace Page;

Device::Device():lastFocus(nullptr)
{
}

Device::~Device()
{

}

void Device::onCustomAttrConfig()
{

}

void Device::onViewLoad()
{
    Model.Init();
    View.Create(_root);
    AttachEvent(_root);
    lastFocus = nullptr;

    DeviceView::item_t* item_grp = ((DeviceView::item_t*)&View.ui);

    for (int i = 0; i < sizeof(View.ui) / sizeof(DeviceView::item_t); i++)
    {
        AttachEvent(item_grp[i].icon);
    }
}

void Device::onViewDidLoad()
{

}

void Device::onViewWillAppear()
{
 
    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_BLACK);
    Model.SetRemoteMode(DataProc::OperationMode_t::MODE_OTA);
    timer = lv_timer_create(onTimerUpdate, 100, this);
    lv_timer_ready(timer);

    View.SetScrollToY(_root, -LV_VER_RES, LV_ANIM_OFF);
    lv_obj_set_style_opa(_root, LV_OPA_TRANSP, 0);
    lv_obj_fade_in(_root, 300, 0);
}

void Device::onViewDidAppear()
{
    lv_group_t* group = lv_group_get_default();
    lv_group_set_wrap(group, true);
    LV_ASSERT_NULL(group);

    // 确保Device页面的对象重新添加到焦点组
    DeviceView::item_t* item_grp = ((DeviceView::item_t*)&View.ui);
    for (int i = 0; i < sizeof(View.ui) / sizeof(DeviceView::item_t); i++)
    {
        lv_group_add_obj(group, item_grp[i].icon);
    }
    if (lastFocus)
    {
        lv_group_focus_obj(lastFocus);
    }
    else
    {
        // 设置焦点到第一个项目
        lv_group_focus_obj(item_grp[0].icon);
    }
    
    // 确保焦点组的包装模式正确
    
    View.onFocus(group);
}

void Device::onViewWillDisappear()
{
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    lastFocus = lv_group_get_focused(group);
    lv_obj_fade_out(_root, 300, 0);
}

void Device::onViewDidDisappear()
{
    lv_timer_del(timer);
}

void Device::onViewUnload()
{
    View.Delete();
    Model.Deinit();
}

void Device::onViewDidUnload()
{

}

void Device::AttachEvent(lv_obj_t* obj)
{
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void Device::Update()
{
    // char buf[64];

    /* Sport */
    // float trip;
    // float maxSpd;
    // Model.GetSportInfo(&trip, buf, sizeof(buf), &maxSpd);
    // // View.SetSport(trip, buf, maxSpd);

    // /* GPS */
    // float lat;
    // float lng;
    // float alt;
    // float course;
    // float speed;
    // Model.GetGPSInfo(&lat, &lng, &alt, buf, sizeof(buf), &course, &speed);
    // // View.SetGPS(lat, lng, alt, buf, course, speed);

    // /* MAG */
    // float dir;
    // int x;
    // int y;
    // int z;
    // Model.GetMAGInfo(&dir, &x, &y, &z);
    // // View.SetMAG(dir, x, y, z);

    // /* IMU */
    // int steps;
    // Model.GetIMUInfo(&steps, buf, sizeof(buf));
    // View.SetIMU(steps, buf);

    // /* Power */
    // int usage;
    // float voltage;
    // Model.GetBatteryInfo(&usage, &voltage, buf, sizeof(buf));
    // View.SetBattery(usage, voltage, buf);


    // /* System */
    // // DataProc::MakeTimeString(lv_tick_get(), buf, sizeof(buf));
    // View.SetSystem(
    //     VERSION_FIRMWARE_NAME " " VERSION_SOFTWARE,
    //     VERSION_AUTHOR_NAME,
    //     VERSION_LVGL,
    //     buf,
    //     VERSION_COMPILER,
    //     VERSION_BUILD_TIME
    // );
}

void Device::onTimerUpdate(lv_timer_t* timer)
{
    Device* instance = (Device*)timer->user_data;

    instance->Update();
}

void Device::onBtnClicked(lv_obj_t* btn)
{
    // if (btn == View.ui.pair.icon)
    // {
    //     //_Manager->Push("Pages/Pair");
    // }
    // else if (btn == View.ui.home.icon)
    // {
    //     // _Manager->Push("Pages/Home");
    //     _Manager->Pop();
    // }
    // else if (btn == View.ui.calibrate.icon)
    // {
    //    // _Manager->Push("Pages/Calibrate");
    // }
    // else if (btn == View.ui.device.icon)
    // {
    //    // _Manager->Push("Pages/Device");
    // }    

     _Manager->Pop();
}
void Device::onEvent(lv_event_t* event)
{
    Device* instance = (Device*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSED)
    {
        instance->onBtnClicked(obj);
    }

    // if (code == LV_EVENT_PRESSED)
    // {
    //     if (lv_obj_has_state(obj, LV_STATE_FOCUSED))
    //     {
    //         instance->_Manager->Pop();
    //     }
    // }

    // if (obj == instance->_root)
    // {
    //     if (code == LV_EVENT_LEAVE)
    //     {
    //         instance->_Manager->Pop();
    //     }
    // }
}
