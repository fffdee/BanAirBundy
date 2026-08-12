Example_PMU工程主要演示了NVM 和 powerkey的基本操作

1、硬件环境要求:
	BP15系列开发板，外接串口小板，TX/RX/GND 接至 RX(A9)/TX(A10)/GND (波特率 2000000)；

2、软件使用说明:
	NVM 示例，演示了首次上电和后面Powerdown/PowerUP 上电后NVM数据读取，修改，写入前后的变化情况
	
	PowerKey 示例
	1) 演示了通过PowerKey控制开发板进入/退出Powerdown状态的操作
	2) 演示了通过PowerKey长按复位，控制开发板硬件复位的操作(需要打开宏定义 DEMO_8S_RESET)。
	
	用户可以修改SystemPowerKeyInit()函数参数，来调整模式及按键检测时间。
	其中模式参数，主要有下面几种：
	E_PWRKEY_MODE_BYPASS:           PowerKey功能关闭
	E_PWRKEY_MODE_PUSHBUTTON:       针对Powerkey为按键类型应用，根据设定时长来进入/退出Powerdown状态，此模式有长按8s硬件复位的功能
	E_PWRKEY_MODE_SLIDESWITCH_HON:  针对Powerkey为拔动开关类型应用，开关拔到低电平进入Powerdown，高电平PowerUp，此模式下无长按8s硬件复位功能
	E_PWRKEY_MODE_SLIDESWITCH_LON:  针对Powerkey为拔动开关类型应用，开关拔到高电平进入Powerdown，低电平PowerUp，此模式下无长按8s硬件复位功能
	
3、试验注意事项：
   1) 程序改动更新后，开发板应断开所有供电(包括调试器供电，USB供电等)，重新上电后新的修改设置才能生效
   2) 开发板拔码开关设置：在BP15XX_DEVELOPMENT_V1.3开发板上，P2拔到ON位置时，Powerkey为低电平；P2拔到OFF位置时，Powerkey为高电平。
                          测试E_PWRKEY_MODE_PUSHBUTTON应用时，P2应拔到OFF位置；