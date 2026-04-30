# 编译器配置
CXX = g++
CFLAGS = -Wall -g -I./Headers `pkg-config --cflags gtk+-3.0 webkit2gtk-4.0 json-glib-1.0` -pthread
LIBS = `pkg-config --libs gtk+-3.0 webkit2gtk-4.0 json-glib-1.0` -pthread

# 源文件列表
SRCS = Sources/main.cpp \
       Sources/WifiControl.cpp \
       Sources/WebView.cpp \
       Sources/WindowManager.cpp \
       Sources/ExitSettingButton.cpp \
       Sources/DeviceStatusDialog.cpp \
       Sources/Styles.cpp \
       Sources/HotspotUtils.cpp \
       Sources/WebViewJSBridge.cpp \
       Sources/UARTProtocol.cpp \
       Sources/SerialMonitor.cpp \
       Sources/ResourcePath.cpp \
       Sources/DeviceState.cpp

# 目标文件（.cpp替换为.o）
OBJS = $(SRCS:.cpp=.o)

TARGET = AI-BOX

# 默认目标
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

# 编译规则：sources/%.cpp → sources/%.o
# -I./headers 让 #include "xxx.h" 能找到headers目录
Sources/%.o: Sources/%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

# 清理
clean:
	rm -f Sources/*.o $(TARGET)

# 运行测试
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
