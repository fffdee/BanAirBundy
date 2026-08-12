AudioADC_Example工程主要演示了AudioADC的基本使用方法。

功能：
1.演示MIC输入，DAC输出过程；
2.演示LineIn1输入，DAC输出过程；
3.演示LineIn2输入，DAC输出过程。

硬件环境要求:
    1. BP15系列开发板；
    2. 使用BP15系列开发板时，外接串口小板，TX/RX/GND 接至 A10/A9/GND 引脚 (波特率 2000000)；
    3、DAC默认使用单端模式，BP1532AX系列DAC不支持单端模式，如果使用BP1532AX系列开发板DAC配置请选择差分模式。
    4、PGAGain设置为22是按照BP1564全功能demo板为平台来设置的(BP1564全功能demo板MIC端有放大器，放大20dB)。如果使用其他没有放大器的平台可以调大MIC PGAGain

软件使用说明:
    上电启动之后，可通过串口输入不同指令，进入3种例程：
    1. 输入‘m’或‘M’，进入MIC模式；
    2. 输入‘l’或‘L’，进入LineIn1模式；
    3. 输入‘x’或‘X’，进入LineIn2模式。