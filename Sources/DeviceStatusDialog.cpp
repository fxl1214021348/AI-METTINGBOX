#include "DeviceStatusDialog.h"
#include "WifiControl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ========== 极简CSS ==========
static const char* DIALOG_CSS = R"(
/* 全屏背景 */
.fullscreen-bg {
    background-color: #f5f5f5;
}

/* 标题栏容器 */
.title-bar-container {
    background-color: transparent;
    border-bottom: 1px solid #f0f0f0;
    min-height: 80px;
    padding: 0;
    border: 1px solid red;
}

/* 标题 */
.dialog-title {
    font-size: 40px;
    font-weight: bold;
    color: #1A1A1A;
}

/* 关闭按钮 */
.close-btn {
    /* background-color: rgba(26, 26, 26, 0.08); */
    background-color: transparent;
    border-radius: 20px;
    min-width: 40px;
    min-height: 40px;
    border: none;
}

.close-btn:hover {
    background-color: rgba(26, 26, 26, 0.15);
}

/* 水平卡片容器 */
.horizontal-cards-container {
    background-color: transparent;
    spacing: 80px;
}

/* 信息卡片 */
.info-card {
    background-color: white;
    border: 1px solid #e8e8e8;
    border-radius: 12px;
    min-width: 720px;
    min-height: 440px;
    padding: 0;
}

/* 卡片标题栏 */
.card-title-bar {
    border-bottom: 1px solid #f0f0f0;
    margin: 60px 52px 32px 60px;
    padding: 0;
}

.card-title-text {
    font-size: 36px;
    font-weight: 600;
    color: #1a1a1a;
}

/* 设置链接 */
.settings-link {
    color: #267DFF;
    font-size: 30px;
    font-weight: 500;
    border: none;
    background: transparent;
}

.settings-link:hover {
    text-decoration: underline;
}

/*创建外部容器存放IP显示框和刷新按钮样版*/
.ip-external-container {
    min-width: 600px;
    min-height: 80px;
    margin: 24px 60px 60px 60px;
    border: 1px solid red;
}

/* IP显示框 */
.ip-display {
    min-width: 520px;
    min-height: 80px;
    background-color: #f5f5f5;
    border-radius: 16px;
    margin: 0px 40px 0px 0px;
}

.ip-value {
    font-family: monospace;
    font-size: 30px;
    color: #333333;
    font-weight: 500;
    margin-left: 32px;
}

/* 刷新按钮 */
.refresh-btn {
    background: transparent;
    border: none;
    min-width: 40px;
    min-height: 44px;
}

/* IP和设备热点 */
.hotspot-ipwifi {
    margin: 40px 52px 24px 60px;
}

/* 热点内容 */
.hotspot-content {
    margin: 24px 52px 24px 60px;
}

.hotspot-desc {
    font-size: 30px;
    color: rgba(26, 26, 26, 0.6); 
    margin-bottom: 10px;
    border: 1px solid red;
}

.hotspot-name {
    font-size: 20px;
    color: #1a1a1a;
    font-weight: 600;
}

/* 开关按钮 */
.toggle-switch {
    background: transparent;
    border: none;
    min-width: 80px;
    min-height: 40px;
}

/* WiFi图标按钮样式 */
.wifi-icon-btn {
    background-color: transparent;
    border: none;
    padding: 8px;
    border-radius: 8px;
    min-width: 40px;
    min-height: 40px;
}

/* 暂无数据 - 红色 */
.ip-value.no-data {
    font-size: 30px;
    color: #FA372D;  /* 红色 */
    font-weight: 400;
}

/* 获取中 - 蓝色或灰色 */
.ip-value.getting {
    color: #1A1A1A;  /* 黑色 */
    font-weight: 400;
    opacity: 0.35;
}
)";

// ========== 辅助函数声明 ==========
static gboolean is_hotspot_active(void);

// ========== 回调函数声明 ==========
static void on_close_clicked(GtkWidget *widget, gpointer data);
static void on_refresh_clicked(GtkWidget *widget, gpointer data);
static void on_toggle_hotspot(GtkWidget *widget, gpointer data);
static void on_go_settings(GtkWidget *widget, gpointer data);

// ========== 工具函数实现 ==========

// 检测系统热点是否实际开启
static gboolean is_hotspot_active(void) {
    FILE* fp = popen("nmcli -t -f NAME,DEVICE connection show --active 2>/dev/null | grep -i hotspot", "r");
    if (!fp) {
        printf("[DEBUG] 执行热点检测命令失败\n");
        return FALSE;
    }
    
    char buffer[256];
    gboolean result = (fgets(buffer, sizeof(buffer), fp) != NULL);
    pclose(fp);
    
    printf("[DEBUG] 热点状态检测: %s\n", result ? "已开启" : "已关闭");
    return result;
}

