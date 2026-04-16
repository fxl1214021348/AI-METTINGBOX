#include <ResourcePath.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>

const std::string& getResourcePath() {
    static std::string path;
    static bool inited = false;

    if (!inited) {
        char exe[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (len > 0) {
            exe[len] = '\0';
            std::string dir(exe);
            dir = dir.substr(0, dir.rfind('/'));

            struct stat st;
            // 安装环境: /opt/apps/meetingbox/bin/ → ../resources
            std::string installed = dir + "/../resources";
            if (stat(installed.c_str(), &st) == 0) {
                path = installed;
            }
            // 开发环境: 项目目录 → ./Assets
            else {
                std::string dev = dir + "/Assets";
                if (stat(dev.c_str(), &st) == 0) {
                    path = dev;
                } else {
                    path = dir;
                }
            }
        }
        inited = true;
    }
    return path;
}

std::string resFile(const std::string& relativePath) {
    return getResourcePath() + "/" + relativePath;
}


