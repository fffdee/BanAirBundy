UART_Example工程主要演示了UART的基本使用方法。

功能：
1.MCU非中断方式进行串口收发。
2.MUC中断方式进行串口收发。
3.DMA Circular 中断方式发送数据。
4.DMA Circular 非中断方式发送数据。
5.DMA Circular 中断方式接收数据。
6.DMA Circular 非中断方式接收数据。
7.DMA Block 模式发送数据。


硬件环境要求:
    1. 使用BPxx系列开发板时，TX/RX/GND 接至配置好的引脚 (波特率默认2000000，可调整)；

软件使用说明:
    上电启动之后，会打印如下信息，根据提示信息输入对应的字母，便可进入对应的示例程序。
    ****************************************************************
	               UART Example MVSilicon  
	****************************************************************

	Select an example:
	0:: MCU with no interrupt
	1:: MCU with interrupt
	2:: DMA Circular send with interrupt
	3:: DMA Circular send with no interrupt
	4:: DMA Circular receive with interrupt
	5:: DMA Circular receive with no interrupt
	6:: DMA Block send