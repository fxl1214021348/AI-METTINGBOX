#ifndef RESOURCE_PATH_H
#define RESOURCE_PATH_H

#include <string>

// 获取资源根目录（首次调用自动检测，之后返回缓存值）
const std::string& getResourcePath();

// 拼接资源文件完整路径的便捷函数
std::string resFile(const std::string& relativePath);

#endif