Example_USB 已改造为 Bootloader USB-CDC 固件升级工程。

功能:
  1. 上电后检查 burn flag / 分区标志，有合法 APP 则跳转
  2. 否则以 CDC_ONLY 枚举 (VID=0x8888 PID=0x1722)，等待上位机升级协议

协议: 与 Banux/05_component/firmware_upgrade 相同 (SOF=0xAA)

入口文件: src/main.c
升级模块: src/firmware_upgrade/
USB CDC:  otg/device/src/otg_device_cdc.c

调试串口: A9/A10 UART1 115200
