#include "WebView.h"
#include <stdio.h>

WebKitWebView* webview_create(void) {
   return WEBKIT_WEB_VIEW(webkit_web_view_new());
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
