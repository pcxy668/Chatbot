# Chatbot聊天机器人项目

### 1.目录结构 
* main/Com 公共层             
* main/Int 接口层
* main/main.c 主程序入口
### 2.硬件组成
* 主控芯片 ESP32-S3-WROOM-1-N16R8
* 音频编解码模块 ES8311+NS4150B+扬声器+数字麦克风 
* 显示屏 2.0寸ST7789VW驱动IPS屏（SPI通信）
### 3.项目说明
* 本项目参考虾哥同款开源项目，通信协议、消息格式等信息详见其文档：https://github.com/78/xiaozhi-esp32.git 
* WIFI配网采用BLE方式，具体实现同Smart-Bell项目
* 按键模块：采用ADC按键，长按键2可重置WIFI配网信息，短按键2可在激活Chatbot后重启系统
* 屏幕交互显示采用LVGL，页面为自由布局，分标题、表情、文本三个标签
* 其他ESP32组件：
  * SR 语音识别及人声检测
  * codec_dev ES8311编解码器驱动
  * audio_codec 音频编解码器（用于OPUS与PCM格式转化）
  * websocket_client 与服务器建立websocket连接
* 项目架构如下：
<img width="1543" height="438" alt="image" src="https://github.com/user-attachments/assets/ca81051e-2b24-41dc-8d10-e68b8090ed3b" />