// 获取设备IP地址
gchar* get_device_ip(void) {
    printf("[DEBUG] 获取当前活动网络IP...\n");
    
    // 方法1：通过默认路由获取当前活动IP
    // 获取默认路由的接口
    FILE* fp = popen("ip route show default 0.0.0.0/0 2>/dev/null | awk '/default/ {print $5}' | head -1", "r");
    
    if (fp) {
        char interface[64];
        if (fgets(interface, sizeof(interface), fp)) {
            interface[strcspn(interface, "\n")] = 0;
            pclose(fp);
            
            if (strlen(interface) > 0) {
                printf("[DEBUG] 当前活动接口: %s\n", interface);
                
                // 获取该接口的IP
                gchar command[256];
                snprintf(command, sizeof(command), 
                    "ip -4 addr show %s 2>/dev/null | awk '/inet/ {print $2}' | cut -d/ -f1 | head -1", 
                    interface);
                
                fp = popen(command, "r");
                if (fp) {
                    char ip[64];
                    if (fgets(ip, sizeof(ip), fp)) {
                        ip[strcspn(ip, "\n")] = 0;
                        pclose(fp);
                        
                        if (strlen(ip) > 0 && strcmp(ip, "127.0.0.1") != 0) {  // 排除回环地址
                            printf("[DEBUG] 当前活动IP: %s\n", ip);
                            return g_strdup(ip);
                        }
                    }
                    pclose(fp);
                }
            }
        } else {
            pclose(fp);
        }
    }
    
    // ========== 修改开始 ==========
    // 检查是否有非回环的IP地址
    // 使用更精确的命令排除回环接口
    fp = popen("ip -4 addr show 2>/dev/null | grep -v ' lo:' | grep 'inet ' | head -1 | awk '{print $2}' | cut -d/ -f1", "r");
    if (fp) {
        char ip[64] = {0};
        if (fgets(ip, sizeof(ip), fp)) {
            ip[strcspn(ip, "\n")] = 0;
            pclose(fp);
            
            if (strlen(ip) > 0) {
                // 再次检查是否为回环地址（可能有多个回环接口）
                if (strncmp(ip, "127.", 4) != 0) {  // 排除所有127.x.x.x地址
                    printf("[DEBUG] 获取到非回环IP: %s\n", ip);
                    return g_strdup(ip);
                } else {
                    printf("[DEBUG] 排除回环地址: %s\n", ip);
                }
            }
        } else {
            pclose(fp);
        }
    }
    
    // 检查是否有活动的网络连接（非回环）
    fp = popen("nmcli -t -f DEVICE,CONNECTION,TYPE device status 2>/dev/null | grep -E '(wifi|ethernet)' | grep -v '^lo:' | head -1", "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            pclose(fp);
            // 解析设备状态
            char* device = strtok(line, ":");
            char* connection = strtok(NULL, ":");
            char* type = strtok(NULL, ":");
            
            if (device && connection && type) {
                connection[strcspn(connection, "\n")] = 0;
                printf("[DEBUG] 检测到网络设备: %s, 连接: %s, 类型: %s\n", device, connection, type);
                
                if (strcmp(connection, "--") != 0 && strlen(connection) > 0) {
                    printf("[DEBUG] 设备有连接但无IP\n");
                    return g_strdup("IP_GETTING");  // 有连接但无IP
                }
            }
        } else {
            pclose(fp);
        }
    }
    
    // 检查是否有物理网络接口存在
    fp = popen("ls /sys/class/net 2>/dev/null | grep -v '^lo$' | grep -E '^(eth|en|wlan|wlp|wls|wl)' | head -1", "r");
    if (fp) {
        char iface[64] = {0};
        if (fgets(iface, sizeof(iface), fp)) {
            iface[strcspn(iface, "\n")] = 0;
            pclose(fp);
            
            if (strlen(iface) > 0) {
                printf("[DEBUG] 检测到物理网络接口: %s\n", iface);
                return g_strdup("NO_IP_DETECTED");  // 有网卡但未连接
            }
        }
        pclose(fp);
    }
    
    // 最后检查：通过nmcli检查网络连接状态
    fp = popen("nmcli -t -f STATE general 2>/dev/null | head -1", "r");
    if (fp) {
        char state[32];
        if (fgets(state, sizeof(state), fp)) {
            state[strcspn(state, "\n")] = 0;
            pclose(fp);
            
            if (strcmp(state, "connected") == 0) {
                printf("[DEBUG] NetworkManager状态: 已连接\n");
                return g_strdup("IP_GETTING");
            } else if (strcmp(state, "connecting") == 0) {
                printf("[DEBUG] NetworkManager状态: 连接中\n");
                return g_strdup("IP_GETTING");
            } else {
                printf("[DEBUG] NetworkManager状态: %s\n", state);
            }
        } else {
            pclose(fp);
        }
    }
    // ========== 修改结束 ==========
    
    printf("[DEBUG] 无网络连接\n");
    return g_strdup("NO_IP_DETECTED");  // 默认返回"暂无数据"
}

