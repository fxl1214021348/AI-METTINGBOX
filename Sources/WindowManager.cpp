#include "WindowManager.h"
#include "WebView.h"
#include "SerialMonitor.h"
#include "WebViewJSBridge.h"  // 新增
#include "ExitSettingButton.h"
#include "HotspotUtils.h"
#include "DeviceStatusDialog.h"
#include <json-glib/json-glib.h>
#include <iostream> 

// 全局串口监测器
extern SerialMonitor* g_serialMonitor;
// 声明外部全局变量（在WindowManager.cpp中定义的）
extern WebKitWebView *g_webView;

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
                    if(hotspot_enable()){
                        device_status_update_hotspot_ui(TRUE);
                    }
                    return G_SOURCE_REMOVE;
                },nullptr);
            } else {
                // 收到关闭执行，调度到主线程执行
                g_idle_add([](gpointer) -> gboolean {
                    if(hotspot_disable()){
                        device_status_update_hotspot_ui(FALSE);
                    }
                    return G_SOURCE_REMOVE;
                },nullptr);
            }
            break;
            
        case 0xA2:  // 会议状态
            {
                const char* states[] = {"开始", "暂停", "结束"};
                printf("[串口解析] 会议状态: %s\n", pkt.data2 <= 2 ? states[pkt.data2] : "未知");
                if(pkt.data2 == 0){
                    // 发送会议开始消息到Web端
                    if (g_webView) {
                        webview_bridge_send(g_webView, "{\"type\":\"CREATE_MEETING\",\"value\":\"true\"}");
                        g_print("会议开始\n");
                    }
                }
                if(pkt.data2 == 1){
                    // 发送会议暂停消息到Web端
                    if (g_webView) {
                        webview_bridge_send(g_webView, "{\"type\":\"PAUSE_MEETING\",\"value\":\"true\"}");
                        g_print("会议暂停\n");
                    }
                }
                if(pkt.data2 == 2){
                    // 发送会议结束消息到Web端
                    if (g_webView) {
                        webview_bridge_send(g_webView, "{\"type\":\"END_MEETING\",\"value\":\"true\"}");
                        g_print("会议结束\n");
                    }
                }
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

// ========== 新增：消息处理回调 ==========
static void on_js_message(const char *message, void *user_data) {
    g_print("[收到JS消息] %s\n", message);
    
    // // 这里可以根据消息内容做不同处理
    // if (strstr(message, "wifi") != NULL) {
    //     g_print("这是WiFi相关的消息\n");
    // } else if (strstr(message, "device") != NULL) {
    //     g_print("这是设备相关的消息\n");
    // }

    // 解析JSON
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser, message, -1, NULL)) {
        g_print("[JS消息] 非JSON格式，忽略: %s\n", message);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonObject *obj = json_node_get_object(root);

    const gchar *type = json_object_get_string_member(obj, "type");
    const gchar *action = json_object_get_string_member(obj, "action");

    if (!type || !action) {
        g_print("[JS消息] 缺少 type 或 action 字段\n");
        g_object_unref(parser);
        return;
    }

    // 根据 type 分发处理
    if (strcmp(type, "hotspot") == 0) {
        // 热点控制
        if (strcmp(action, "on") == 0) {
            if (hotspot_enable()) {
                device_status_update_hotspot_ui(TRUE);
                // 回复前端
                webview_bridge_send(g_webView,
                    "{\"type\":\"hotspot\",\"state\":\"on\",\"result\":\"success\"}");
            } else {
                webview_bridge_send(g_webView,
                    "{\"type\":\"hotspot\",\"state\":\"off\",\"result\":\"fail\"}");
            }
        } else if (strcmp(action, "off") == 0) {
            if (hotspot_disable()) {
                device_status_update_hotspot_ui(FALSE);
                webview_bridge_send(g_webView,
                    "{\"type\":\"hotspot\",\"state\":\"off\",\"result\":\"success\"}");
            } else {
                webview_bridge_send(g_webView,
                    "{\"type\":\"hotspot\",\"state\":\"on\",\"result\":\"fail\"}");
            }
        }

    } else if (strcmp(type, "volume") == 0) {
        // 音量控制 → 通过串口发给硬件
        if (g_serialMonitor) {
            if (strcmp(action, "up") == 0) {
                g_serialMonitor->sendVolume(UARTProtocol::VOLUME_UP);
            } else if (strcmp(action, "down") == 0) {
                g_serialMonitor->sendVolume(UARTProtocol::VOLUME_DOWN);
            }
        }

    } else if (strcmp(type, "meeting") == 0) {
        // 会议控制 → 通过串口发给硬件
        if (g_serialMonitor) {
            if (strcmp(action, "start") == 0) {
                g_print("接收到web端发送的会议开始消息\n");
                g_serialMonitor->sendMeeting(UARTProtocol::MEETING_START);
            } else if (strcmp(action, "pause") == 0) {
                g_print("接收到web端发送的会议暂停消息\n");
                g_serialMonitor->sendMeeting(UARTProtocol::MEETING_PAUSE);
            } else if (strcmp(action, "end") == 0) {
                g_print("接收到web端发送的会议结束消息\n");
                g_serialMonitor->sendMeeting(UARTProtocol::MEETING_END);
            }
        }

    } else if (strcmp(type, "power") == 0) {
        // 电源控制 → 通过串口发给硬件
        if (g_serialMonitor) {
            g_serialMonitor->sendPower(strcmp(action, "on") == 0);
        }

    } else {
        g_print("[JS消息] 未知类型: %s\n", type);
    }

    g_object_unref(parser);
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
    const char* serial_device = "/dev/meetingbox_serial";  // udev固定软连接
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
    webview_load_url(webView, "http://133.156.1.145:9529/");
    gtk_container_add(GTK_CONTAINER(overlay), GTK_WIDGET(webView));
    
    create_two_buttons(GTK_OVERLAY(overlay));
 
    return window;
}
