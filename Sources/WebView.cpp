// #include "WebView.h"
// #include <stdio.h>

// WebKitWebView* webview_create(void) {
//    return WEBKIT_WEB_VIEW(webkit_web_view_new());
// }

// void webview_load_url(WebKitWebView *webView, const gchar *url) {
//     // 防御性编程：去除首尾空格，避免URL加载失败
//     gchar *trimmed = g_strdup(url);
//     g_strstrip(trimmed);
    
//     g_print("加载URL: %s\n", trimmed);
//     webkit_web_view_load_uri(webView, trimmed);
//     g_free(trimmed);
// }

// WebKitWebView* webview_on_create_new_window(WebKitWebView *web_view, 
//                                              WebKitNavigationAction *action,
//                                              gpointer user_data) {
//     WebKitURIRequest *request = webkit_navigation_action_get_request(action);
//     const gchar *uri = webkit_uri_request_get_uri(request);
    
//     g_print("拦截新窗口请求，在当前页打开: %s\n", uri);
    
//     // 在当前WebView加载，阻止真正的新窗口弹出
//     webkit_web_view_load_uri(web_view, uri);
//     return NULL;  // NULL表示在当前web_view处理
// }

#include "WebView.h"
#include <stdio.h>

/**
 * @brief 权限请求回调 —— 自动批准麦克风/摄像头访问
 * 
 * WebKitGTK 默认拒绝所有 getUserMedia() 请求，
 * 必须显式监听 permission-request 信号并调用 allow()
 */
static gboolean
on_permission_request(WebKitWebView *web_view,
                      WebKitPermissionRequest *request,
                      gpointer user_data)
{
    if (WEBKIT_IS_USER_MEDIA_PERMISSION_REQUEST(request)) {
        g_print("[WebView] 批准麦克风/摄像头权限请求\n");
        webkit_permission_request_allow(request);
        return TRUE;
    }

    // 通知权限也一并批准（如果需要的话）
    if (WEBKIT_IS_NOTIFICATION_PERMISSION_REQUEST(request)) {
        g_print("[WebView] 批准通知权限请求\n");
        webkit_permission_request_allow(request);
        return TRUE;
    }

    return FALSE;
}

WebKitWebView* webview_create(void) {
    WebKitWebView *webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

    // 配置 WebKitSettings：启用媒体流（麦克风/摄像头）
    WebKitSettings *settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_enable_media_stream(settings, TRUE);
    webkit_settings_set_enable_mediasource(settings, TRUE);
    //webkit_settings_set_enable_webrtc(settings, TRUE);
    g_print("[WebView] 已启用 media-stream / media-source / webrtc\n");

    // 连接权限请求信号 —— 这是关键！
    g_signal_connect(webView, "permission-request",
                     G_CALLBACK(on_permission_request), NULL);

    return webView;
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
