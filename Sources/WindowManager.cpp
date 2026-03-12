#include "WindowManager.h"
#include "WebView.h"
#include "WebViewJSBridge.h"
#include "ExitSettingButton.h"
#include <iostream> 

static void on_js_message_received(WebKitUserContentManager *manager,
                                   WebKitJavascriptResult *js_result,
                                   gpointer user_data) {
    WebKitWebView *webView = WEBKIT_WEB_VIEW(user_data);
    gchar *msg = js_bridge_get_message_string(js_result);
    
    if (msg) {
        std::cout << "[处理] " << msg << std::endl;
        
        if (g_str_has_prefix(msg, "WIFI:")) {
            js_bridge_send_to_web(webView, "WiFi功能收到");
        } else if (g_str_has_prefix(msg, "AUDIO:")) {
            js_bridge_send_to_web(webView, "音频功能收到");
        } else {
            js_bridge_send_to_web(webView, "未知命令");
        }
        g_free(msg);
    }
}

gboolean window_on_close_request(GtkWidget *widget, GdkEvent *event, gpointer data) {
    g_print("阻止窗口关闭！提示：按Ctrl+Alt+F1进入TTY1终端\n");
    return TRUE;  // TRUE=阻止，FALSE=允许关闭
}

void window_on_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

// 内部：页面加载状态回调
static void on_load_changed(WebKitWebView *web_view, WebKitLoadEvent event, gpointer data) {
    webview_on_load_changed(web_view, event, data);
}

GtkWidget* window_create_kiosk(void) {
    // 创建主窗口：全屏、无边框、无标题栏
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_fullscreen(GTK_WINDOW(window));
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    
    // 连接窗口生命周期信号
    g_signal_connect(window, "delete-event", 
                     G_CALLBACK(window_on_close_request), NULL);
    g_signal_connect(window, "destroy", 
                     G_CALLBACK(window_on_destroy), NULL);
    
    // 使用Overlay布局：WebView在下层，按钮在上层
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), overlay);
    
    // 创建并添加WebView（底层）（内部已配置JS桥接）
    WebKitWebView *webView = webview_create();

    // 关键：重新连接JS消息信号，传入web_view作为user_data
    // 这样js_bridge里的回调就能拿到web_view实例来回复消息
    WebKitUserContentManager *ucm = webkit_web_view_get_user_content_manager(webView);
    g_signal_connect(ucm, "script-message-received::native",
                     G_CALLBACK(on_js_message_received), webView);

    // 连接加载完成信号
    g_signal_connect(webView, "load-changed", G_CALLBACK(on_load_changed), NULL);

    // 连接新窗口拦截
    g_signal_connect(webView, "create", G_CALLBACK(webview_on_create_new_window), NULL);

    //webview_load_url(webView, "https://www.baidu.com");
    webview_load_url(webView, "file:///home/q/%E4%BC%9A%E8%AE%AE%E7%9B%92%E5%AD%90%E6%96%B0%E7%94%9F%E7%89%88/AI-MTB/test.html");
    gtk_container_add(GTK_CONTAINER(overlay), GTK_WIDGET(webView));
    
    // 创建并添加悬浮WiFi按钮（叠加层）
    //GtkWidget *wifi_button = ui_create_floating_wifi_button();
    //gtk_overlay_add_overlay(GTK_OVERLAY(overlay), wifi_button);
    
    // 定位到左下角（可改为START/END组合）
    //ui_position_floating_button(wifi_button, GTK_ALIGN_START, GTK_ALIGN_END);
    
    create_two_buttons(GTK_OVERLAY(overlay));

    // 连接WebView的新窗口信号
    //g_signal_connect(webView, "create", G_CALLBACK(webview_on_create_new_window), NULL);
 
    return window;
}