// 获取热点名称（获取的SSID）使用SSID当热点名称
gchar* get_hotspot_name(void) {
    printf("[DEBUG] 获取热点名称...\n");
    
    // 方法1：从活跃连接获取热点名称
    FILE* fp = popen("nmcli -t -f NAME connection show --active 2>/dev/null | grep -i hotspot | head -1", "r");
    if (fp) {
        char conn_name[256];
        if (fgets(conn_name, sizeof(conn_name), fp)) {
            conn_name[strcspn(conn_name, "\n")] = 0;
            printf("[DEBUG] 找到活跃热点连接: %s\n", conn_name);
            
            // 获取这个连接的SSID
            pclose(fp);
            
            gchar cmd[512];
            snprintf(cmd, sizeof(cmd), 
                "nmcli -t -f 802-11-wireless.ssid connection show \"%s\" 2>/dev/null", 
                conn_name);
            
            fp = popen(cmd, "r");
            if (fp) {
                char buffer[256];
                if (fgets(buffer, sizeof(buffer), fp)) {
                    buffer[strcspn(buffer, "\n")] = 0;
                    printf("[DEBUG] 获取到SSID配置: '%s'\n", buffer);
                    
                    // 提取SSID
                    char* colon = strchr(buffer, ':');
                    if (colon) {
                        gchar* result = g_strdup(colon + 1);
                        pclose(fp);
                        
                        if (strlen(result) > 0) {
                            printf("[DEBUG] 热点名称: '%s'\n", result);
                            return result;
                        }
                        g_free(result);
                    }
                }
                pclose(fp);
            }
        } else {
            pclose(fp);
        }
    }
    
    printf("[DEBUG] 使用默认热点名称\n");
    return g_strdup("AI-Meeting");
}

// 获取开启热点的名称（非SSID）
// gchar* get_hotspot_connection_name(void) {
//     printf("[DEBUG] 获取热点连接名称...\n");
    
//     // 从活跃连接获取热点连接名称
//     FILE* fp = popen("nmcli -t -f NAME connection show --active 2>/dev/null | grep -i hotspot | head -1", "r");
//     if (fp) {
//         char conn_name[256];
//         if (fgets(conn_name, sizeof(conn_name), fp)) {
//             conn_name[strcspn(conn_name, "\n")] = 0;
//             printf("[DEBUG] 找到活跃热点连接: %s\n", conn_name);
//             pclose(fp);
//             return g_strdup(conn_name);
//         }
//         pclose(fp);
//     }
    
//     printf("[DEBUG] 使用默认热点名称\n");
//     return g_strdup("Hotspot");
// }

// 获取热点AP的IP地址（主机在热点网络中的IP）
gchar* get_hotspot_ap_ip(void) {
    printf("[DEBUG] 获取热点AP IP地址（热点模式）...\n");
    
    // 方法1：检查无线网卡是否处于AP模式，并获取其IP
    // 先找出所有无线接口
    FILE* fp = popen("iw dev 2>/dev/null | awk '/Interface/ {print $2}'", "r");
    if (!fp) {
        printf("[ERROR] 无法执行iw命令，可能没有安装wireless-tools\n");
        return g_strdup("10.42.0.1");
    }
    
    char wifi_iface[64] = {0};
    gchar* hotspot_ip = NULL;
    
    while (fgets(wifi_iface, sizeof(wifi_iface), fp)) {
        wifi_iface[strcspn(wifi_iface, "\n")] = 0;
        
        if (strlen(wifi_iface) > 0) {
            printf("[DEBUG] 检查无线接口: %s\n", wifi_iface);
            
            // 检查这个接口是否处于AP模式
            gchar check_cmd[256];
            snprintf(check_cmd, sizeof(check_cmd), 
                     "iw dev %s info 2>/dev/null | grep -q 'type AP' && echo 'AP_MODE'", 
                     wifi_iface);
            
            FILE* check_fp = popen(check_cmd, "r");
            if (check_fp) {
                char result[32] = {0};
                if (fgets(result, sizeof(result), check_fp)) {
                    if (strstr(result, "AP_MODE") != NULL) {
                        printf("[DEBUG] 接口 %s 处于AP模式（热点模式）\n", wifi_iface);
                        
                        // 现在获取这个AP接口的IP
                        gchar ip_cmd[256];
                        snprintf(ip_cmd, sizeof(ip_cmd),
                                 "ip -4 addr show dev %s 2>/dev/null | "
                                 "awk '/inet/ {print $2}' | cut -d/ -f1 | head -1",
                                 wifi_iface);
                        
                        pclose(check_fp);
                        FILE* ip_fp = popen(ip_cmd, "r");
                        if (ip_fp) {
                            char ip[64] = {0};
                            if (fgets(ip, sizeof(ip), ip_fp)) {
                                ip[strcspn(ip, "\n")] = 0;
                                if (strlen(ip) > 0) {
                                    printf("[DEBUG] 热点AP IP地址: %s (接口: %s)\n", ip, wifi_iface);
                                    pclose(ip_fp);
                                    pclose(fp);
                                    return g_strdup(ip);
                                }
                            }
                            pclose(ip_fp);
                        }
                    }
                }
                pclose(check_fp);
            }
        }
    }
    pclose(fp);

    printf("[DEBUG] 使用默认热点AP IP\n");
    return g_strdup("10.42.0.1");
}

