#include "Device.h"
#include "Version.h"
#include "Utils/PageManager/PM_Log.h"
#include <Arduino.h>
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
}

void Device::onViewDidLoad()
{
}

void Device::RebuildList()
{
    // Get device list
    DataProc::DeviceList_Info_t devInfo;
    DATA_PROC_INIT_STRUCT(devInfo);
    Model.GetDeviceList(&devInfo);
    
    // Calculate total items: devices + pair + back
    int devCount = devInfo.count;
    if (devCount > 10) devCount = 10;
    int totalItems = devCount + 2; // devices + ADD + BACK
    
    // Rebuild view
    View.RebuildItems(totalItems);
    
    // Set labels
    for (int i = 0; i < totalItems && i < MAX_DEVICE_ITEMS; i++) {
        if (i < devCount) {
            // Device entry (truncate to 13 chars)
            char nameBuf[20];
            strncpy(nameBuf, devInfo.devices[i].name, 13);
            nameBuf[13] = '\0';
            if (strlen(devInfo.devices[i].name) > 13) strcat(nameBuf, "..");
            lv_label_set_text(View.ui.items[i].labelInfo, nameBuf);
            char dataBuf[32];
            snprintf(dataBuf, sizeof(dataBuf), "%02X%02X",
                     devInfo.devices[i].mac[4], devInfo.devices[i].mac[5]);
            if (i == devInfo.currentIndex) {
                strcat(dataBuf, "  \xe2\x97\x86"); // ◆ marker for current
            }
            lv_label_set_text(View.ui.items[i].labelData, dataBuf);
        } else if (i == devCount) {
            // Pair entry
            lv_label_set_text(View.ui.items[i].labelInfo, "Add New Device");
            lv_label_set_text(View.ui.items[i].labelData, "");
        } else {
            // Back entry
            lv_label_set_text(View.ui.items[i].labelInfo, "Back");
            lv_label_set_text(View.ui.items[i].labelData, "");
        }
        
        // Attach event to icon
        AttachEvent(View.ui.items[i].icon);
    }
    
    Serial.printf("[Device] Rebuild: %d devices\n", devCount);
}

void Device::onViewWillAppear()
{
    lastFocus = nullptr; // Reset stale pointer from previous visit
    RebuildList();
    View.GroupReset();
    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_BLACK);
    
    timer = lv_timer_create(onTimerUpdate, 500, this);
    lv_timer_ready(timer);

    View.SetScrollToY(_root, -LV_VER_RES, LV_ANIM_OFF);
    lv_obj_set_style_opa(_root, LV_OPA_TRANSP, 0);
    lv_obj_fade_in(_root, 300, 0);
}

void Device::onViewDidAppear()
{
    lv_group_t* group = lv_group_get_default();
    if (!group) return;
    lv_group_set_wrap(group, true);

    for (int i = 0; i < View.ui.itemCount; i++) {
        lv_group_add_obj(group, View.ui.items[i].icon);
    }
    if (lastFocus) {
        lv_group_focus_obj(lastFocus);
    } else if (View.ui.itemCount > 0) {
        lv_group_focus_obj(View.ui.items[0].icon);
    }
    View.onFocus(group);
}

void Device::onViewWillDisappear()
{
    lv_group_t* group = lv_group_get_default();
    if (group) {
        lastFocus = lv_group_get_focused(group);
        lv_group_remove_all_objs(group);
    }
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
    // Check if device count changed, rebuild if so
    DataProc::DeviceList_Info_t devInfo;
    DATA_PROC_INIT_STRUCT(devInfo);
    Model.GetDeviceList(&devInfo);
    
    int expected = (devInfo.count > 10 ? 10 : devInfo.count) + 2;
    if (expected != View.ui.itemCount) {
        // Detach old events by rebuilding
        RebuildList();
        // Re-add to group
        lv_group_t* group = lv_group_get_default();
        if (group) {
            for (int i = View.ui.itemCount - 1; i >= 0; i--) {
                lv_group_add_obj(group, View.ui.items[i].icon);
            }
        }
    }
}

void Device::onTimerUpdate(lv_timer_t* timer)
{
    Device* instance = (Device*)timer->user_data;
    instance->Update();
}

void Device::onBtnClicked(lv_obj_t* btn)
{
    // Root click -> back
    if (btn == _root) {
        _Manager->Pop();
        return;
    }
    
    int devCount = View.ui.itemCount - 2;
    if (devCount < 0) devCount = 0;
    
    for (int i = 0; i < View.ui.itemCount; i++) {
        if (btn == View.ui.items[i].icon) {
            if (i < devCount) {
                // Device entry: switch + go back
                Serial.printf("[Device] Switch to device %d\n", i);
                Model.SwitchDevice((uint8_t)i);
                _Manager->Pop(); // go back to SystemInfos
                return;
            } else if (i == devCount) {
                // Pair button: go to pair page
                Model.EnterPairingMode();
                _Manager->Push("Pages/Pair");
                return;
            } else {
                // Back button: go back
                _Manager->Pop();
                return;
            }
        }
    }
}

void Device::onEvent(lv_event_t* event)
{
    Device* instance = (Device*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_SHORT_CLICKED) {
        instance->onBtnClicked(obj);
    }
}
