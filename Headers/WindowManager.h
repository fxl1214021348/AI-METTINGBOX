#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <gtk/gtk.h>

// 创建kiosk模式主窗口（全屏、无装饰、阻止关闭）
// 返回：窗口实例，内部已组装WebView和悬浮按钮
GtkWidget* window_create_kiosk(void);

// 关闭请求回调（阻止默认关闭行为）
gboolean window_on_close_request(GtkWidget *widget, GdkEvent *event, gpointer data);

// 销毁回调（退出GTK主循环）
void window_on_destroy(GtkWidget *widget, gpointer data);

#endif
