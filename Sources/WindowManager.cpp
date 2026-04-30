#include "WindowManager.h"
#include "WebView.h"
#include "SerialMonitor.h"
#include "WebViewJSBridge.h"  // 新增
#include "ExitSettingButton.h"
#include "HotspotUtils.h"
#include "DeviceStatusDialog.h"
#include "DeviceState.h"
#include <json-glib/json-glib.h>
#include <iostream> 

// 全局串口监测器
SerialMonitor* g_serialMonitor;
// 声明外部全局变量（在WindowManager.cpp中定义的）
WebKitWebView *g_webView;

/**
 * @brief 串口数据接收回调函数
 * @param pkt 解析好的协议包
 * 
 * 当串口收到完整且有效的协议包时，此函数被调用
 * 新协议：type=模块ID，data1=指令码（Device→App: 0x01起）
 */
static void on_serial_data(const UARTProtocol::Packet& pkt) {
    // 打印原始数据（8字节十六进制）
    printf("[串口接收] 原始数据: ");
    printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
           pkt.header, pkt.length, pkt.xorCheck, pkt.type,
           pkt.data1, pkt.data2, pkt.data3, pkt.data4);
    
    // 根据模块ID分发处理
    switch(pkt.type) {
        case UARTProtocol::TYPE_POWER:  // 0x01 开关机
            printf("[串口解析] 开关机指令: 0x%02X\n", pkt.data1);
            if (pkt.data1 == UARTProtocol::DEV_POWER_ON) {
                printf("[串口解析] 开机\n");
                device_state_set_power(POWER_STATUS_ON);
                // 回复device当前电源状态
                if (g_serialMonitor) {
                    g_serialMonitor->sendPower(true);
                }
            } else if (pkt.data1 == UARTProtocol::DEV_POWER_OFF) {
                printf("[串口解析] 关机\n");
                device_state_set_power(POWER_STATUS_OFF);
                if (g_serialMonitor) {
                    g_serialMonitor->sendPower(false);
                }
            }
            break;

        case UARTProtocol::TYPE_MEETING:  // 0x02 会议
            printf("[串口解析] 会议指令: 0x%02X\n", pkt.data1);
            switch (pkt.data1) {
                case UARTProtocol::DEV_MEET_QUERY:
                    // device查询会议状态，回复当前状态
                    printf("[串口解析] 查询会议状态\n");
                    if (g_serialMonitor) {
                        MeetingStatus ms = device_state_get_meeting();
                        uint8_t code;
                        switch (ms) {
                            case MEETING_STATUS_IDLE:        code = UARTProtocol::APP_MEET_IDLE; break;
                            case MEETING_STATUS_ONGOING:     code = UARTProtocol::APP_MEET_RUNNING; break;
                            case MEETING_STATUS_PAUSED:      code = UARTProtocol::APP_MEET_PAUSED; break;
                            case MEETING_STATUS_ENDED:       code = UARTProtocol::APP_MEET_ENDED; break;
                            case MEETING_STATUS_SUMMARIZING: code = UARTProtocol::APP_MEET_SUMMARY; break;
                            case MEETING_STATUS_START_FAIL:  code = UARTProtocol::APP_MEET_START_ERR; break;
                            case MEETING_STATUS_PAUSE_FAIL:  code = UARTProtocol::APP_MEET_PAUSE_ERR; break;
                            case MEETING_STATUS_END_FAIL:    code = UARTProtocol::APP_MEET_STOP_ERR; break;
                            case MEETING_STATUS_RESUME_FAIL: code = UARTProtocol::APP_MEET_RESUME_ERR; break;
                            default:                         code = UARTProtocol::APP_MEET_IDLE; break;
                        }
                        g_serialMonitor->sendMeeting(code);
                    }
                    break;

                case UARTProtocol::DEV_MEET_START:
                    printf("[串口解析] 开始会议\n");
                    device_state_set_meeting(MEETING_STATUS_ONGOING);
                    if (g_webView) {
                        g_idle_add([](gpointer) -> gboolean {
                            webview_bridge_send(g_webView, "{\"type\":\"CREATE_MEETING\",\"value\":\"true\"}");
                            return G_SOURCE_REMOVE;
                        }, nullptr);
                    }
                    break;

                case UARTProtocol::DEV_MEET_PAUSE:
                    printf("[串口解析] 暂停会议\n");
                    device_state_set_meeting(MEETING_STATUS_PAUSED);
                    if (g_webView) {
                        g_idle_add([](gpointer) -> gboolean {
                            webview_bridge_send(g_webView, "{\"type\":\"PAUSE_MEETING\",\"value\":\"true\"}");
                            return G_SOURCE_REMOVE;
                        }, nullptr);
                    }
                    break;
                case UARTProtocol::DEV_MEET_RESUME:
                    printf("[串口解析] 继续会议\n");
                    device_state_set_meeting(MEETING_STATUS_ONGOING);
                    if (g_webView) {
                        g_idle_add([](gpointer) -> gboolean {
                            webview_bridge_send(g_webView, "{\"type\":\"RESUME_MEETING\",\"value\":\"true\"}");
                            return G_SOURCE_REMOVE;
                        }, nullptr);
                    }
                    break;
                case UARTProtocol::DEV_MEET_STOP:
                    printf("[串口解析] 结束会议\n");
                    device_state_set_meeting(MEETING_STATUS_IDLE);
                    if (g_webView) {
                        g_idle_add([](gpointer) -> gboolean {
                            webview_bridge_send(g_webView, "{\"type\":\"END_MEETING\",\"value\":\"true\"}");
                            return G_SOURCE_REMOVE;
                        }, nullptr);
                    }
                    break;

                default:
                    printf("[串口解析] 未知会议指令: 0x%02X\n", pkt.data1);
            }
            break;
            
        case UARTProtocol::TYPE_HOTSPOT:  // 0x03 热点
            printf("[串口解析] 热点指令: 0x%02X\n", pkt.data1);
            switch (pkt.data1) {
                case UARTProtocol::DEV_HOT_QUERY:
                    // device查询热点状态，回复当前状态
                    printf("[串口解析] 查询热点状态\n");
                    if (g_serialMonitor) {
                        HotspotStatus hs = device_state_get_hotspot();
                        uint8_t code;
                        switch (hs) {
                            case HOTSPOT_STATUS_ON:            code = UARTProtocol::APP_HOT_ON; break;
                            case HOTSPOT_STATUS_OFF:           code = UARTProtocol::APP_HOT_OFF; break;
                            case HOTSPOT_STATUS_TURNING_ON:    code = UARTProtocol::APP_HOT_ONING; break;
                            case HOTSPOT_STATUS_TURNING_OFF:   code = UARTProtocol::APP_HOT_OFFING; break;
                            case HOTSPOT_STATUS_TURN_ON_FAIL:  code = UARTProtocol::APP_HOT_ON_ERR; break;
                            case HOTSPOT_STATUS_TURN_OFF_FAIL: code = UARTProtocol::APP_HOT_OFF_ERR; break;
                            default:                           code = UARTProtocol::APP_HOT_OFF; break;
                        }
                        g_serialMonitor->sendHotspot(code);
                    }
                    break;

                case UARTProtocol::DEV_HOT_ON:
                    // device指令开启热点（异步执行）
                    printf("[串口解析] 开启热点\n");
                device_state_set_hotspot(HOTSPOT_STATUS_TURNING_ON);
                // 回复device"开启中"
               if (g_serialMonitor) {
                   g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_ONING);
               }
                g_idle_add([](gpointer) -> gboolean {
                    // 更新UI为"开启中"
                    device_status_update_hotspot_ui_loading(TRUE);
                    // 子线程执行耗时的nmcli操作
                    g_thread_new("serial-hotspot-on", [](gpointer) -> gpointer {
                        gboolean ok = hotspot_enable();
                        g_idle_add([](gpointer data) -> gboolean {
                            gboolean success = GPOINTER_TO_INT(data);
                            if (success) {
                                device_state_set_hotspot(HOTSPOT_STATUS_ON);
                                device_status_update_hotspot_ui(TRUE);
                                if (g_serialMonitor) {
                                   g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_ON);
                                }
                            } else {
                                device_state_set_hotspot(HOTSPOT_STATUS_TURN_ON_FAIL);
                                device_status_update_hotspot_ui(FALSE);
                                if (g_serialMonitor) {
                                   g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_ON_ERR);
                                }
                            }
                            return G_SOURCE_REMOVE;
                        }, GINT_TO_POINTER(ok));
                        return nullptr;
                    }, nullptr);
                    return G_SOURCE_REMOVE;
                }, nullptr);
            	 break;
            	 
            case UARTProtocol::DEV_HOT_OFF:
                // device指令关闭热点（异步执行）
                printf("[串口解析] 关闭热点\n");
                device_state_set_hotspot(HOTSPOT_STATUS_TURNING_OFF);
                if (g_serialMonitor) {
                    g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_OFFING);
                }
                g_idle_add([](gpointer) -> gboolean {
                    device_status_update_hotspot_ui_loading(FALSE);
                    g_thread_new("serial-hotspot-off", [](gpointer) -> gpointer {
                        gboolean ok = hotspot_disable();
                        g_idle_add([](gpointer data) -> gboolean {
                            gboolean success = GPOINTER_TO_INT(data);
                            if (success) {
                                device_state_set_hotspot(HOTSPOT_STATUS_OFF);
                                device_status_update_hotspot_ui(FALSE);
                                if (g_serialMonitor) {
                                    g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_OFF);
                                }
                            } else {
                                device_state_set_hotspot(HOTSPOT_STATUS_TURN_OFF_FAIL);
                                device_status_update_hotspot_ui(TRUE);
                                if (g_serialMonitor) {
                                    g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_OFF_ERR);
                                }
                            }
                            return G_SOURCE_REMOVE;
                        }, GINT_TO_POINTER(ok));
                        return nullptr;
                    }, nullptr);
                    return G_SOURCE_REMOVE;
                }, nullptr);
            	 break;
            	 
            default:
                printf("[串口解析] 未知热点指令: 0x%02X\n", pkt.data1);
            }
            break;
            
        case UARTProtocol::TYPE_MODE:  // 0x04 工作模式
            printf("[串口解析] 工作模式: 0x%02X\n", pkt.data1);
            if (pkt.data1 == UARTProtocol::DEV_MODE_PC) {
                printf("[串口解析] PC模式\n");
                device_state_set_work_mode(WORK_MODE_PC);
            } else if (pkt.data1 == UARTProtocol::DEV_MODE_STANDALONE) {
                printf("[串口解析] 独立模式\n");
                device_state_set_work_mode(WORK_MODE_STANDALONE);
            }
            break;
            
        default:
            printf("[串口解析] 未知模块ID: 0x%02X\n", pkt.type);
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
        g_print("[JS消息] 音量控制: %s（新协议不支持串口热点）\n", action);
       // 热点控制（异步执行，不阻塞主线程）
        // if (strcmp(action, "on") == 0) {
        //     device_state_set_hotspot(HOTSPOT_STATUS_TURNING_ON);
        //     device_status_update_hotspot_ui_loading(TRUE);
        //     // 回复device"开启中"
        //     if (g_serialMonitor) {
        //         g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_ONING);
        //     }
        //     g_thread_new("js-hotspot-on", [](gpointer) -> gpointer {
        //         gboolean ok = hotspot_enable();
        //         g_idle_add([](gpointer data) -> gboolean {
        //             gboolean success = GPOINTER_TO_INT(data);
        //             if (success) {
        //                 device_state_set_hotspot(HOTSPOT_STATUS_ON);
        //                 device_status_update_hotspot_ui(TRUE);
        //                 webview_bridge_send(g_webView,
        //                     "{\"type\":\"hotspot\",\"state\":\"on\",\"result\":\"success\"}");
        //                 if (g_serialMonitor) {
        //                     g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_ON);
        //                 }
        //             } else {
        //                 device_state_set_hotspot(HOTSPOT_STATUS_TURN_ON_FAIL);
        //                 device_status_update_hotspot_ui(FALSE);
        //                 webview_bridge_send(g_webView,
        //                     "{\"type\":\"hotspot\",\"state\":\"off\",\"result\":\"fail\"}");
        //                 if (g_serialMonitor) {
        //                     g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_ON_ERR);
        //                 }
        //             }
        //             return G_SOURCE_REMOVE;
        //         }, GINT_TO_POINTER(ok));
        //         return nullptr;
        //     }, nullptr);
        // } else if (strcmp(action, "off") == 0) {
        //     device_state_set_hotspot(HOTSPOT_STATUS_TURNING_OFF);
        //     device_status_update_hotspot_ui_loading(FALSE);
        //     if (g_serialMonitor) {
        //         g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_OFFING);
        //     }
        //     g_thread_new("js-hotspot-off", [](gpointer) -> gpointer {
        //         gboolean ok = hotspot_disable();
        //         g_idle_add([](gpointer data) -> gboolean {
        //             gboolean success = GPOINTER_TO_INT(data);
        //             if (success) {
        //                 device_state_set_hotspot(HOTSPOT_STATUS_OFF);
        //                 device_status_update_hotspot_ui(FALSE);
        //                 webview_bridge_send(g_webView,
        //                     "{\"type\":\"hotspot\",\"state\":\"off\",\"result\":\"success\"}");
        //                 if (g_serialMonitor) {
        //                     g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_OFF);
        //                 }
        //             } else {
        //                 device_state_set_hotspot(HOTSPOT_STATUS_TURN_OFF_FAIL);
        //                 device_status_update_hotspot_ui(TRUE);
        //                 webview_bridge_send(g_webView,
        //                     "{\"type\":\"hotspot\",\"state\":\"on\",\"result\":\"fail\"}");
        //                 if (g_serialMonitor) {
        //                     g_serialMonitor->sendHotspot(UARTProtocol::APP_HOT_OFF_ERR);
        //                 }
        //             }
        //             return G_SOURCE_REMOVE;
        //         }, GINT_TO_POINTER(ok));
        //         return nullptr;
        //     }, nullptr);
        // }

    } else if (strcmp(type, "volume") == 0) {
        // 音量控制（新协议中已移除串口音量，仅打印日志）
        g_print("[JS消息] 音量控制: %s（新协议不支持串口音量）\n", action);

    } else if (strcmp(type, "meeting") == 0) {
        // 会议控制 → 更新状态 + 通过串口回复device
        if (strcmp(action, "start") == 0) {
            g_print("接收到web端发送的会议开始消息\n");
            device_state_set_meeting(MEETING_STATUS_ONGOING);
            if (g_serialMonitor) {
                g_serialMonitor->sendMeeting(UARTProtocol::APP_MEET_RUNNING);
            }
        } else if (strcmp(action, "pause") == 0) {
            g_print("接收到web端发送的会议暂停消息\n");
            device_state_set_meeting(MEETING_STATUS_PAUSED);
            if (g_serialMonitor) {
                g_serialMonitor->sendMeeting(UARTProtocol::APP_MEET_PAUSED);
            }
        } else if (strcmp(action, "end") == 0) {
            g_print("接收到web端发送的会议结束消息\n");
            device_state_set_meeting(MEETING_STATUS_IDLE);
            if (g_serialMonitor) {
                g_serialMonitor->sendMeeting(UARTProtocol::APP_MEET_IDLE);
            }
        } else if (strcmp(action, "resume") == 0) {
            g_print("接收到web端发送的会议继续消息\n");
            device_state_set_meeting(MEETING_STATUS_ONGOING);
            if (g_serialMonitor) {
                g_serialMonitor->sendMeeting(UARTProtocol::APP_MEET_RUNNING);
        	}
        } else if (strcmp(action, "start_err") == 0) {
            g_print("接收到web端发送的会议开始失败消息\n");
            device_state_set_meeting(MEETING_STATUS_START_FAIL);
            if (g_serialMonitor) {
                g_serialMonitor->sendMeeting(UARTProtocol::APP_MEET_START_ERR);
        	}
        } else if (strcmp(action, "pause_err") == 0) {
            g_print("接收到web端发送的会议暂停失败消息\n");
            device_state_set_meeting(MEETING_STATUS_PAUSE_FAIL);
            if (g_serialMonitor) {
                g_serialMonitor->sendMeeting(UARTProtocol::APP_MEET_PAUSE_ERR);
        	}
        } else if (strcmp(action, "resume_err") == 0) {
            g_print("接收到web端发送的会议继续失败消息\n");
            device_state_set_meeting(MEETING_STATUS_RESUME_FAIL);
            if (g_serialMonitor) {
                g_serialMonitor->sendMeeting(UARTProtocol::APP_MEET_RESUME_ERR);
        	}
        } else if (strcmp(action, "stop_err") == 0) {
            g_print("接收到web端发送的会议结束失败消息\n");
            device_state_set_meeting(MEETING_STATUS_END_FAIL);
            if (g_serialMonitor) {
                g_serialMonitor->sendMeeting(UARTProtocol::APP_MEET_STOP_ERR);
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

    // 初始化设备状态管理
    device_state_init();
    
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
    //const char* serial_device = "/dev/ttyUSB0";
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
    webview_load_url(webView, "http://localhost:9529/");
    gtk_container_add(GTK_CONTAINER(overlay), GTK_WIDGET(webView));
    
    create_two_buttons(GTK_OVERLAY(overlay));
 
    return window;
}
