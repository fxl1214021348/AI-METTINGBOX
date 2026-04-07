#ifndef HOTSPOTUTILS_H
#define HOTSPOTUTILS_H

#include <gtk/gtk.h>

//监控热点状态是否开启
gboolean is_hotspot_active(void);

//获取当前设备IP地址（通过NetworkManager或系统命令）
gchar* get_device_ip(void);

// 获取热点名称（获取的SSID）使用SSID当热点名称
gchar* get_hotspot_name(void);

// 获取开启热点的名称（非SSID）
// gchar* get_hotspot_connection_name(void);

// 获取热点AP的IP地址（主机在热点网络中的IP）
gchar* get_hotspot_ap_ip(void);

// 开启热点（纯逻辑，不涉及UI），返回TRUE表示成功
gboolean hotspot_enable(void);

// 关闭热点（纯逻辑，不涉及UI），返回TRUE表示成功
gboolean hotspot_disable(void);

#endif