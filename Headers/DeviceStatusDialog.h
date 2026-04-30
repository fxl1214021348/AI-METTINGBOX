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

// ========== 回调函数声明 ==========
// 关闭按钮回调函数
void on_close_clicked(GtkWidget *widget, gpointer data);

// 刷新按钮回调函数
void on_refresh_clicked(GtkWidget *widget, gpointer data);

// 热点开关按钮回调函数
void on_toggle_hotspot(GtkWidget *widget, gpointer data);

// 前往设置按钮回调函数
void on_go_settings(GtkWidget *widget, gpointer data);

// WiFi图标点击回调函数
void on_wifi_icon_clicked(GtkWidget *widget, gpointer data);

// 从外部更新热点UI状态(串口等场景调用，需在GTK主线程中调用)
void device_status_update_hotspot_ui(gboolean hotspot_on);

// 更新热点UI为"操作中"状态（开启中/关闭中，需在GTK主线程中调用）
// turning_on: TRUE表示"正在开启热点..."，FALSE表示"正在关闭热点..."
void device_status_update_hotspot_ui_loading(gboolean turning_on);

#endif // DEVICE_STATUS_DIALOG_H