// ========== UI回调函数实现 ==========

// 关闭对话框
static void on_close_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = GTK_WIDGET(data);
    gtk_widget_destroy(dialog);
}

// 刷新IP地址
static void on_refresh_clicked(GtkWidget *widget, gpointer data) {
    // GtkWidget *ip_label = GTK_WIDGET(data);
    // gchar* ip = get_device_ip();
    // gtk_label_set_text(GTK_LABEL(ip_label), ip);
    // g_free(ip);

    GtkWidget *ip_label = GTK_WIDGET(data);
    
    // 先移除所有特殊样式
    gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "no-data");
    gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "getting");
    
    gchar* ip = get_device_ip();
    
    if (strcmp(ip, "NO_IP_DETECTED") == 0) {
        gtk_label_set_text(GTK_LABEL(ip_label), "暂无数据");
        gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "no-data");
    } else if (strcmp(ip, "IP_GETTING") == 0) {
        gtk_label_set_text(GTK_LABEL(ip_label), "获取中...");
        gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "getting");
    } else {
        gtk_label_set_text(GTK_LABEL(ip_label), ip);
    }
    
    g_free(ip);
}

// 热点开关回调函数
static void on_toggle_hotspot(GtkWidget *widget, gpointer data) {
    // 防抖动：防止快速连续点击
    static gint64 last_click_time = 0;
    gint64 now = g_get_monotonic_time();
    if (now - last_click_time < 1000000) { // 1秒内只能点击一次
        printf("[DEBUG] 操作太快，请稍候\n");
        return;
    }
    last_click_time = now;
    
    // 从按钮获取当前状态
    gpointer state_ptr = g_object_get_data(G_OBJECT(widget), "hotspot_state");
    gboolean hotspot_on = (state_ptr != NULL) ? GPOINTER_TO_INT(state_ptr) : FALSE;
    
    GtkWidget *status_label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "status_label"));
    GtkWidget *name_label = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "name_label"));
    
    // 切换状态
    hotspot_on = !hotspot_on;
    
    if (hotspot_on) {
        // 开启热点
        printf("[DEBUG] 正在开启热点...\n");
        
        // 1. 更新UI状态
        gtk_label_set_text(GTK_LABEL(status_label), "正在开启热点...");

        // ========== 修改开始：更新IP显示为"获取中" ==========
        // 找到IP标签并更新
        GtkWidget *dialog = gtk_widget_get_toplevel(widget);
        GtkWidget *ip_label = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "ip_label_ref");
        if (ip_label) {
            gtk_label_set_text(GTK_LABEL(ip_label), "获取中...");
            // 设置样式
            gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "no-data");
            gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "getting");
            gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "getting");
        }
        // ========== 修改结束 ==========

        GtkWidget *image = gtk_image_new_from_file("Assets/images/on.png");
        gtk_button_set_image(GTK_BUTTON(widget), image);
        
        // 2. 执行开启命令
        int result = system("timeout 5 nmcli device wifi hotspot 2>/dev/null");
        
        if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
            // 3. 等待热点完全启动
            //sleep(2);

            // ========== 新增代码开始 ==========
            // gchar* hotspot_name11 = get_hotspot_connection_name();

            // if (strlen(hotspot_name11) > 0) {
            //     // 修改为无密码热点
            //     gchar modify_cmd[512];
            //     snprintf(modify_cmd, sizeof(modify_cmd),
            //         "nmcli connection modify \"%s\" 802-11-wireless-security.key-mgmt none 2>/dev/null && "
            //         "nmcli connection down \"%s\" 2>/dev/null && "
            //         "nmcli connection up \"%s\" 2>/dev/null",
            //         hotspot_name11, hotspot_name11, hotspot_name11);
                
            //     int modify_result = system(modify_cmd);
            //     if (WIFEXITED(modify_result) && WEXITSTATUS(modify_result) == 0) {
            //         printf("[DEBUG] 成功修改为无密码热点\n");
            //     } else {
            //         printf("[ERROR] 修改为无密码热点失败: %d\n", WEXITSTATUS(modify_result));
            //     }
            // }
            // ========== 新增代码结束 ==========

            gchar* hostpot_ip = get_hotspot_ap_ip();
            printf("热点IP:%s",hostpot_ip);
            // 4. 获取热点名称并更新UI
            gchar* hotspot_name = get_hotspot_name();
            gchar* formatted_text = g_strdup_printf("设备热点已开启。热点名称： %s 热点IP： %s", hotspot_name, hostpot_ip);
            gtk_label_set_text(GTK_LABEL(status_label), formatted_text);
            //gtk_label_set_text(GTK_LABEL(name_label), hotspot_name);
            
            g_free(formatted_text);
            g_free(hotspot_name);
            
            printf("[DEBUG] 热点开启成功\n");
            
            // 5. 保存新状态
            g_object_set_data(G_OBJECT(widget), "hotspot_state", GINT_TO_POINTER(TRUE));
            
        } else {
            // 开启失败
            gtk_label_set_text(GTK_LABEL(status_label), "热点开启失败，请检查网络");
            printf("[ERROR] 热点开启失败，错误码: %d\n", WEXITSTATUS(result));
            
            // 回退UI
            GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
            gtk_button_set_image(GTK_BUTTON(widget), image);
            
            // 保存状态
            g_object_set_data(G_OBJECT(widget), "hotspot_state", GINT_TO_POINTER(FALSE));
        }
        
    } else {
        // 关闭热点
        printf("[DEBUG] 正在关闭热点...\n");
        
        // 1. 更新UI状态
        gtk_label_set_text(GTK_LABEL(status_label), "正在关闭热点...");

        // ========== 修改开始：更新IP显示为"获取中" ==========
        GtkWidget *dialog = gtk_widget_get_toplevel(widget);
        GtkWidget *ip_label = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "ip_label_ref");
        if (ip_label) {
            gtk_label_set_text(GTK_LABEL(ip_label), "获取中...");
            gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "no-data");
            gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "getting");
            gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "getting");
        }
        // ========== 修改结束 =========

        GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
        gtk_button_set_image(GTK_BUTTON(widget), image);
        
        // 2. 执行关闭命令
        int result = system("timeout 5 nmcli connection down \"$(nmcli -t -f NAME connection show --active | grep -i hotspot)\" 2>/dev/null");
        
        if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
            // 关闭成功
            gtk_label_set_text(GTK_LABEL(status_label), 
                "设备热点已关闭。可通过上方的开关进行开启或按下设备上的");
            gtk_label_set_text(GTK_LABEL(name_label), "");
            printf("[DEBUG] 热点关闭成功\n");
            
            // 保存新状态
            g_object_set_data(G_OBJECT(widget), "hotspot_state", GINT_TO_POINTER(FALSE));
            
        } else {
            // 关闭失败
            gtk_label_set_text(GTK_LABEL(status_label), "热点关闭失败");
            printf("[ERROR] 热点关闭失败，错误码: %d\n", WEXITSTATUS(result));
            
            // 回退UI
            GtkWidget *image = gtk_image_new_from_file("Assets/images/on.png");
            gtk_button_set_image(GTK_BUTTON(widget), image);
            
            // 保存状态
            g_object_set_data(G_OBJECT(widget), "hotspot_state", GINT_TO_POINTER(TRUE));
        }
    }
}

