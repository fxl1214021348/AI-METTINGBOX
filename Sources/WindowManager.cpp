#include "WindowManager.h"
#include "WebView.h"
#include "WifiHoverButton.h"

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
    webview_load_url(webView, "https://www.baidu.com");
    gtk_container_add(GTK_CONTAINER(overlay), GTK_WIDGET(webView));
    
    // 创建并添加悬浮WiFi按钮（叠加层）
    GtkWidget *wifi_button = ui_create_floating_wifi_button();
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), wifi_button);
    
    // 定位到左下角（可改为START/END组合）
    ui_position_floating_button(wifi_button, GTK_ALIGN_START, GTK_ALIGN_END);
    
    // 连接WebView的新窗口信号
    g_signal_connect(webView, "create", 
                     G_CALLBACK(webview_on_create_new_window), NULL);
    
    return window;
}
