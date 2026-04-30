// SerialMonitor.cpp
// 功能：串口监测类的实现，包含串口操作和协议处理
// 新协议：包头0xAA，XOR = type ^ data1
#include "SerialMonitor.h"
#include <fcntl.h>       // 文件控制：open, O_RDWR等
#include <unistd.h>      // UNIX标准函数：read, write, close
#include <termios.h>     // 终端I/O：串口配置
#include <cstring>       // 字符串处理：strerror
#include <iostream>      // 输入输出：cout, cerr
#include <chrono>        // 时间库：sleep_for
#include <sys/select.h>  // I/O多路复用：select
#include <errno.h>       // 错误号：errno

// ========== 构造函数和析构函数 ==========

SerialMonitor::SerialMonitor() 
    : fd_(-1)                    // 初始化为-1，表示串口未打开
    , baudrate_(115200)          // 默认波特率115200
    , running_(false)            // 初始未运行
    , callback_(nullptr) {       // 回调函数为空
}

SerialMonitor::~SerialMonitor() {
    stop();  // 析构时自动停止监测
}

// ========== 串口操作函数 ==========

bool SerialMonitor::openSerialPort() {
    // 步骤1：打开串口设备
    // O_RDWR: 读写模式
    // O_NOCTTY: 不让该设备成为控制终端
    // O_NDELAY: 非阻塞模式
    fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ < 0) {
        last_error_ = "无法打开串口: " + std::string(strerror(errno));
        return false;
    }
    
    // 步骤2：获取当前串口配置
    struct termios options;
    if (tcgetattr(fd_, &options) < 0) {
        last_error_ = "获取串口属性失败: " + std::string(strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    
    // 步骤3：设置波特率（115200）
    cfsetispeed(&options, B115200);  // 输入波特率
    cfsetospeed(&options, B115200);  // 输出波特率
    
    // 步骤4：配置8N1格式（8数据位，无校验，1停止位）
    options.c_cflag &= ~PARENB;   // 清除校验位使能 -> 无校验
    options.c_cflag &= ~CSTOPB;   // 清除2停止位标志 -> 1停止位
    options.c_cflag &= ~CSIZE;    // 清除数据位设置
    options.c_cflag |= CS8;       // 设置为8数据位
    options.c_cflag |= (CLOCAL | CREAD);  // CLOCAL:忽略调制解调器控制线, CREAD:启用接收器
    
    // 步骤5：配置原始模式（不经过任何处理）
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  // 禁用规范模式、回显、信号
    options.c_iflag &= ~(IXON | IXOFF | IXANY);          // 禁用软件流控
    options.c_iflag &= ~(INLCR | ICRNL | IGNCR);         // 禁用换行符转换
    options.c_oflag &= ~OPOST;                            // 禁用输出处理
    
    // 步骤6：设置读取行为 - 阻塞模式，等待8字节
    options.c_cc[VMIN] = 8;      // 最少读取8字节才返回
    options.c_cc[VTIME] = 10;    // 10*100ms = 1秒超时
    
    // 步骤7：清空输入输出缓冲区
    tcflush(fd_, TCIOFLUSH);
    
    // 步骤8：应用新配置
    if (tcsetattr(fd_, TCSANOW, &options) < 0) {
        last_error_ = "设置串口属性失败: " + std::string(strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    
    return true;
}

void SerialMonitor::closeSerialPort() {
    if (fd_ >= 0) {
        ::close(fd_);  // 关闭串口文件描述符
        fd_ = -1;
    }
}

// ========== 监测线程函数 ==========

void SerialMonitor::monitoringThread() {
    uint8_t buffer[256];  // 临时接收缓冲区
    
    while (running_) {
        // 步骤1：使用select检查是否有数据可读（非阻塞方式）
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd_, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms超时
        
        int result = select(fd_ + 1, &read_fds, NULL, NULL, &timeout);
        
        if (result > 0 && FD_ISSET(fd_, &read_fds)) {
            // 步骤2：有数据可读，读取数据
            ssize_t bytes = read(fd_, buffer, sizeof(buffer));
            
            if (bytes > 0) {
                // 步骤3：将新数据添加到接收缓冲区
                receive_buffer_.insert(receive_buffer_.end(), buffer, buffer + bytes);
                
                // 步骤4：循环解析缓冲区中的完整数据包
                while (receive_buffer_.size() >= 8) {  // 至少8字节才可能是一个完整包
                    
                    // 4.1 查找包头0xAA
                    if (receive_buffer_[0] == UARTProtocol::PACKET_HEADER) {
                        // 找到包头，尝试解析8字节数据包
                        UARTProtocol::Packet pkt;
                        if (UARTProtocol::parse(receive_buffer_.data(), 8, pkt)) {
                            // 解析成功，调用回调函数
                            if (callback_) {
                                callback_(pkt);
                            }
                        }
                        // 移除已处理的8字节
                        receive_buffer_.erase(receive_buffer_.begin(), receive_buffer_.begin() + 8);
                    } else {
                        // 不是包头，丢弃这个字节
                        receive_buffer_.erase(receive_buffer_.begin());
                    }
                }
            }
        }
        
        // 步骤5：短暂休眠，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 线程结束时关闭串口
    closeSerialPort();
}

// ========== 公共接口实现 ==========

bool SerialMonitor::start(const std::string& device, int baudrate) {
    // 防止重复启动
    if (running_) {
        last_error_ = "监测已在运行中";
        return false;
    }
    
    // 保存参数
    device_ = device;
    baudrate_ = baudrate;
    receive_buffer_.clear();  // 清空接收缓冲区
    
    // 打开串口
    if (!openSerialPort()) {
        return false;
    }
    
    // 启动监测线程
    running_ = true;
    monitor_thread_ = std::thread(&SerialMonitor::monitoringThread, this);
    
    return true;
}

void SerialMonitor::stop() {
    if (!running_) return;  // 如果没运行，直接返回
    
    running_ = false;  // 设置停止标志
    
    // 等待线程结束
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

// ========== App → Device 发送命令实现 ==========

bool SerialMonitor::sendPower(bool on) {
    // 发送电源状态：TYPE_POWER + APP_POWER_ON/OFF
    auto data = UARTProtocol::buildPacket(
        UARTProtocol::TYPE_POWER,
        on ? UARTProtocol::APP_POWER_ON : UARTProtocol::APP_POWER_OFF);
    return write(fd_, data.data(), data.size()) == 8;
}

bool SerialMonitor::sendMeeting(uint8_t code) {
    // 发送会议状态：TYPE_MEETING + AppMeetingCode
    auto data = UARTProtocol::buildPacket(UARTProtocol::TYPE_MEETING, code);
    return write(fd_, data.data(), data.size()) == 8;
}

bool SerialMonitor::sendHotspot(uint8_t code) {
    // 发送热点状态：TYPE_HOTSPOT + AppHotspotCode
    auto data = UARTProtocol::buildPacket(UARTProtocol::TYPE_HOTSPOT, code);
    return write(fd_, data.data(), data.size()) == 8;
}

bool SerialMonitor::sendGetWorkMode() {
    // 发送获取工作模式请求：TYPE_MODE + APP_MODE_GET
    auto data = UARTProtocol::buildPacket(UARTProtocol::TYPE_MODE, UARTProtocol::APP_MODE_GET);
    return write(fd_, data.data(), data.size()) == 8;
}

void SerialMonitor::setCallback(SimpleDataCallback callback) {
    callback_ = callback;  // 设置回调函数
}