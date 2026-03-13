#include "Styles.h"

const char* DIALOG_CSS = R"(
/* 全屏背景 */
.fullscreen-bg {
    background-color: #f5f5f5;
}

/* 标题栏容器 */
.title-bar-container {
    background-color: transparent;
    border-bottom: 1px solid #f0f0f0;
    min-height: 80px;
    padding: 0;
    border: 1px solid red;
}

/* 标题 */
.dialog-title {
    font-size: 40px;
    font-weight: bold;
    color: #1A1A1A;
}

/* 关闭按钮 */
.close-btn {
    background-color: transparent;
    border-radius: 20px;
    min-width: 40px;
    min-height: 40px;
    border: none;
}

.close-btn:hover {
    background-color: rgba(26, 26, 26, 0.15);
}

/* 水平卡片容器 */
.horizontal-cards-container {
    background-color: transparent;
    spacing: 80px;
}

/* 信息卡片 */
.info-card {
    background-color: white;
    border: 1px solid #e8e8e8;
    border-radius: 12px;
    min-width: 720px;
    min-height: 440px;
    padding: 0;
}

/* 卡片标题栏 */
.card-title-bar {
    border-bottom: 1px solid #f0f0f0;
    margin: 60px 52px 32px 60px;
    padding: 0;
}

.card-title-text {
    font-size: 36px;
    font-weight: 600;
    color: #1a1a1a;
}

/* 设置链接 */
.settings-link {
    color: #267DFF;
    font-size: 30px;
    font-weight: 500;
    border: none;
    background: transparent;
}

.settings-link:hover {
    text-decoration: underline;
}

/*创建外部容器存放IP显示框和刷新按钮样版*/
.ip-external-container {
    min-width: 600px;
    min-height: 80px;
    margin: 24px 60px 60px 60px;
    border: 1px solid red;
}

/* IP显示框 */
.ip-display {
    min-width: 520px;
    min-height: 80px;
    background-color: #f5f5f5;
    border-radius: 16px;
    margin: 0px 40px 0px 0px;
}

.ip-value {
    font-family: monospace;
    font-size: 30px;
    color: #333333;
    font-weight: 500;
    margin-left: 32px;
}

/* 刷新按钮 */
.refresh-btn {
    background: transparent;
    border: none;
    min-width: 40px;
    min-height: 44px;
}

/* IP和设备热点 */
.hotspot-ipwifi {
    margin: 40px 52px 24px 60px;
}

/* 热点内容 */
.hotspot-content {
    margin: 24px 52px 24px 60px;
}

.hotspot-desc {
    font-size: 30px;
    color: rgba(26, 26, 26, 0.6); 
    margin-bottom: 10px;
    border: 1px solid red;
}

.hotspot-name {
    font-size: 20px;
    color: #1a1a1a;
    font-weight: 600;
}

/* 开关按钮 */
.toggle-switch {
    background: transparent;
    border: none;
    min-width: 80px;
    min-height: 40px;
}

/* WiFi图标按钮样式 */
.wifi-icon-btn {
    background: transparent;
    border: none;
    padding: 0;
    border-radius: 8px;
}

/* 暂无数据 - 红色 */
.ip-value.no-data {
    font-size: 30px;
    color: #FA372D;  /* 红色 */
    font-weight: 400;
}

/* 获取中 - 蓝色或灰色 */
.ip-value.getting {
    color: #1A1A1A;  /* 黑色 */
    font-weight: 400;
    opacity: 0.35;
}
)";