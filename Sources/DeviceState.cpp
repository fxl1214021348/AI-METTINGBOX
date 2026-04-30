// DeviceState.cpp
// 功能：集中管理设备状态（热点、工作模式、会议、开关机）
#include "DeviceState.h"
#include "HotspotUtils.h"
#include <atomic>
#include <cstdio>

// ========== 内部状态变量（原子操作保证线程安全）==========
static std::atomic<HotspotStatus> g_hotspot_status(HOTSPOT_STATUS_OFF);
static std::atomic<WorkMode>      g_work_mode(WORK_MODE_PC);
static std::atomic<MeetingStatus> g_meeting_status(MEETING_STATUS_IDLE);
static std::atomic<PowerStatus>   g_power_status(POWER_STATUS_ON);

// ========== 初始化 ==========

void device_state_init(void) {
    // 从系统查询热点初始状态
    gboolean active = is_hotspot_active();
    g_hotspot_status.store(active ? HOTSPOT_STATUS_ON : HOTSPOT_STATUS_OFF);

    // 工作模式默认PC模式，等待device下发
    g_work_mode.store(WORK_MODE_PC);

    // 会议默认空闲
    g_meeting_status.store(MEETING_STATUS_IDLE);

    // 开关机默认开机（APP已启动说明已开机）
    g_power_status.store(POWER_STATUS_ON);

    printf("[DeviceState] 初始化完成 - 热点: %s, 工作模式: %s, 会议: %s, 电源: %s\n",
           device_state_hotspot_to_string(g_hotspot_status.load()),
           device_state_work_mode_to_string(g_work_mode.load()),
           device_state_meeting_to_string(g_meeting_status.load()),
           device_state_power_to_string(g_power_status.load()));
}

// ========== 热点状态操作 ==========

void device_state_set_hotspot(HotspotStatus status) {
    HotspotStatus old = g_hotspot_status.exchange(status);
    if (old != status) {
        printf("[DeviceState] 热点状态变更: %s -> %s\n",
               device_state_hotspot_to_string(old),
               device_state_hotspot_to_string(status));
    }
}

HotspotStatus device_state_get_hotspot(void) {
    return g_hotspot_status.load();
}

const char* device_state_hotspot_to_string(HotspotStatus status) {
    switch (status) {
        case HOTSPOT_STATUS_ON:            return "已开启";
        case HOTSPOT_STATUS_OFF:           return "已关闭";
        case HOTSPOT_STATUS_TURNING_ON:    return "开启中";
        case HOTSPOT_STATUS_TURNING_OFF:   return "关闭中";
        case HOTSPOT_STATUS_TURN_ON_FAIL:  return "开启失败";
        case HOTSPOT_STATUS_TURN_OFF_FAIL: return "关闭失败";
        default:                           return "未知";
    }
}

// ========== 工作模式操作 ==========

void device_state_set_work_mode(WorkMode mode) {
    WorkMode old = g_work_mode.exchange(mode);
    if (old != mode) {
        printf("[DeviceState] 工作模式变更: %s -> %s\n",
               device_state_work_mode_to_string(old),
               device_state_work_mode_to_string(mode));
    }
}

WorkMode device_state_get_work_mode(void) {
    return g_work_mode.load();
}

const char* device_state_work_mode_to_string(WorkMode mode) {
    switch (mode) {
        case WORK_MODE_PC:         return "PC模式";
        case WORK_MODE_STANDALONE: return "独立模式";
        default:                   return "未知";
    }
}

// ========== 会议状态操作 ==========

void device_state_set_meeting(MeetingStatus status) {
    MeetingStatus old = g_meeting_status.exchange(status);
    if (old != status) {
        printf("[DeviceState] 会议状态变更: %s -> %s\n",
               device_state_meeting_to_string(old),
               device_state_meeting_to_string(status));
    }
}

MeetingStatus device_state_get_meeting(void) {
    return g_meeting_status.load();
}

const char* device_state_meeting_to_string(MeetingStatus status) {
    switch (status) {
        case MEETING_STATUS_IDLE:        return "空闲";
        case MEETING_STATUS_ONGOING:     return "进行中";
        case MEETING_STATUS_PAUSED:      return "暂停";
        case MEETING_STATUS_ENDED:       return "结束";
        case MEETING_STATUS_SUMMARIZING: return "会议总结中";
        case MEETING_STATUS_START_FAIL:  return "开启失败";
        case MEETING_STATUS_PAUSE_FAIL:  return "暂停失败";
        case MEETING_STATUS_END_FAIL:    return "结束失败";
        default:                         return "未知";
    }
}

// ========== 开关机状态操作 ==========

void device_state_set_power(PowerStatus status) {
    PowerStatus old = g_power_status.exchange(status);
    if (old != status) {
        printf("[DeviceState] 电源状态变更: %s -> %s\n",
               device_state_power_to_string(old),
               device_state_power_to_string(status));
    }
}

PowerStatus device_state_get_power(void) {
    return g_power_status.load();
}

const char* device_state_power_to_string(PowerStatus status) {
    switch (status) {
        case POWER_STATUS_ON:  return "开机";
        case POWER_STATUS_OFF: return "关机";
        default:               return "未知";
    }
}
