#include "WifiControl.h"
#include <glib.h>
#include <stdio.h>

void wifi_control_open_settings(void) {
    g_print("异步打开系统WiFi设置...\n");
    
    GError *error = NULL;
    
    // 首选：GNOME控制中心
    GSubprocess *process = g_subprocess_new(
        G_SUBPROCESS_FLAGS_NONE,
        &error,
        "gnome-control-center",
        "network",
        NULL
    );
    
    if (process == NULL) {
        g_warning("启动gnome-control-center失败: %s", error->message);
        g_error_free(error);
        
        // 备选：nm-connection-editor
        error = NULL;
        process = g_subprocess_new(
            G_SUBPROCESS_FLAGS_NONE,
            &error,
            "nm-connection-editor",
            NULL
        );
        
        if (process == NULL) {
            g_warning("备选nm-connection-editor也失败: %s", error->message);
            g_error_free(error);
            return;
        }
    }
    
    g_print("WiFi设置已启动（异步）\n");
    g_object_unref(process);
}
