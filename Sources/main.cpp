#include <gtk/gtk.h>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "WindowManager.h"
#include "SerialMonitor.h"

// 外部全局变量
extern SerialMonitor* g_serialMonitor;

/**
 * @brief 标记是否已经执行过清理，防止重复清理
 */
static volatile sig_atomic_t g_cleanupDone = 0;

/**
 * @brief 异常退出时的资源清理函数
 * 
 * 在信号处理器中只做最小限度的安全操作：
 * - 停止串口监测线程、关闭串口文件描述符
 * - 不调用 GTK/GLib 函数（它们不是 async-signal-safe 的）
 */
static void cleanup_resources() {
    if (g_cleanupDone) return;
    g_cleanupDone = 1;

    // 串口清理：stop() 内部会设置 running_=false 并 join 线程
    if (g_serialMonitor) {
        g_serialMonitor->stop();
        delete g_serialMonitor;
        g_serialMonitor = nullptr;
    }
}

/**
 * @brief POSIX 信号处理函数
 * @param sig 收到的信号编号
 * 
 * 捕获以下信号：
 *   SIGINT  (Ctrl+C)
 *   SIGTERM (kill 默认信号)
 *   SIGSEGV (段错误)
 *   SIGABRT (abort 调用)
 *   SIGPIPE (写入已关闭的管道/socket)
 */
static void signal_handler(int sig) {
    const char *name = "UNKNOWN";
    switch (sig) {
        case SIGINT:  name = "SIGINT";  break;
        case SIGTERM: name = "SIGTERM"; break;
        case SIGSEGV: name = "SIGSEGV"; break;
        case SIGABRT: name = "SIGABRT"; break;
        case SIGPIPE: name = "SIGPIPE"; break;
    }

    // write() 是 async-signal-safe 的，printf 不是
    const char prefix[] = "\n[异常退出] 收到信号: ";
    write(STDERR_FILENO, prefix, sizeof(prefix) - 1);
    write(STDERR_FILENO, name, strlen(name));
    write(STDERR_FILENO, "\n", 1);

    cleanup_resources();

    // 对于致命信号，恢复默认处理并重新发送，让系统生成 core dump
    if (sig == SIGSEGV || sig == SIGABRT) {
        signal(sig, SIG_DFL);
        raise(sig);
    }

    _exit(1);
}

/**
 * @brief 注册所有需要捕获的信号
 */
static void install_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    // SA_RESETHAND: 对 SIGSEGV/SIGABRT 只处理一次，避免死循环
    sa.sa_flags = 0;

    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGPIPE, &sa, nullptr);

    // 致命信号加 SA_RESETHAND，处理一次后恢复默认行为
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
}

int main(int argc, char *argv[]) {
    // 1. 注册信号处理器（在所有初始化之前）
    install_signal_handlers();

    // 2. 注册 atexit 回调，处理正常退出时的清理
    atexit(cleanup_resources);

    gtk_init(&argc, &argv);
    
    // 创建并显示kiosk窗口（内部已组装所有组件）
    GtkWidget *window = window_create_kiosk();
    gtk_widget_show_all(window);

    printf("✓ 信号处理器已安装 (SIGINT/SIGTERM/SIGSEGV/SIGABRT/SIGPIPE)\n");
    
    // 进入GTK事件循环
    gtk_main();
    
    return 0;
}