// 前往系统设置
static void on_go_settings(GtkWidget *widget, gpointer data) {
    printf("[DEBUG] 打开网络设置...\n");
    system("gnome-control-center network 2>/dev/null 1>/dev/null &");
}

// WiFi图标点击回调函数
static void on_wifi_icon_clicked(GtkWidget *widget, gpointer data) {
    printf("[DEBUG] WiFi图标被点击，尝试开启热点\n");
    
    // 防抖动：防止快速连续点击
    static gint64 last_click_time = 0;
    gint64 now = g_get_monotonic_time();
    if (now - last_click_time < 1000000) { // 1秒内只能点击一次
        printf("[DEBUG] 操作太快，请稍候\n");
        return;
    }
    last_click_time = now;
    
    // 获取相关控件
    GtkWidget *dialog = gtk_widget_get_toplevel(widget);
    GtkWidget *toggle_btn = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "toggle_btn_ref");
    GtkWidget *status_label = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "status_label_ref");
    
    if (!toggle_btn || !status_label) {
        printf("[ERROR] 无法获取相关控件引用\n");
        return;
    }
    
    // 获取当前热点状态
    gpointer state_ptr = g_object_get_data(G_OBJECT(toggle_btn), "hotspot_state");
    gboolean hotspot_on = (state_ptr != NULL) ? GPOINTER_TO_INT(state_ptr) : FALSE;
    
    if (hotspot_on) {
        printf("[DEBUG] 热点已开启，无需操作\n");
        return;
    }
    
    printf("[DEBUG] 热点已关闭，开始开启热点\n");
    
    // 1. 更新UI状态
    gtk_label_set_text(GTK_LABEL(status_label), "正在开启热点...");
    
    // 2. 更新IP显示为"获取中"
    GtkWidget *ip_label = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "ip_label_ref");
    if (ip_label) {
        gtk_label_set_text(GTK_LABEL(ip_label), "获取中...");
        gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "no-data");
        gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "getting");
        gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "getting");
    }
    
    // 3. 更新开关按钮图标为"on.png"
    GtkWidget *image = gtk_image_new_from_file("Assets/images/on.png");
    gtk_button_set_image(GTK_BUTTON(toggle_btn), image);
    
    // 4. 执行开启命令
    int result = system("timeout 5 nmcli device wifi hotspot 2>/dev/null");
    
    if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
        // 5. 获取热点信息
        gchar* hostpot_ip = get_hotspot_ap_ip();
        printf("热点IP:%s", hostpot_ip);
        
        gchar* hotspot_name = get_hotspot_name();
        gchar* formatted_text = g_strdup_printf("设备热点已开启。热点名称： %s 热点IP： %s", hotspot_name, hostpot_ip);
        
        // 6. 更新状态标签
        gtk_label_set_text(GTK_LABEL(status_label), formatted_text);
        
        // 7. 更新开关按钮状态
        g_object_set_data(G_OBJECT(toggle_btn), "hotspot_state", GINT_TO_POINTER(TRUE));
        
        printf("[DEBUG] 热点开启成功\n");
        
        g_free(formatted_text);
        g_free(hotspot_name);
        
    } else {
        // 开启失败
        gtk_label_set_text(GTK_LABEL(status_label), "热点开启失败，请检查网络");
        printf("[ERROR] 热点开启失败，错误码: %d\n", WEXITSTATUS(result));
        
        // 回退UI
        GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
        gtk_button_set_image(GTK_BUTTON(toggle_btn), image);
        
        // 保存状态
        g_object_set_data(G_OBJECT(toggle_btn), "hotspot_state", GINT_TO_POINTER(FALSE));
    }
}

