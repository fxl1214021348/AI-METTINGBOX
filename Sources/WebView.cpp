#include "WebView.h"
#include "WebViewJSBridge.h"
#include <stdio.h>

//WebKitWebView* webview_create(void) {
//    return WEBKIT_WEB_VIEW(webkit_web_view_new());
//}

WebKitWebView* webview_create(void) {
    // 初始化JS桥接，获取配置好的UserContentManager
    WebKitUserContentManager *ucm = js_bridge_init();

    // 使用带JS桥接的构造方式创建WebView
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(
        webkit_web_view_new_with_user_content_manager(ucm)
    );

    // 关键：重新设置signal的user_data为web_view，让回调能发消息回来
    // 先找到之前的handler ID断开，再重新连接（或者直接在js_bridge_init里不传user_data）
    // 简化方案：在js_bridge_init里不连接信号，在这里连接

    // 实际上更好的设计：把g_signal_connect移到外面，见window_manager

    return web_view;
}

void webview_load_url(WebKitWebView *webView, const gchar *url) {
    // 防御性编程：去除首尾空格，避免URL加载失败
    gchar *trimmed = g_strdup(url);
    g_strstrip(trimmed);
    
    g_print("加载URL: %s\n", trimmed);
    webkit_web_view_load_uri(webView, trimmed);
    g_free(trimmed);
}

WebKitWebView* webview_on_create_new_window(WebKitWebView *web_view, 
                                             WebKitNavigationAction *action,
                                             gpointer user_data) {
    WebKitURIRequest *request = webkit_navigation_action_get_request(action);
    const gchar *uri = webkit_uri_request_get_uri(request);
    
    g_print("拦截新窗口请求，在当前页打开: %s\n", uri);
    
    // 在当前WebView加载，阻止真正的新窗口弹出
    webkit_web_view_load_uri(web_view, uri);
    return NULL;  // NULL表示在当前web_view处理
}

void webview_on_load_changed(WebKitWebView *web_view,
                              WebKitLoadEvent event,
                              gpointer user_data) {
    if (event == WEBKIT_LOAD_FINISHED) {
        g_print("页面加载完成，发送就绪通知\n");
        js_bridge_send_to_web(web_view, "C++后端已就绪，支持功能: wifi,audio");
    }
}
