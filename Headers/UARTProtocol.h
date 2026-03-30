// UARTProtocol.h
// 功能：定义串口通信协议的数据结构和基本操作
#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <vector>
#include <cstdint>
#include <string>

class UARTProtocol {
public:
    // ========== 协议常量定义 ==========
    static constexpr uint8_t PACKET_HEADER = 0xF3;  // 固定包头
    static constexpr size_t PACKET_SIZE = 8;        // 固定包大小：8字节
    
    // ========== 数据类型定义（对应协议中的bit3）==========
    enum DataType : uint8_t {
        DATA_SEND = 0x01,     // 数据发送
        DATA_REQUEST = 0x02    // 数据请求（预留）
    };
    
    // ========== 功能码定义（对应协议中的data1）==========
    enum FunctionCode : uint8_t {
        FUNC_POWER = 0xA0,     // 电源状态
        FUNC_HOTSPOT = 0xA1,   // 热点状态
        FUNC_MEETING = 0xA2,   // 会议状态
        FUNC_VOLUME = 0xA3     // 音量控制
    };
    
    // ========== 状态值定义（对应协议中的data2）==========
    // 电源状态
    enum PowerState : uint8_t {
        POWER_ON = 0x00,       // 电源开启
        POWER_OFF = 0x01       // 电源关闭
    };
    
    // 热点状态
    enum HotspotState : uint8_t {
        HOTSPOT_ON = 0x00,     // 热点开启
        HOTSPOT_OFF = 0x01     // 热点关闭
    };
    
    // 会议状态
    enum MeetingState : uint8_t {
        MEETING_START = 0x00,  // 会议开始
        MEETING_PAUSE = 0x01,  // 会议暂停
        MEETING_END = 0x02     // 会议结束
    };
    
    // 音量控制动作
    enum VolumeAction : uint8_t {
        VOLUME_UP = 0x00,       // 音量增加
        VOLUME_DOWN = 0x01,     // 音量减少
        VOLUME_LONG_UP = 0x02,  // 音量长加
        VOLUME_LONG_DOWN = 0x03,// 音量长减
        VOLUME_LONG_END = 0x04  // 长按结束
    };
    
    // ========== 数据包结构（对应协议格式）==========
    struct Packet {
        uint8_t header;      // [0] 包头，固定0xF3
        uint8_t length;      // [1] 数据长度，范围1-4
        uint8_t xorCheck;    // [2] XOR校验值
        uint8_t dataType;    // [3] 数据类型：0x01发送/0x02请求
        uint8_t data1;       // [4] 功能码：0xA0电源/0xA1热点/0xA2会议/0xA3音量
        uint8_t data2;       // [5] 状态/动作值
        uint8_t data3;       // [6] 预留
        uint8_t data4;       // [7] 预留
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
     * @brief 构建发送数据包
     * @param dataType 数据类型（0x01发送/0x02请求）
     * @param func 功能码
     * @param state 状态值
     * @return 构建好的8字节数据包
     */
    static std::vector<uint8_t> buildPacket(uint8_t dataType, uint8_t func, uint8_t state);
    
    /**
     * @brief 计算XOR校验值
     * @param pkt 数据包
     * @return XOR校验结果
     */
    static uint8_t calculateXOR(const Packet& pkt);
};

#endif // UART_PROTOCOL_H