// ========== 主函数 ==========
void device_status_dialog_show(GtkWidget *parent) {
    // 1. 创建全屏窗口
    GtkWidget *dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(dialog), "设备状态");
    gtk_window_set_decorated(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ALWAYS);
    
    // 全屏显示
    gtk_window_fullscreen(GTK_WINDOW(dialog));
    
    // 获取屏幕尺寸
    GdkScreen *screen = gtk_widget_get_screen(dialog);
    gint screen_width = gdk_screen_get_width(screen);
    gint screen_height = gdk_screen_get_height(screen);
    
    // 2. 应用CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, DIALOG_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    // 3. 创建固定布局容器
    GtkWidget *fixed = gtk_fixed_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(fixed), "fullscreen-bg");
    gtk_container_add(GTK_CONTAINER(dialog), fixed);
    
    // 4. 创建标题栏容器
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title_bar), "title-bar-container");
    gtk_widget_set_size_request(title_bar, screen_width, 80);
    gtk_fixed_put(GTK_FIXED(fixed), title_bar, 0, 0);
    
    // 左侧占位
    GtkWidget *left_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(left_spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(title_bar), left_spacer, TRUE, TRUE, 0);
    
    // 标题（居中）
    GtkWidget *title = gtk_label_new("设备状态");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "dialog-title");
    gtk_box_pack_start(GTK_BOX(title_bar), title, FALSE, FALSE, 0);
    
    // 右侧占位
    GtkWidget *right_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(right_spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(title_bar), right_spacer, TRUE, TRUE, 0);
    
    // 关闭按钮（在右边）
    GtkWidget *close_btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(close_btn), "close-btn");
    GtkWidget *close_img = gtk_image_new_from_file("Assets/images/close.png");
    gtk_button_set_image(GTK_BUTTON(close_btn), close_img);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_can_focus(close_btn, FALSE);
    gtk_widget_set_margin_end(close_btn, 0);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), dialog);
    gtk_box_pack_end(GTK_BOX(title_bar), close_btn, FALSE, FALSE, 0);
    
    // 5. 计算水平排列卡片的位置
    gint total_width = 2 * 720 + 80;  // 两个卡片宽度 + 间距
    gint left_margin = (screen_width - total_width) / 2;
    gint top_margin = screen_height * 0.20;  // 标题栏下方20%开始
    
    // 6. 创建水平卡片容器
    GtkWidget *cards_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 80);
    gtk_style_context_add_class(gtk_widget_get_style_context(cards_hbox), "horizontal-cards-container");
    gtk_fixed_put(GTK_FIXED(fixed), cards_hbox, left_margin, top_margin);
    
    // 7. 第一个卡片：IP信息 (720x440)
    GtkWidget *ip_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(ip_card), "info-card");
    gtk_widget_set_size_request(ip_card, 720, 440);
    gtk_box_pack_start(GTK_BOX(cards_hbox), ip_card, FALSE, FALSE, 0);
    
    // IP卡片标题栏
    GtkWidget *ip_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(ip_header), "card-title-bar");
    gtk_box_pack_start(GTK_BOX(ip_card), ip_header, FALSE, FALSE, 0);
    
    GtkWidget *network_icon = gtk_image_new_from_file("Assets/images/network.png");
    gtk_box_pack_start(GTK_BOX(ip_header), network_icon, FALSE, FALSE, 0);
    
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(ip_header), spacer, TRUE, TRUE, 0);
    
    GtkWidget *settings_btn = gtk_button_new_with_label("前往设置 >");
    gtk_style_context_add_class(gtk_widget_get_style_context(settings_btn), "settings-link");
    gtk_button_set_relief(GTK_BUTTON(settings_btn), GTK_RELIEF_NONE);
    gtk_widget_set_can_focus(settings_btn, FALSE);
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_go_settings), NULL);
    gtk_box_pack_end(GTK_BOX(ip_header), settings_btn, FALSE, FALSE, 0);
    
    // 当前设备IP标题区域
    GtkWidget *ip_devicename = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(ip_devicename), "hotspot-ipwifi");
    gtk_box_pack_start(GTK_BOX(ip_card), ip_devicename, TRUE, TRUE, 0);
    
    GtkWidget *ip_title = gtk_label_new("当前设备IP");
    gtk_style_context_add_class(gtk_widget_get_style_context(ip_title), "card-title-text");
    gtk_widget_set_halign(ip_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(ip_devicename), ip_title, FALSE, FALSE, 0);
    
    // IP显示和刷新区域
    GtkWidget *ip_refresh = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(ip_refresh), "ip-external-container");
    gtk_box_pack_start(GTK_BOX(ip_card), ip_refresh, FALSE, FALSE, 0);
    
    // IP显示区域
    GtkWidget *ip_display = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(ip_display), "ip-display");
    gtk_box_pack_start(GTK_BOX(ip_refresh), ip_display, FALSE, FALSE, 0);
    
    // gchar* ip = get_device_ip();
    // GtkWidget *ip_label = gtk_label_new(ip);
    // gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "ip-value");
    // gtk_widget_set_halign(ip_label, GTK_ALIGN_START);
    // gtk_widget_set_hexpand(ip_label, TRUE);
    // gtk_box_pack_start(GTK_BOX(ip_display), ip_label, TRUE, TRUE, 0);
    // g_free(ip);
    
    gchar* ip = get_device_ip();
    GtkWidget *ip_label = gtk_label_new(ip);
    gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "ip-value");
    gtk_widget_set_halign(ip_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(ip_label, TRUE);

    // 根据IP结果设置文本和样式
    if (strcmp(ip, "NO_IP_DETECTED") == 0) {
        gtk_label_set_text(GTK_LABEL(ip_label), "暂无数据");
        // 添加红色文本样式类
        gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "no-data");
    } else if (strcmp(ip, "IP_GETTING") == 0) {
        gtk_label_set_text(GTK_LABEL(ip_label), "获取中...");
        // 添加获取中样式类
        gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "getting");
    } else {
        gtk_label_set_text(GTK_LABEL(ip_label), ip);
    }

    gtk_box_pack_start(GTK_BOX(ip_display), ip_label, TRUE, TRUE, 0);
    g_free(ip);

    // 刷新按钮
    GtkWidget *refresh_btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(refresh_btn), "refresh-btn");
    GtkWidget *refresh_img = gtk_image_new_from_file("Assets/images/refresh.png");
    gtk_button_set_image(GTK_BUTTON(refresh_btn), refresh_img);
    gtk_button_set_relief(GTK_BUTTON(refresh_btn), GTK_RELIEF_NONE);
    gtk_widget_set_can_focus(refresh_btn, FALSE);
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), ip_label);
    gtk_box_pack_end(GTK_BOX(ip_refresh), refresh_btn, FALSE, FALSE, 0);
    
    // 8. 第二个卡片：热点信息 (720x440)
    GtkWidget *hotspot_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(hotspot_card), "info-card");
    gtk_widget_set_size_request(hotspot_card, 720, 440);
    gtk_box_pack_start(GTK_BOX(cards_hbox), hotspot_card, FALSE, FALSE, 0);
    
    // 热点卡片标题栏
    GtkWidget *hotspot_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(hotspot_header), "card-title-bar");
    gtk_box_pack_start(GTK_BOX(hotspot_card), hotspot_header, FALSE, FALSE, 0);
    
    // GtkWidget *wifi_icon = gtk_image_new_from_file("Assets/images/wifi.png");
    // gtk_box_pack_start(GTK_BOX(hotspot_header), wifi_icon, FALSE, FALSE, 0);
    GtkWidget *wifi_icon_btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(wifi_icon_btn), "wifi-icon-btn");  // 添加CSS类
    GtkWidget *wifi_img = gtk_image_new_from_file("Assets/images/wifi.png");
    gtk_button_set_image(GTK_BUTTON(wifi_icon_btn), wifi_img);
    gtk_button_set_relief(GTK_BUTTON(wifi_icon_btn), GTK_RELIEF_NONE);
    gtk_widget_set_can_focus(wifi_icon_btn, FALSE);
    g_signal_connect(wifi_icon_btn, "clicked", G_CALLBACK(on_wifi_icon_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hotspot_header), wifi_icon_btn, FALSE, FALSE, 0);
    

    spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(hotspot_header), spacer, TRUE, TRUE, 0);
    
    // 热点开关按钮
    GtkWidget *toggle_btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(toggle_btn), "toggle-switch");
    gtk_button_set_relief(GTK_BUTTON(toggle_btn), GTK_RELIEF_NONE);
    gtk_widget_set_can_focus(toggle_btn, FALSE);
    
    // 检测系统热点状态
    gboolean hotspot_active = is_hotspot_active();
    
    // 根据状态设置按钮图标
    if (hotspot_active) {
        GtkWidget *image = gtk_image_new_from_file("Assets/images/on.png");
        gtk_button_set_image(GTK_BUTTON(toggle_btn), image);
    } else {
        GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
        gtk_button_set_image(GTK_BUTTON(toggle_btn), image);
    }
    
    gtk_box_pack_end(GTK_BOX(hotspot_header), toggle_btn, FALSE, FALSE, 0);
    
    // 设备热点标题区域
    GtkWidget *hotspot_devicetitle = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(hotspot_devicetitle), "hotspot-ipwifi");
    gtk_box_pack_start(GTK_BOX(hotspot_card), hotspot_devicetitle, TRUE, TRUE, 0);
    
    GtkWidget *hotspot_title = gtk_label_new("设备热点");
    gtk_style_context_add_class(gtk_widget_get_style_context(hotspot_title), "card-title-text");
    gtk_widget_set_halign(hotspot_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hotspot_devicetitle), hotspot_title, FALSE, FALSE, 0);
    
    // 热点内容区域
    GtkWidget *hotspot_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(hotspot_content), "hotspot-content");
    gtk_box_pack_start(GTK_BOX(hotspot_card), hotspot_content, TRUE, TRUE, 0);
    
    // 初始化状态标签
    GtkWidget *name_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(name_label), "hotspot-name");
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    
    GtkWidget *status_label = NULL;
    
    if (hotspot_active) {
        //获取热点AP IP
        gchar* hostpot_ip = get_hotspot_ap_ip();
        // 热点已开启，获取名称
        gchar* hotspot_name = get_hotspot_name();
        gchar* formatted_text = g_strdup_printf("设备热点已开启。热点名称： %s 热点IP： %s", hotspot_name, hostpot_ip);
        status_label = gtk_label_new(formatted_text);
        
        // 设置热点名称标签
        //gtk_label_set_text(GTK_LABEL(name_label), hotspot_name);
        
        g_free(formatted_text);
        g_free(hotspot_name);
    } else {
        // 热点已关闭
        status_label = gtk_label_new("设备热点已关闭。可通过上方的开关进行开启或按下设备上的");
    }
    
    gtk_style_context_add_class(gtk_widget_get_style_context(status_label), "hotspot-desc");
    gtk_label_set_line_wrap(GTK_LABEL(status_label), TRUE);
    //gtk_label_set_line_wrap_mode(GTK_LABEL(status_label), PANGO_WRAP_WORD);   //按单词换行
    gtk_label_set_line_wrap_mode(GTK_LABEL(status_label), PANGO_WRAP_CHAR);      //按字符换行
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hotspot_content), status_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hotspot_content), name_label, FALSE, FALSE, 0);
    
    // 保存引用用于回调
    g_object_set_data(G_OBJECT(toggle_btn), "status_label", status_label);
    g_object_set_data(G_OBJECT(toggle_btn), "name_label", name_label);
    g_signal_connect(toggle_btn, "clicked", G_CALLBACK(on_toggle_hotspot), NULL);
    
    // 保存初始状态到按钮
    g_object_set_data(G_OBJECT(toggle_btn), "hotspot_state", GINT_TO_POINTER(hotspot_active));
    
    // 在创建IP标签后，保存引用
    g_object_set_data(G_OBJECT(dialog), "ip_label_ref", ip_label);

    // 在这里保存toggle_btn引用
    g_object_set_data(G_OBJECT(dialog), "toggle_btn_ref", toggle_btn);
    // 在这里保存status_label的引用，供WiFi图标回调使用
    g_object_set_data(G_OBJECT(dialog), "status_label_ref", status_label);

    // 9. 显示所有组件
    gtk_widget_show_all(dialog);
    
    // 10. 清理
    g_object_unref(provider);
}