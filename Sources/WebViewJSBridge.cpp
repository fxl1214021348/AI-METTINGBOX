#include "WebViewJSBridge.h"
#include <map>
#include <string>
#include <cstring>

// 内部数据结构（处理器信息）
struct HandlerInfo {
    std::string name;               //处理器名称
    MessageCallback callback;       //回调函数
    void *user_data;                //用户数据
};

struct BridgeContext {
    WebKitWebView *webView;                         //WebView实例
    WebKitUserContentManager *manager;              //内容管理器
    std::map<std::string, HandlerInfo> handlers;    //多个处理器
};

// 全局上下文映射
static std::map<WebKitWebView*, BridgeContext*> g_contexts;

// 修正：使用旧版API获取JS值
static void on_message_received(WebKitUserContentManager *manager,
                                 WebKitJavascriptResult *js_result,
                                 gpointer user_data) {
    // 获取WebView和上下文
    WebKitWebView *webView = WEBKIT_WEB_VIEW(user_data);
    
    auto it = g_contexts.find(webView);
    if (it == g_contexts.end()) {
        webkit_javascript_result_unref(js_result);
        return;
    }
    
    // 旧版本API：使用 get_value 和 get_global_context（从JS结果中提取消息内容）
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(js_result);
    JSValueRef value = webkit_javascript_result_get_value(js_result);
    
    // 转换为字符串
    JSStringRef js_string = JSValueToStringCopy(context, value, NULL);
    if (js_string) {
        size_t len = JSStringGetLength(js_string) * 4 + 1; // 预留足够空间
        char *message_str = (char*)g_malloc(len);
        
        // 获取UTF8字符串
        JSStringGetUTF8CString(js_string, message_str, len);
        
        g_print("[Bridge] 收到JS消息: %s\n", message_str);
        
        // 通知所有注册的处理器
        for (auto &pair : it->second->handlers) {
            if (pair.second.callback) {
                pair.second.callback(message_str, pair.second.user_data);
            }
        }
        
        g_free(message_str);
        JSStringRelease(js_string);
    }
    
    webkit_javascript_result_unref(js_result);
}

static void on_js_executed(GObject *object, GAsyncResult *result, gpointer user_data) {
    WebKitWebView *webView = WEBKIT_WEB_VIEW(object);
    GError *error = NULL;
    
    WebKitJavascriptResult *js_result = webkit_web_view_run_javascript_finish(webView, result, &error);
    
    if (error) {
        g_print("[Bridge] JS执行失败: %s\n", error->message);
        g_error_free(error);
    }
    
    if (js_result) {
        webkit_javascript_result_unref(js_result);
    }
}

// 公开API实现
void webview_bridge_init(WebKitWebView *webView) {
    if (!webView) return;
    
    // 直接从WebView获取UserContentManager
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(webView);
    
    BridgeContext *ctx = new BridgeContext();
    ctx->webView = webView;
    ctx->manager = manager;
    
    g_contexts[webView] = ctx;
    
    g_print("[Bridge] 通信模块初始化完成\n");
}

void webview_bridge_register(WebKitWebView *webView,
                             const char *handlerName,
                             MessageCallback callback,
                             void *user_data) {
    if (!webView || !handlerName || !callback) return;
    
    auto it = g_contexts.find(webView);
    if (it == g_contexts.end()) {
        g_print("[Bridge] 错误: WebView未初始化\n");
        return;
    }
    
    BridgeContext *ctx = it->second;
    
    // 检查是否已存在
    if (ctx->handlers.find(handlerName) != ctx->handlers.end()) {
        g_print("[Bridge] 处理器 %s 已存在\n", handlerName);
        return;
    }
    
    // 旧版本：register_script_message_handler 只有2个参数（在WebKit中注册消息处理器）
    webkit_user_content_manager_register_script_message_handler(ctx->manager, handlerName);
    
    // 构建信号名并连接
    std::string signal_name = "script-message-received::" + std::string(handlerName);
    g_signal_connect(ctx->manager,
                     signal_name.c_str(),
                     G_CALLBACK(on_message_received),
                     webView);
    
    // 保存处理器信息
    HandlerInfo info;
    info.name = handlerName;
    info.callback = callback;
    info.user_data = user_data;
    ctx->handlers[handlerName] = info;
    
    g_print("[Bridge] 注册处理器: %s\n", handlerName);
}

void webview_bridge_execute(WebKitWebView *webView, const char *script) {
    if (!webView || !script) return;
    
    g_print("[Bridge] 执行JS: %s\n", script);
    webkit_web_view_run_javascript(webView, script, NULL, on_js_executed, NULL);
}

// 发送消息到JS（Web端）
void webview_bridge_send(WebKitWebView *webView, const char *message) {
    if (!webView || !message) return;
    
    // 转义单引号
    GString *escaped = g_string_new("");
    for (const char *p = message; *p; p++) {
        if (*p == '\'') {
            g_string_append(escaped, "\\'");
        } else {
            g_string_append_c(escaped, *p);
        }
    }
    
    char *script = g_strdup_printf("if (window.handleMessageFromCPP) window.handleMessageFromCPP('%s');", escaped->str);
    
    webview_bridge_execute(webView, script);
    
    g_string_free(escaped, TRUE);
    g_free(script);
}

void webview_bridge_cleanup(WebKitWebView *webView) {
    auto it = g_contexts.find(webView);
    if (it != g_contexts.end()) {
        delete it->second;
        g_contexts.erase(it);
        g_print("[Bridge] 通信模块清理完成\n");
    }
}