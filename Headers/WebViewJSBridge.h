#ifndef WEBVIEW_JSBRIDGE_H
#define WEBVIEW_JSBRIDGE_H

#include <webkit2/webkit2.h>
#include <JavaScriptCore/JavaScript.h>  // 需要包含这个头文件

#ifdef __cplusplus
extern "C" {
#endif

// 消息回调函数类型
typedef void (*MessageCallback)(const char *message, void *user_data);

// 初始化通信功能
void webview_bridge_init(WebKitWebView *webView);

// 注册JS -> C++的消息处理器
void webview_bridge_register(WebKitWebView *webView,
                             const char *handlerName,
                             MessageCallback callback,
                             void *user_data);

// 执行JS代码（C++ -> JS）
void webview_bridge_execute(WebKitWebView *webView, const char *script);

// 向JS发送消息
void webview_bridge_send(WebKitWebView *webView, const char *message);

// 清理通信功能
void webview_bridge_cleanup(WebKitWebView *webView);

#ifdef __cplusplus
}
#endif

#endif