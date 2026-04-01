#include "DeviceStatusDialog.h"
#include "HotspotUtils.h"
#include "Styles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ========== UI回调函数实现 ==========

// 关闭对话框
void on_close_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = GTK_WIDGET(data);
    gtk_widget_destroy(dialog);
}

// 刷新IP地址
void on_refresh_clicked(GtkWidget *widget, gpointer data) {
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
void on_toggle_hotspot(GtkWidget *widget, gpointer data) {
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
        // ========== 开启热点 - 修改开始 ==========
        printf("[DEBUG] 正在开启无密码热点...\n");
        
        // 1. 更新UI状态
        gtk_label_set_text(GTK_LABEL(status_label), "正在开启热点...");

        // 更新IP显示为"获取中"
        GtkWidget *dialog = gtk_widget_get_toplevel(widget);
        GtkWidget *ip_label = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "ip_label_ref");
        if (ip_label) {
            gtk_label_set_text(GTK_LABEL(ip_label), "获取中...");
            gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "no-data");
            gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "getting");
            gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "getting");
        }

        GtkWidget *image = gtk_image_new_from_file("Assets/images/on.png");
        gtk_button_set_image(GTK_BUTTON(widget), image);
        
        // 2. 获取无线网卡名称
        char wifi_iface[64] = {0};
        FILE *fp = popen("ip link | awk -F': ' '/^[0-9]+: wl/{print $2; exit}'", "r");
        if (fp) {
            if (fgets(wifi_iface, sizeof(wifi_iface), fp)) {
                wifi_iface[strcspn(wifi_iface, "\n")] = 0;
            }
            pclose(fp);
        }
        
        if (strlen(wifi_iface) == 0) {
            // 如果没找到wl开头的，尝试其他可能的无线网卡名
            fp = popen("ip link | awk -F': ' '/^[0-9]+: (wlan|wlp|wls)/{print $2; exit}'", "r");
            if (fp) {
                if (fgets(wifi_iface, sizeof(wifi_iface), fp)) {
                    wifi_iface[strcspn(wifi_iface, "\n")] = 0;
                }
                pclose(fp);
            }
        }
        
        const char* ssid = "AI-Meeting";  // 热点名称，可以根据需要修改
        
        // 3. 删除可能存在的旧热点连接（避免冲突）
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), 
            "nmcli connection delete \"%s\" 2>/dev/null", ssid);
        system(cmd);
        
        // 4. 创建无密码热点连接
        // 注意：由于配置了PolicyKit，现在不需要sudo
        if (strlen(wifi_iface) > 0) {
            snprintf(cmd, sizeof(cmd),
                "nmcli connection add type wifi ifname %s con-name \"%s\" ssid \"%s\" "
                "wifi.mode ap ipv4.method shared 2>/dev/null",
                wifi_iface, ssid, ssid);
        } else {
            // 如果无法自动获取网卡名，尝试不指定网卡（让nmcli自动选择）
            snprintf(cmd, sizeof(cmd),
                "nmcli connection add type wifi con-name \"%s\" ssid \"%s\" "
                "wifi.mode ap ipv4.method shared 2>/dev/null",
                ssid, ssid);
        }
        
        int result = system(cmd);
        
        if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
            // 5. 启动热点
            snprintf(cmd, sizeof(cmd), "nmcli connection up \"%s\" 2>/dev/null", ssid);
            result = system(cmd);
            
            if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
                // 6. 获取热点信息并更新UI
                gchar* hostpot_ip = get_hotspot_ap_ip();
                printf("热点IP:%s", hostpot_ip);
                
                gchar* formatted_text = g_strdup_printf("设备热点已开启。热点名称： %s 热点IP： %s", ssid, hostpot_ip);
                gtk_label_set_text(GTK_LABEL(status_label), formatted_text);
                
                g_free(formatted_text);
                
                printf("[DEBUG] 无密码热点开启成功\n");
                
                // 7. 保存新状态
                g_object_set_data(G_OBJECT(widget), "hotspot_state", GINT_TO_POINTER(TRUE));
            } else {
                // 启动失败
                gtk_label_set_text(GTK_LABEL(status_label), "热点启动失败");
                printf("[ERROR] 热点启动失败\n");
                
                // 回退UI
                GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
                gtk_button_set_image(GTK_BUTTON(widget), image);
                g_object_set_data(G_OBJECT(widget), "hotspot_state", GINT_TO_POINTER(FALSE));
            }
        } else {
            // 创建连接失败
            gtk_label_set_text(GTK_LABEL(status_label), "热点创建失败，请检查网络");
            printf("[ERROR] 热点创建失败\n");
            
            GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
            gtk_button_set_image(GTK_BUTTON(widget), image);
            g_object_set_data(G_OBJECT(widget), "hotspot_state", GINT_TO_POINTER(FALSE));
        }
        // ========== 开启热点 - 修改结束 ==========
        
    } else {
        // ========== 关闭热点 - 修改开始 ==========
        printf("[DEBUG] 正在关闭热点...\n");
        
        // 1. 更新UI状态
        gtk_label_set_text(GTK_LABEL(status_label), "正在关闭热点...");

        // 更新IP显示为"获取中"
        GtkWidget *dialog = gtk_widget_get_toplevel(widget);
        GtkWidget *ip_label = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "ip_label_ref");
        if (ip_label) {
            gtk_label_set_text(GTK_LABEL(ip_label), "获取中...");
            gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "no-data");
            gtk_style_context_remove_class(gtk_widget_get_style_context(ip_label), "getting");
            gtk_style_context_add_class(gtk_widget_get_style_context(ip_label), "getting");
        }

        GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
        gtk_button_set_image(GTK_BUTTON(widget), image);
        
        // 2. 关闭指定名称的热点
        const char* ssid = "AI-Meeting";  // 与开启时保持一致
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "nmcli connection down \"%s\" 2>/dev/null", ssid);
        int result = system(cmd);
        
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
            printf("[ERROR] 热点关闭失败\n");
            
            // 回退UI
            GtkWidget *image = gtk_image_new_from_file("Assets/images/on.png");
            gtk_button_set_image(GTK_BUTTON(widget), image);
            g_object_set_data(G_OBJECT(widget), "hotspot_state", GINT_TO_POINTER(TRUE));
        }
        // ========== 关闭热点 - 修改结束 ==========
    }
}

