#include "HotspotUtils.h"

// 检测系统热点是否实际开启
gboolean is_hotspot_active(void) {
    // 检查我们指定的热点名称是否处于活跃状态
    const char* ssid = "AI-Meeting";
    char cmd[256];
    snprintf(cmd, sizeof(cmd), 
        "nmcli -t -f NAME connection show --active 2>/dev/null | grep -q \"^%s$\"", ssid);
    int ret = system(cmd);
    
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        printf("[DEBUG] 热点状态检测: 已开启 (SSID: %s)\n", ssid);
        return TRUE;
    }
    
    printf("[DEBUG] 热点状态检测: 已关闭\n");
    return FALSE;
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
    FILE* fp = popen("nmcli -t -f NAME connection show --active 2>/dev/null | grep -i AI-Meeting | head -1", "r");
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

// 开启热点（纯逻辑，不涉及UI）
gboolean hotspot_enable(void) {
    if (is_hotspot_active()) {
        printf("[热点] 热点已处于开启状态，无需重复开启\n");
        return TRUE;
    }

    printf("[热点] 正在开启热点...\n");

    // 获取无线网卡名称
    char wifi_iface[64] = {0};
    FILE *fp = popen("ip link | awk -F': ' '/^[0-9]+: wl/{print $2; exit}'", "r");
    if (fp) {
        if (fgets(wifi_iface, sizeof(wifi_iface), fp))
            wifi_iface[strcspn(wifi_iface, "\n")] = 0;
        pclose(fp);
    }

    const char* ssid = "AI-Meeting";
    char cmd[1024];

    // 删除旧连接
    snprintf(cmd, sizeof(cmd), "nmcli connection delete \"%s\" 2>/dev/null", ssid);
    system(cmd);

    // 创建热点连接
    if (strlen(wifi_iface) > 0) {
        snprintf(cmd, sizeof(cmd),
            "nmcli connection add type wifi ifname %s con-name \"%s\" ssid \"%s\" "
            "wifi.mode ap ipv4.method shared 2>/dev/null",
            wifi_iface, ssid, ssid);
    } else {
        snprintf(cmd, sizeof(cmd),
            "nmcli connection add type wifi con-name \"%s\" ssid \"%s\" "
            "wifi.mode ap ipv4.method shared 2>/dev/null",
            ssid, ssid);
    }

    int result = system(cmd);
    if (!WIFEXITED(result) || WEXITSTATUS(result) != 0) {
        printf("[热点] 创建热点连接失败\n");
        return FALSE;
    }

    // 启动热点
    snprintf(cmd, sizeof(cmd), "nmcli connection up \"%s\" 2>/dev/null", ssid);
    result = system(cmd);
    if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
        printf("[热点] 热点开启成功\n");
        return TRUE;
    }

    printf("[热点] 热点启动失败\n");
    return FALSE;
}

// 关闭热点（纯逻辑，不涉及UI）
gboolean hotspot_disable(void) {
    if (!is_hotspot_active()) {
        printf("[热点] 热点已处于关闭状态，无需重复关闭\n");
        return TRUE;
    }

    printf("[热点] 正在关闭热点...\n");

    const char* ssid = "AI-Meeting";
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nmcli connection down \"%s\" 2>/dev/null", ssid);
    int result = system(cmd);

    if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
        printf("[热点] 热点关闭成功\n");
        return TRUE;
    }

    printf("[热点] 热点关闭失败\n");
    return FALSE;
}