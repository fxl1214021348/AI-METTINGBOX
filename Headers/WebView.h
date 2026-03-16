#ifndef WEBVIEW_HANDLER_H
#define WEBVIEW_HANDLER_H

#include <webkit2/webkit2.h>

// 创建并配置WebView实例（已集成JS桥接）
WebKitWebView* webview_create(void);

// 加载URL（自动去除首尾空格）
void webview_load_url(WebKitWebView *webView, const gchar *url);

// 新窗口请求处理回调（用于连接create信号）
// 功能：在当前窗口打开新链接，而不是弹出窗口
WebKitWebView* webview_on_create_new_window(WebKitWebView *web_view, 
                                             WebKitNavigationAction *action,
                                             gpointer user_data);

#endif
