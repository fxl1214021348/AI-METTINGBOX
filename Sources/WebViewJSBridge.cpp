#include "WebViewJSBridge.h"
#include <iostream>

// 内部：JS调用C++时的回调处理
static void on_js_message_received(WebKitUserContentManager *manager,
                                   WebKitJavascriptResult *js_result,
                                   gpointer user_data) {
    // 解析参数
    gchar *message = js_bridge_get_message_string(js_result);
    if (message) {
        std::cout << "[C++收到JS消息] " << message << std::endl;
        
        // TODO: 在这里根据message内容分发处理
        // 比如解析JSON，调用不同功能（WiFi控制、音频等）
        
        // 回复网页
        WebKitWebView *web_view = WEBKIT_WEB_VIEW(user_data);
        js_bridge_send_to_web(web_view, "C++已收到: ");
        
        g_free(message);
    }
}

WebKitUserContentManager* js_bridge_init(void) {
    WebKitUserContentManager *ucm = webkit_user_content_manager_new();
    
    // 注册 "native" 消息通道
    // JS调用方式: window.webkit.messageHandlers.native.postMessage(data)
    webkit_user_content_manager_register_script_message_handler(ucm, "native");
    
    // 注意：user_data先传NULL，在webview_handler里连接时会传入web_view
    g_signal_connect(ucm, "script-message-received::native",
                     G_CALLBACK(on_js_message_received), NULL);
    
    // 注入初始化脚本，让网页知道怎么调用C++
    const gchar *init_script = R"(
        window.KShareAPI = {
            // JS调用C++的接口
            callCpp: function(data) {
                if (window.webkit && window.webkit.messageHandlers.native) {
                    window.webkit.messageHandlers.native.postMessage(data);
                } else {
                    console.error('KShareAPI: native bridge not ready');
                }
            },
            // 注册C++回调的接口（网页设置这个函数接收C++消息）
            onCppMessage: null
        };
        
        // C++主动发消息时会调用这个全局函数
        window.onMessageFromCpp = function(msg) {
            if (window.KShareAPI.onCppMessage) {
                window.KShareAPI.onCppMessage(msg);
            }
            console.log('[来自C++]', msg);
        };
        
        console.log('[KShare] C++桥接已就绪');
    )";
    
    WebKitUserScript *script = webkit_user_script_new(
        init_script,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL, NULL
    );
    webkit_user_content_manager_add_script(ucm, script);
    
    return ucm;
}

void js_bridge_send_to_web(WebKitWebView *web_view, const gchar *message) {
    // 转义单引号防止JS注入
    gchar *escaped = g_strescape(message, "'");
    gchar *script = g_strdup_printf(
        "if(window.onMessageFromCpp) window.onMessageFromCpp('%s');",
        escaped
    );
    
    webkit_web_view_run_javascript(web_view, script, NULL, NULL, NULL);
    
    g_free(script);
    g_free(escaped);
}

gchar* js_bridge_get_message_string(WebKitJavascriptResult *js_result) {
    JSCValue *value = webkit_javascript_result_get_js_value(js_result);
    
    if (jsc_value_is_string(value)) {
        return jsc_value_to_string(value);
    }
    // 也可以支持JSON对象，这里简化只处理字符串
    return NULL;
}
