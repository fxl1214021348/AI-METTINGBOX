#include "ExitSettingButton.h"
#include "DeviceStatusDialog.h"
#include "WifiControl.h"
#include <gtk/gtk.h>
#include <cmath>

// ========== 直接定义结构体（原 LayeredButton.h 里的内容）==========
typedef struct {
    GtkAlign halign;      // 水平对齐：START(左), END(右), CENTER(中)
    GtkAlign valign;      // 垂直对齐
    int margin_top;
    int margin_bottom;
    int margin_start;     // 左边距
    int margin_end;       // 右边距
} ButtonPosition;

// ========== 辅助函数：绘制圆角矩形 ==========
static void draw_rounded_rect(cairo_t *cr, int w, int h, double r) {
    cairo_arc(cr, r, r, r, M_PI, 1.5 * M_PI);
    cairo_arc(cr, w - r, r, r, 1.5 * M_PI, 2 * M_PI);
    cairo_arc(cr, w - r, h - r, r, 0, 0.5 * M_PI);
    cairo_arc(cr, r, h - r, r, 0.5 * M_PI, M_PI);
    cairo_close_path(cr);
}

// ========== 直接实现创建按钮函数（原 LayeredButton.cpp 里的内容）==========
static GtkWidget* create_layered_button(const char *icon_path,
                                        const char *bg_color,
                                        ButtonPosition *pos) {
    // 1. 固定布局容器
    GtkWidget *fixed = gtk_fixed_new();
    gtk_widget_set_size_request(fixed, 80, 80);

    // 2. 下层：纯色背景
    GtkWidget *bg = gtk_drawing_area_new();
    gtk_widget_set_size_request(bg, 80, 80);
    
    // 存储颜色
    // char *color_copy = g_strdup(bg_color ? bg_color : "#cccccc");
    // g_object_set_data_full(G_OBJECT(bg), "bg_color", color_copy, g_free);
    
    //直接硬编码目标颜色，不再从参数读取
    g_object_set_data(G_OBJECT(bg), "use_custom_color", GINT_TO_POINTER(1));
    
    // 绘制信号
    g_signal_connect(bg, "draw",
        G_CALLBACK(+[](GtkWidget *w, cairo_t *cr, gpointer d) {
            // const char *c = (const char*)g_object_get_data(G_OBJECT(w), "bg_color");
            // GdkRGBA color;
            // gdk_rgba_parse(&color, c ? c : "#cccccc");
            
            //使用 rgba(26,26,26,0.10)
            cairo_set_source_rgba(cr, 26.0/255.0, 26.0/255.0, 26.0/255.0, 0.04);

            int width = gtk_widget_get_allocated_width(w);
            int height = gtk_widget_get_allocated_height(w);
            
            //draw_rounded_rect(cr, width, height, 20.0);  // 20px圆角

            //border-radius: 40px (正圆形，80/2=40)
            draw_rounded_rect(cr, width, height, 40.0);
            //gdk_cairo_set_source_rgba(cr, &color);
            cairo_fill(cr);
            return FALSE;
        }), NULL);
    
    gtk_fixed_put(GTK_FIXED(fixed), bg, 0, 0);

    // 3. 上层：PNG图标（居中 16=(80-48)/2）
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(icon_path, 48, 48, NULL);
    if (!pixbuf) {
        g_warning("加载图标失败: %s", icon_path);
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 48, 48);
        gdk_pixbuf_fill(pixbuf, 0xff0000ff);  // 红色占位
    }
    GtkWidget *icon = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    gtk_fixed_put(GTK_FIXED(fixed), icon, 16, 16);

    // 4. 包装成按钮
    GtkWidget *button = gtk_button_new();
    gtk_widget_set_size_request(button, 80, 80);
    gtk_container_add(GTK_CONTAINER(button), fixed);
    
    // 去除默认样式
    GtkStyleContext *ctx = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(ctx, "flat");
    // 移除焦点框（方框边框的主要来源）
    gtk_widget_set_can_focus(button, FALSE);
    
    // 应用位置
    gtk_widget_set_halign(button, pos->halign);
    gtk_widget_set_valign(button, pos->valign);
    gtk_widget_set_margin_top(button, pos->margin_top);
    gtk_widget_set_margin_bottom(button, pos->margin_bottom);
    gtk_widget_set_margin_start(button, pos->margin_start);
    gtk_widget_set_margin_end(button, pos->margin_end);
    
    // 初始半透明
    //gtk_widget_set_opacity(button, 0.4);

    return button;
}


// 点击回调
static void on_exit_clicked(GtkWidget *w, gpointer d) {
    g_print("退出应用\n");
    gtk_main_quit();
}

static void on_settings_clicked(GtkWidget *w, gpointer d) {
    g_print("打开设置界面\n");
    //wifi_control_open_settings();  // 委托给系统控制模块
    GtkWidget *main_window = GTK_WIDGET(d);
    device_status_dialog_show(main_window);
}

void create_two_buttons(GtkOverlay *overlay) {
    // 按钮1：退出 - 左上角
    ButtonPosition pos1 = {GTK_ALIGN_START, GTK_ALIGN_START, 54, 0, 60, 0};
    GtkWidget *btn1 = create_layered_button(
        "Assets/images/exit.png", "#cccccc", &pos1
    );
    g_signal_connect(btn1, "clicked", G_CALLBACK(on_exit_clicked), NULL);
    gtk_overlay_add_overlay(overlay, btn1);

    // 按钮2：设置 - 左上角
    ButtonPosition pos2 = {GTK_ALIGN_START, GTK_ALIGN_START, 54, 0, 164, 0};
    GtkWidget *btn2 = create_layered_button(
        "Assets/images/setting.png", "#cccccc", &pos2
    );
    g_signal_connect(btn2, "clicked", G_CALLBACK(on_settings_clicked), NULL);
    gtk_overlay_add_overlay(overlay, btn2);
}