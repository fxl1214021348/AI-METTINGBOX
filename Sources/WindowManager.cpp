#include "WindowManager.h"
#include "WebView.h"
#include "SerialMonitor.h"
#include "WebViewJSBridge.h"  // 新增
#include "ExitSettingButton.h"
#include "HotspotUtils.h"
#include <iostream> 

// 全局串口监测器
extern SerialMonitor* g_serialMonitor;

/**
 * @brief 串口数据接收回调函数
 * @param pkt 解析好的协议包
 * 
 * 当串口收到完整且有效的协议包时，此函数被调用
 * 这里我们简单打印出收到的数据
 */
static void on_serial_data(const UARTProtocol::Packet& pkt) {
    // 打印原始数据（8字节十六进制）
    printf("[串口接收] 原始数据: ");
    printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
           pkt.header,      // 包头：应该是F3
           pkt.length,      // 数据长度
           pkt.xorCheck,    // XOR校验值
           pkt.dataType,    // 数据类型
           pkt.data1,       // 数据1：功能码
           pkt.data2,       // 数据2：状态值
           pkt.data3,       // 数据3：预留
           pkt.data4);      // 数据4：预留
    
    // 根据功能码解析具体含义
    switch(pkt.data1) {
        case 0xA0:  // 电源状态
            printf("[串口解析] 电源状态: %s\n", 
                   pkt.data2 == 0 ? "开启" : "关闭");
            break;
            
        case 0xA1:  // 热点状态
            printf("[串口解析] 热点状态: %s\n", 
                   pkt.data2 == 0 ? "开启" : "关闭");
            if(pkt.data2 == 0){
                // 收到开启指令，调度到主线程执行
                g_idle_add([](gpointer) -> gboolean {
                    hotspot_enable();
                    return G_SOURCE_REMOVE;
                },nullptr);
            } else {
                // 收到关闭执行，调度到主线程执行
                g_idle_add([](gpointer) -> gboolean {
                    hotspot_disable();
                    return G_SOURCE_REMOVE;
                },nullptr);
            }
            break;
            
        case 0xA2:  // 会议状态
            {
                const char* states[] = {"开始", "暂停", "结束"};
                printf("[串口解析] 会议状态: %s\n", 
                       pkt.data2 <= 2 ? states[pkt.data2] : "未知");
            }
            break;
            
        case 0xA3:  // 音量控制
            {
                const char* actions[] = {"增加", "减少", "长加", "长减", "结束"};
                printf("[串口解析] 音量动作: %s\n", 
                       pkt.data2 <= 4 ? actions[pkt.data2] : "未知");
            }
            break;
            
        default:
            printf("[串口解析] 未知功能码: 0x%02X\n", pkt.data1);
    }
}

// 声明外部全局变量（在WindowManager.cpp中定义的）
extern WebKitWebView *g_webView;

// ========== 新增：消息处理回调 ==========
static void on_js_message(const char *message, void *user_data) {
    g_print("[收到JS消息] %s\n", message);
    
    // 这里可以根据消息内容做不同处理
    if (strstr(message, "wifi") != NULL) {
        g_print("这是WiFi相关的消息\n");
    } else if (strstr(message, "device") != NULL) {
        g_print("这是设备相关的消息\n");
    }
}

gboolean window_on_close_request(GtkWidget *widget, GdkEvent *event, gpointer data) {
    g_print("阻止窗口关闭！提示：按Ctrl+Alt+F1进入TTY1终端\n");
    return TRUE;  // TRUE=阻止，FALSE=允许关闭
}

void window_on_destroy(GtkWidget *widget, gpointer data) {

    // 清理串口资源
    if (g_serialMonitor) {
        g_serialMonitor->stop();    // 停止监测线程
        delete g_serialMonitor;      // 删除对象
        g_serialMonitor = nullptr;
        printf("串口资源已清理\n");
    }

    gtk_main_quit();
}

GtkWidget* window_create_kiosk(void) {
    // 创建主窗口：全屏、无边框、无标题栏
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_fullscreen(GTK_WINDOW(window));
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    
    // 连接窗口生命周期信号
    g_signal_connect(window, "delete-event", 
                     G_CALLBACK(window_on_close_request), NULL);
    g_signal_connect(window, "destroy", 
                     G_CALLBACK(window_on_destroy), NULL);
    
    // 使用Overlay布局：WebView在下层，按钮在上层
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), overlay);
    
    // 创建并添加WebView（底层）
    WebKitWebView *webView = webview_create();

    // 保存到全局变量（记住遥控器在哪）
    g_webView = webView;  // ← 关键：把webView地址保存到全局变量

    // ========== 新增：初始化通信 ==========
    webview_bridge_init(webView);
    webview_bridge_register(webView, "fromJS", on_js_message, NULL);

    // ========== 串口初始化部分 ==========
    // 1. 创建串口监测对象
    g_serialMonitor = new SerialMonitor();
    // 2. 设置数据接收回调
    g_serialMonitor->setCallback(on_serial_data);
    // 3. 尝试打开串口（根据实际硬件修改设备路径）
    const char* serial_device = "/dev/ttyUSB0";  // RV1126常用串口
    printf("正在尝试打开串口: %s ...\n", serial_device);
    if (g_serialMonitor->start(serial_device, 115200)) {
        printf("✓ 串口打开成功！\n");
        
        // 4. 可选：发送测试命令
        // printf("发送测试命令...\n");
        // g_serialMonitor->sendPower(true);    // 电源开启
        // g_serialMonitor->sendVolume(0x00);   // 音量增加
    } else {
        printf("✗ 串口打开失败: %s\n", g_serialMonitor->getLastError().c_str());
        printf("  可能原因：\n");
        printf("  1. 串口设备不存在\n");
        printf("  2. 权限不足（尝试: sudo chmod 666 %s）\n", serial_device);
        printf("  3. 设备已被占用\n");
    }
    // ========== 串口初始化结束 ==========


    // 连接WebView的新窗口信号
    g_signal_connect(webView, "create", G_CALLBACK(webview_on_create_new_window), NULL);

    //webview_load_url(webView, "https://www.baidu.com");
    //webview_load_url(webView, "file:///home/q/%E4%BC%9A%E8%AE%AE%E7%9B%92%E5%AD%90%E6%96%B0%E7%94%9F%E7%89%88/AI-MTB/test.html");
    webview_load_url(webView, "http://133.156.1.107:8000/");
    gtk_container_add(GTK_CONTAINER(overlay), GTK_WIDGET(webView));
    
    create_two_buttons(GTK_OVERLAY(overlay));
 
    return window;
}
