// UARTProtocol.cpp
// 功能：实现协议解析和封装的函数
// 新协议：包头0xAA，XOR = type ^ data1
#include "UARTProtocol.h"
#include <cstring>  // for memcpy

/**
 * @brief 解析原始数据为协议包
 * 
 * 解析步骤：
 * 1. 检查数据长度是否足够（至少8字节）
 * 2. 检查包头是否为0xAA
 * 3. 验证数据长度字段是否为0x02
 * 4. 验证XOR校验是否正确（type ^ data1）
 */
bool UARTProtocol::parse(const uint8_t* data, size_t len, Packet& pkt) {
    // 检查数据长度和包头
    if (len < PACKET_SIZE || data[0] != PACKET_HEADER) {
        return false;  // 数据不够或包头错误
    }
    
    // 复制数据到包结构
    memcpy(&pkt, data, PACKET_SIZE);
    
    // 验证长度字段
    if (pkt.length != DATA_LENGTH) {
        return false;  // 长度字段不是0x02
    }
    
    // 计算并验证XOR校验值（新协议只校验 type ^ data1）
    uint8_t xorCheck = pkt.type ^ pkt.data1;
    
    return (xorCheck == pkt.xorCheck);
}

/**
 * @brief 构建发送数据包（App → Device）
 * 
 * 构建步骤：
 * 1. 创建Packet结构并填充数据
 * 2. 设置固定值（包头0xAA，长度0x02，保留字节0x00）
 * 3. 计算XOR校验值（type ^ data1）
 * 4. 转换为字节数组返回
 */
std::vector<uint8_t> UARTProtocol::buildPacket(uint8_t type, uint8_t data1) {
    Packet pkt;
    
    // 填充固定字段
    pkt.header = PACKET_HEADER;      // 包头：0xAA
    pkt.length = DATA_LENGTH;        // 数据长度：0x02
    
    // 填充可变字段
    pkt.type = type;                 // 数据类型（模块ID）
    pkt.data1 = data1;              // 指令/状态码
    pkt.data2 = 0x00;               // 保留
    pkt.data3 = 0x00;               // 保留
    pkt.data4 = 0x00;               // 保留
    
    // 计算XOR校验值
    pkt.xorCheck = type ^ data1;
    
    // 转换为字节数组
    std::vector<uint8_t> result(PACKET_SIZE);
    memcpy(result.data(), &pkt, PACKET_SIZE);
    
    return result;
}

/**
 * @brief 计算XOR校验值
 */
uint8_t UARTProtocol::calculateXOR(uint8_t type, uint8_t data1) {
    return type ^ data1;
}