#ifndef WEBVIEW_JS_BRIDGE_H
#define WEBVIEW_JS_BRIDGE_H

#include <webkit2/webkit2.h>

// 初始化WebView的JS通信桥接（注册message handler、注入脚本）
// 返回：配置好的WebKitUserContentManager
WebKitUserContentManager* js_bridge_init(void);

// C++主动发送消息给网页JS
// web_view: 目标WebView
// message: 要发送的字符串（会被JS的onMessageFromCpp接收）
void js_bridge_send_to_web(WebKitWebView *web_view, const gchar *message);

// 获取JS调用时的参数字符串（辅助函数）
// js_result: WebKitJavascriptResult对象
// 返回：gchar* 需要调用者用g_free释放
gchar* js_bridge_get_message_string(WebKitJavascriptResult *js_result);

#endif
