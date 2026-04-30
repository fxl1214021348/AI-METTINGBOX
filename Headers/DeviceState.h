// DeviceState.h
// 功能：集中管理设备状态（热点、工作模式、会议、开关机）
// APP端负责存储状态，device端（1126开发板）负责下发指令/状态
#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <cstdint>

// ========== 热点状态枚举（APP端）==========
enum HotspotStatus : uint8_t {
    HOTSPOT_STATUS_ON           = 0,  // 已开启
    HOTSPOT_STATUS_OFF          = 1,  // 已关闭
    HOTSPOT_STATUS_TURNING_ON   = 2,  // 开启中
    HOTSPOT_STATUS_TURNING_OFF  = 3,  // 关闭中
    HOTSPOT_STATUS_TURN_ON_FAIL = 4,  // 开启失败
    HOTSPOT_STATUS_TURN_OFF_FAIL= 5   // 关闭失败
};

// ========== 工作模式枚举（device下发，APP存储）==========
enum WorkMode : uint8_t {
    WORK_MODE_PC         = 0,  // PC模式
    WORK_MODE_STANDALONE = 1   // 独立模式
};

// ========== 会议状态枚举（APP端）==========
enum MeetingStatus : uint8_t {
    MEETING_STATUS_IDLE          = 0,  // 空闲
    MEETING_STATUS_ONGOING       = 1,  // 进行中
    MEETING_STATUS_PAUSED        = 2,  // 暂停
    MEETING_STATUS_ENDED         = 3,  // 结束
    MEETING_STATUS_SUMMARIZING   = 4,  // 会议总结中
    MEETING_STATUS_START_FAIL    = 5,  // 开启失败
    MEETING_STATUS_PAUSE_FAIL    = 6,  // 暂停失败
    MEETING_STATUS_END_FAIL      = 7,  // 结束失败
    MEETING_STATUS_RESUME_FAIL   = 8   // 继续失败
};

// ========== 开关机状态枚举（APP端 & device端）==========
enum PowerStatus : uint8_t {
    POWER_STATUS_ON  = 0,  // 开机
    POWER_STATUS_OFF = 1   // 关机
};

// ========== 初始化 ==========

/**
 * @brief 初始化设备状态
 *
 * 从系统查询热点初始状态，其余状态设为默认值：
 * - 工作模式：PC模式（等待device下发）
 * - 会议状态：空闲
 * - 开关机状态：开机
 * 应在 window_create_kiosk() 中调用一次。
 */
void device_state_init(void);

// ========== 热点状态操作 ==========

/**
 * @brief 设置热点状态
 * @param status 新的热点状态
 */
void device_state_set_hotspot(HotspotStatus status);

/**
 * @brief 获取当前热点状态
 * @return 当前热点状态
 */
HotspotStatus device_state_get_hotspot(void);

/**
 * @brief 获取热点状态的中文描述
 * @param status 热点状态
 * @return 中文字符串（静态存储，无需释放）
 */
const char* device_state_hotspot_to_string(HotspotStatus status);

// ========== 工作模式操作 ==========

/**
 * @brief 设置工作模式（由device下发）
 * @param mode 新的工作模式
 */
void device_state_set_work_mode(WorkMode mode);

/**
 * @brief 获取当前工作模式
 * @return 当前工作模式
 */
WorkMode device_state_get_work_mode(void);

/**
 * @brief 获取工作模式的中文描述
 * @param mode 工作模式
 * @return 中文字符串（静态存储，无需释放）
 */
const char* device_state_work_mode_to_string(WorkMode mode);

// ========== 会议状态操作 ==========

/**
 * @brief 设置会议状态
 * @param status 新的会议状态
 */
void device_state_set_meeting(MeetingStatus status);

/**
 * @brief 获取当前会议状态
 * @return 当前会议状态
 */
MeetingStatus device_state_get_meeting(void);

/**
 * @brief 获取会议状态的中文描述
 * @param status 会议状态
 * @return 中文字符串（静态存储，无需释放）
 */
const char* device_state_meeting_to_string(MeetingStatus status);

// ========== 开关机状态操作 ==========

/**
 * @brief 设置开关机状态
 * @param status 新的开关机状态
 */
void device_state_set_power(PowerStatus status);

/**
 * @brief 获取当前开关机状态
 * @return 当前开关机状态
 */
PowerStatus device_state_get_power(void);

/**
 * @brief 获取开关机状态的中文描述
 * @param status 开关机状态
 * @return 中文字符串（静态存储，无需释放）
 */
const char* device_state_power_to_string(PowerStatus status);

#endif // DEVICE_STATE_H
