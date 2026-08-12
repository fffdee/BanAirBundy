PowerConsumption_Example工程主要演示deepsleep和sleep的基本使用方法供验证开发板功耗情况。

硬件环境要求:
    1. BP15系列开发板；
    2. 使用BP15系列开发板时，外接串口小板，TX/RX/GND 接至 A10/A9/GND 引脚 (波特率 2000000)；

软件使用说明:
	1. 通过串口可以看到相关信息；
	
注意事项：
	1、DeepSleep为功耗最低，支持DCDC的芯片会配置DCDC到1.3V,不支持的芯片则不要配置DCDC。这个配置宏USE_DCDC即可。
	2、UART打印端口，睡眠之后和唤醒之后需要保持一致。为了防止用户错误调用，封装成统一的一个API--UARTLogIOConfig()。