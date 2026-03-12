#ifndef DEVICE_STATUS_DIALOG_H
#define DEVICE_STATUS_DIALOG_H

#include <gtk/gtk.h>
#include <string>

// 设备状态结构
// typedef struct {
//     gboolean hotspot_enabled;       // 热点是否开启
//     gchar *hotspot_name;            // 热点名称（动态获取）
//     gchar *device_ip;               // 设备IP地址
// } DeviceStatus;

// 创建并显示设备状态弹窗
// parent: 父窗口（你的主WebView窗口）
void device_status_dialog_show(GtkWidget *parent);

// 获取当前设备IP地址（通过NetworkManager或系统命令）
gchar* get_device_ip(void);

// 获取热点名称SSID，使用SSID充当热点名称
gchar* get_hotspot_name(void);

// 获取热点连接名称（连接名称​ = Hotspot-163(NetworkManager中的连接配置名)）
gchar* get_hotspot_connection_name(void);

// 获取热点状态（通过nmcli或D-Bus）
gboolean get_hotspot_status(gchar **hotspot_name_out);

// 切换热点状态（开启/关闭）
gboolean toggle_hotspot(gboolean enable, const gchar *ssid, const gchar *password);

#endif // DEVICE_STATUS_DIALOG_H