#include <gtk/gtk.h>
#include "WindowManager.h"

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // 创建并显示kiosk窗口（内部已组装所有组件）
    GtkWidget *window = window_create_kiosk();
    gtk_widget_show_all(window);
    
    // 进入GTK事件循环
    gtk_main();
    
    return 0;
}