// 前往系统设置
void on_go_settings(GtkWidget *widget, gpointer data) {
    printf("[DEBUG] 打开网络设置...\n");
    system("gnome-control-center network 2>/dev/null 1>/dev/null &");
}

// WiFi图标点击回调函数
void on_wifi_icon_clicked(GtkWidget *widget, gpointer data) {
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
    
    // ========== WiFi图标开启热点 - 添加开始 ==========
    // 获取无线网卡名称
    char wifi_iface[64] = {0};
    FILE *fp = popen("ip link | awk -F': ' '/^[0-9]+: wl/{print $2; exit}'", "r");
    if (fp) {
        if (fgets(wifi_iface, sizeof(wifi_iface), fp)) {
            wifi_iface[strcspn(wifi_iface, "\n")] = 0;
        }
        pclose(fp);
    }
    
    if (strlen(wifi_iface) == 0) {
        fp = popen("ip link | awk -F': ' '/^[0-9]+: (wlan|wlp|wls)/{print $2; exit}'", "r");
        if (fp) {
            if (fgets(wifi_iface, sizeof(wifi_iface), fp)) {
                wifi_iface[strcspn(wifi_iface, "\n")] = 0;
            }
            pclose(fp);
        }
    }
    
    const char* ssid = "AI-Meeting";
    
    // 删除旧热点连接
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "nmcli connection delete \"%s\" 2>/dev/null", ssid);
    system(cmd);
    
    // 创建无密码热点连接
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
    
    if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
        // 启动热点
        snprintf(cmd, sizeof(cmd), "nmcli connection up \"%s\" 2>/dev/null", ssid);
        result = system(cmd);
        
        if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
            // 获取热点信息
            gchar* hostpot_ip = get_hotspot_ap_ip();
            printf("热点IP:%s", hostpot_ip);
            
            gchar* formatted_text = g_strdup_printf("设备热点已开启。热点名称： %s 热点IP： %s", ssid, hostpot_ip);
            
            // 更新状态标签
            gtk_label_set_text(GTK_LABEL(status_label), formatted_text);
            
            // 更新开关按钮状态
            g_object_set_data(G_OBJECT(toggle_btn), "hotspot_state", GINT_TO_POINTER(TRUE));
            
            printf("[DEBUG] 无密码热点开启成功\n");
            
            g_free(formatted_text);
        } else {
            // 启动失败
            gtk_label_set_text(GTK_LABEL(status_label), "热点启动失败，请检查网络");
            printf("[ERROR] 热点启动失败\n");
            
            GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
            gtk_button_set_image(GTK_BUTTON(toggle_btn), image);
            g_object_set_data(G_OBJECT(toggle_btn), "hotspot_state", GINT_TO_POINTER(FALSE));
        }
    } else {
        // 创建连接失败
        gtk_label_set_text(GTK_LABEL(status_label), "热点创建失败，请检查网络");
        printf("[ERROR] 热点创建失败\n");
        
        GtkWidget *image = gtk_image_new_from_file("Assets/images/off.png");
        gtk_button_set_image(GTK_BUTTON(toggle_btn), image);
        g_object_set_data(G_OBJECT(toggle_btn), "hotspot_state", GINT_TO_POINTER(FALSE));
    }
    // ========== WiFi图标开启热点 - 添加结束 ==========
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
    gtk_fixed_put(GTK_FIXED(fixed), title_bar, 0, 60);
    
    // 左侧占位
    GtkWidget *left_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(left_spacer, TRUE);
    // 设置左边距，补偿关闭按钮的宽度
    gtk_widget_set_margin_end(left_spacer, 100);  // 关键：右侧增加40px外边距
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