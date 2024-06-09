# 自动浇水器
### 编译说明
- 系统环境：Ubuntu 22.04
- 安装esp-idf：https://docs.espressif.com/projects/esp-idf/zh_CN/release-v5.1/esp32c3/get-started/linux-macos-setup.html
- 执行`./webcomp.sh`编译网页
- 执行`idf.py build`编译
- 执行`idf.py flash`烧录
### 功能说明
- 使用ESP32-C2模组
- 4路独立水泵控制
- 4路土壤湿度检测
- 缺水检测
- 可连接WIFI，通过手机网页控制
### 硬件开源地址
- https://oshwhub.com/qwiaoei/esp_watering
