// UARTProtocol.cpp
// 功能：实现协议解析和封装的函数
#include "UARTProtocol.h"
#include <cstring>  // for memcpy

/**
 * @brief 解析原始数据为协议包
 * 
 * 解析步骤：
 * 1. 检查数据长度是否足够（至少8字节）
 * 2. 检查包头是否为0xF3
 * 3. 验证数据长度字段是否合法（1-4）
 * 4. 验证XOR校验是否正确
 */
bool UARTProtocol::parse(const uint8_t* data, size_t len, Packet& pkt) {
    // 检查数据长度和包头
    if (len < PACKET_SIZE || data[0] != PACKET_HEADER) {
        return false;  // 数据不够或包头错误
    }
    
    // 复制数据到包结构
    memcpy(&pkt, data, PACKET_SIZE);
    
    // 验证长度字段是否在有效范围
    if (pkt.length < 1 || pkt.length > 4) {
        return false;  // 长度字段非法
    }
    
    // 计算XOR校验值
    uint8_t xorCheck = 0;
    xorCheck ^= pkt.dataType;  // 异或数据类型
    xorCheck ^= pkt.data1;      // 异或数据1
    xorCheck ^= pkt.data2;      // 异或数据2
    xorCheck ^= pkt.data3;      // 异或数据3
    xorCheck ^= pkt.data4;      // 异或数据4
    
    // 验证XOR校验
    return (xorCheck == pkt.xorCheck);
}

/**
 * @brief 构建发送数据包
 * 
 * 构建步骤：
 * 1. 创建Packet结构并填充数据
 * 2. 设置默认值（包头0xF3，长度2，预留字节0）
 * 3. 计算XOR校验值
 * 4. 转换为字节数组返回
 */
std::vector<uint8_t> UARTProtocol::buildPacket(uint8_t dataType, uint8_t func, uint8_t state) {
    Packet pkt;
    
    // 填充固定字段
    pkt.header = PACKET_HEADER;     // 包头：0xF3
    pkt.length = 2;                  // 数据长度：2（使用data1和data2）
    
    // 填充可变字段
    pkt.dataType = dataType;         // 数据类型：0x01发送
    pkt.data1 = func;                // 功能码：0xA0/A1/A2/A3
    pkt.data2 = state;               // 状态值
    pkt.data3 = 0;                    // 预留，填0
    pkt.data4 = 0;                    // 预留，填0
    
    // 计算XOR校验值
    pkt.xorCheck = 0;
    pkt.xorCheck ^= pkt.dataType;    // 异或数据类型
    pkt.xorCheck ^= pkt.data1;        // 异或功能码
    pkt.xorCheck ^= pkt.data2;        // 异或状态值
    pkt.xorCheck ^= pkt.data3;        // 异或预留
    pkt.xorCheck ^= pkt.data4;        // 异或预留
    
    // 转换为字节数组
    std::vector<uint8_t> result(PACKET_SIZE);
    memcpy(result.data(), &pkt, PACKET_SIZE);
    
    return result;
}

/**
 * @brief 计算XOR校验值（独立函数，供外部调用）
 */
uint8_t UARTProtocol::calculateXOR(const Packet& pkt) {
    uint8_t result = 0;
    result ^= pkt.dataType;
    result ^= pkt.data1;
    result ^= pkt.data2;
    result ^= pkt.data3;
    result ^= pkt.data4;
    return result;
}