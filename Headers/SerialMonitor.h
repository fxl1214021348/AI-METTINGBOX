// SerialMonitor.h
// 功能：串口监测类，负责串口的打开、读取、发送和数据处理
#ifndef SERIAL_MONITOR_H
#define SERIAL_MONITOR_H

#include <string>
#include <thread>       // 多线程支持
#include <atomic>       // 原子操作
#include <functional>   // 函数对象
#include <vector>       // 动态数组
#include <cstdint>      // 标准整数类型
#include "UARTProtocol.h"

class SerialMonitor {
public:
    // 回调函数类型定义：收到完整协议包时调用
    // 参数：解析好的协议包
    typedef std::function<void(const UARTProtocol::Packet& packet)> SimpleDataCallback;
    
    // 构造函数：初始化成员变量
    SerialMonitor();
    
    // 析构函数：确保串口被正确关闭
    ~SerialMonitor();
    
    // ========== 核心功能接口 ==========
    
    /**
     * @brief 启动串口监测
     * @param device 串口设备路径，如："/dev/ttyS1"
     * @param baudrate 波特率，默认115200
     * @return true:启动成功 false:启动失败
     */
    bool start(const std::string& device, int baudrate = 115200);
    
    /**
     * @brief 停止串口监测
     */
    void stop();
    
    // ========== 发送命令接口 ==========
    
    /**
     * @brief 发送电源控制命令
     * @param on true:开启 false:关闭
     */
    bool sendPower(bool on);
    
    /**
     * @brief 发送音量控制命令
     * @param action 0x00增加 0x01减少 0x02长加 0x03长减 0x04结束
     */
    bool sendVolume(uint8_t action);
    
    /**
     * @brief 发送会议控制命令
     * @param state 0x00开始 0x01暂停 0x02结束
     */
    bool sendMeeting(uint8_t state);
    
    /**
     * @brief 发送热点控制命令
     * @param on true:开启 false:关闭
     */
    bool sendHotspot(bool on);
    
    // ========== 辅助功能接口 ==========
    
    /**
     * @brief 设置数据接收回调函数
     * @param callback 回调函数
     */
    void setCallback(SimpleDataCallback callback);
    
    /**
     * @brief 获取最后错误信息
     */
    std::string getLastError() const { return last_error_; }
    
    /**
     * @brief 检查是否运行中
     */
    bool isRunning() const { return running_; }

private:
    // ========== 内部私有函数 ==========
    
    /**
     * @brief 监测线程主函数
     * 循环读取串口数据并进行处理
     */
    void monitoringThread();
    
    /**
     * @brief 打开串口设备
     * @return true:成功 false:失败
     */
    bool openSerialPort();
    
    /**
     * @brief 关闭串口设备
     */
    void closeSerialPort();
    
    // ========== 成员变量 ==========
    
    int fd_;                          // 串口文件描述符，-1表示未打开
    std::string device_;               // 串口设备路径
    int baudrate_;                     // 波特率
    std::atomic<bool> running_;        // 运行标志，线程安全
    std::thread monitor_thread_;       // 监测线程对象
    std::string last_error_;           // 最后一次错误信息
    SimpleDataCallback callback_;      // 数据回调函数
    std::vector<uint8_t> receive_buffer_;  // 接收缓冲区，用于处理粘包
};

#endif // SERIAL_MONITOR_H