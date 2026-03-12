// #include "WifiHoverButton.h"
// #include "WifiControl.h"

// // 按钮点击回调（内部静态，封装实现细节）
// static void on_wifi_button_clicked(GtkWidget *widget, gpointer data) {
//     wifi_control_open_settings();  // 委托给系统控制模块
// }

// GtkWidget* ui_create_floating_wifi_button(void) {
//     // 创建带WiFi图标的按钮
//     GtkWidget *button = gtk_button_new_from_icon_name(
//         "network-wireless-symbolic",
//         GTK_ICON_SIZE_DIALOG
//     );

//     // 应用CSS样式类：圆形+OSD半透明效果
//     GtkStyleContext *context = gtk_widget_get_style_context(button);
//     gtk_style_context_add_class(context, "circular");
//     gtk_style_context_add_class(context, "osd");

//     // 设置固定大小和边距
//     gtk_widget_set_size_request(button, 64, 64);
//     gtk_widget_set_margin_start(button, 20);
//     gtk_widget_set_margin_end(button, 20);
//     gtk_widget_set_margin_top(button, 20);
//     gtk_widget_set_margin_bottom(button, 20);

//     // 连接信号：点击→打开WiFi设置
//     g_signal_connect(button, "clicked", G_CALLBACK(on_wifi_button_clicked), NULL);

//     // 初始半透明（kiosk模式减少视觉干扰）
//     gtk_widget_set_opacity(button, 0.3);

//     return button;
// }

// void ui_position_floating_button(GtkWidget *button,
//                                   GtkAlign align_x,
//                                   GtkAlign align_y) {
//     gtk_widget_set_halign(button, align_x);
//     gtk_widget_set_valign(button, align_y);
// }
