#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <gtk/gtk.h>

// 异步打开系统WiFi设置，不阻塞GTK主循环
void wifi_control_open_settings(void);

#endif
