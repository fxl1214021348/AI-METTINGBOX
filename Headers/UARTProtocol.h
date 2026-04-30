// UARTProtocol.h
// 功能：定义串口通信协议的数据结构和基本操作
// 协议格式：8字节定长帧，包头0xAA
#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <vector>
#include <cstdint>
#include <string>

class UARTProtocol {
public:
    // ========== 协议常量定义 ==========
    static constexpr uint8_t PACKET_HEADER = 0xAA;  // 固定包头
    static constexpr uint8_t DATA_LENGTH = 0x02;     // 固定数据长度（类型+数据1）
    static constexpr size_t PACKET_SIZE = 8;        // 固定包大小：8字节
    
    // ========== 数据类型定义（模块ID，对应byte3）==========
    enum DataType : uint8_t {
        TYPE_POWER   = 0x01,   // 开关机状态
        TYPE_MEETING = 0x02,   // 会议状态
        TYPE_HOTSPOT = 0x03,   // 热点状态
        TYPE_MODE    = 0x04    // 工作模式
    };
    
    // ========== Device → App 指令码（data1，0x01起）==========

    // 开关机（TYPE_POWER）
    enum DevPowerCode : uint8_t {
        DEV_POWER_ON         = 0x01,   // 开机
        DEV_POWER_OFF        = 0x02    // 关机
    };

    // 会议（TYPE_MEETING）
    enum DevMeetingCode : uint8_t {
        DEV_MEET_QUERY       = 0x01,   // 获取会议状态
        DEV_MEET_START       = 0x02,   // 开始会议
        DEV_MEET_PAUSE       = 0x03,   // 暂停会议
        DEV_MEET_STOP        = 0x04,   // 结束会议
        DEV_MEET_RESUME      = 0x05    // 继续会议
    };

    // 热点（TYPE_HOTSPOT）
    enum DevHotspotCode : uint8_t {
        DEV_HOT_QUERY        = 0x01,   // 获取热点状态
        DEV_HOT_ON           = 0x02,   // 开启热点
        DEV_HOT_OFF          = 0x03    // 关闭热点
    };

    // 工作模式（TYPE_MODE）
    enum DevModeCode : uint8_t {
        DEV_MODE_PC          = 0x01,   // PC模式
        DEV_MODE_STANDALONE  = 0x02    // 独立模式
    };
    
    // ========== App → Device 状态码（data1，0x10起）==========

    // 开关机（TYPE_POWER）
    enum AppPowerCode : uint8_t {
        APP_POWER_ON         = 0x10,   // 开机
        APP_POWER_OFF        = 0x11    // 关机
    };

    // 会议（TYPE_MEETING）
    enum AppMeetingCode : uint8_t {
        APP_MEET_IDLE        = 0x10,   // 空闲
        APP_MEET_RUNNING     = 0x11,   // 进行中
        APP_MEET_PAUSED      = 0x12,   // 暂停
        APP_MEET_ENDED       = 0x13,   // 结束
        APP_MEET_SUMMARY     = 0x14,   // 会议总结中
        APP_MEET_START_ERR   = 0x15,   // 开启失败
        APP_MEET_PAUSE_ERR   = 0x16,   // 暂停失败
        APP_MEET_STOP_ERR    = 0x17,   // 结束失败
        APP_MEET_RESUME_ERR  = 0x18,   // 继续失败
        APP_MEET_RESUME      = 0x19    // 继续 
    };

    // 热点（TYPE_HOTSPOT）
    enum AppHotspotCode : uint8_t {
        APP_HOT_ON           = 0x10,   // 热点已开启
        APP_HOT_OFF          = 0x11,   // 热点已关闭
        APP_HOT_ONING        = 0x12,   // 热点开启中
        APP_HOT_OFFING       = 0x13,   // 热点关闭中
        APP_HOT_ON_ERR       = 0x14,   // 热点开启失败
        APP_HOT_OFF_ERR      = 0x15    // 热点关闭失败
    };

    // 工作模式（TYPE_MODE）
    enum AppModeCode : uint8_t {
        APP_MODE_GET         = 0x10    // 获取工作模式
    };
    
    // ========== 数据包结构（8字节帧）==========
    struct Packet {
        uint8_t header;      // [0] 包头，固定0xAA
        uint8_t length;      // [1] 数据长度，固定0x02
        uint8_t xorCheck;    // [2] XOR校验 = type ^ data1
        uint8_t type;        // [3] 数据类型（模块ID）
        uint8_t data1;       // [4] 指令/状态码
        uint8_t data2;       // [5] 保留 0x00
        uint8_t data3;       // [6] 保留 0x00
        uint8_t data4;       // [7] 保留 0x00
    };
    
    // ========== 核心功能函数 ==========
    
    /**
     * @brief 解析接收到的原始数据
     * @param data 原始数据指针
     * @param len 数据长度
     * @param pkt 解析后的数据包（输出参数）
     * @return true:解析成功 false:解析失败
     */
    static bool parse(const uint8_t* data, size_t len, Packet& pkt);
    
    /**
     * @brief 构建发送数据包（App → Device）
     * @param type 数据类型（模块ID）
     * @param data1 状态码
     * @return 构建好的8字节数据包
     */
    static std::vector<uint8_t> buildPacket(uint8_t type, uint8_t data1);
    
    /**
     * @brief 计算XOR校验值
     * @param type 数据类型
     * @param data1 数据1
     * @return XOR校验结果 = type ^ data1
     */
    static uint8_t calculateXOR(uint8_t type, uint8_t data1);
};

#endif // UART_PROTOCOL_H