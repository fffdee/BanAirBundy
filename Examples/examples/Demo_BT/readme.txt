Demo BT 工程主要演示了蓝牙播放、蓝牙通话、sniff的工作流程，未开启 OS。

功能：
1、音乐播放，蓝牙stack 接收数据，解码器解码sbc为pcm数据送FIFO（DMA）dac播放。
2、通话时，蓝牙stack 接收数据，解码器解码msbc为pcm数据送FIFO（DMA）dac播放。
3、手机进入sniff时，系统进入低功耗状态。

硬件环境要求:
    1. BP15系列开发板；
    2. 使用BP15系列开发板时，外接串口小板，TX/RX/GND 接至 A9/A10/GND 引脚 (波特率 2000000)；
    3、DAC默认使用单端模式，BP1532AX系列DAC不支持单端模式，如果使用BP1532AX系列开发板DAC配置请选择差分模式。
    4、mic输入默认使用单端方式。
    5、手机搜索蓝牙名“BP15_BT_DEMO”，连接后播放音乐，拨打电话。

软件使用说明:
   无

注意：
1、download 必须使用  mv_download_bp1x_v3.2.exe 以及以后
2、IDE使用v300版本

 