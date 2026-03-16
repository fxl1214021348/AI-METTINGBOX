#include "WindowManager.h"
#include "WebView.h"
#include "ExitSettingButton.h"
#include <iostream> 

gboolean window_on_close_request(GtkWidget *widget, GdkEvent *event, gpointer data) {
    g_print("阻止窗口关闭！提示：按Ctrl+Alt+F1进入TTY1终端\n");
    return TRUE;  // TRUE=阻止，FALSE=允许关闭
}

void window_on_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
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
    
    // 创建并添加WebView（底层）
    WebKitWebView *webView = webview_create();

    // 连接WebView的新窗口信号
    g_signal_connect(webView, "create", G_CALLBACK(webview_on_create_new_window), NULL);

    //webview_load_url(webView, "https://www.baidu.com");
    webview_load_url(webView, "file:///home/q/%E4%BC%9A%E8%AE%AE%E7%9B%92%E5%AD%90%E6%96%B0%E7%94%9F%E7%89%88/AI-MTB/test.html");
    gtk_container_add(GTK_CONTAINER(overlay), GTK_WIDGET(webView));
    
    create_two_buttons(GTK_OVERLAY(overlay));
 
    return window;
}
