################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../banux/02_device_drivers/flash/BG_FlashMgr.c \
../banux/02_device_drivers/flash/bg_flash_manager.c \
../banux/02_device_drivers/flash/flash_bus.c \
../banux/02_device_drivers/flash/flash_devices.c \
../banux/02_device_drivers/flash/flash_driver.c \
../banux/02_device_drivers/flash/flash_manager.c \
../banux/02_device_drivers/flash/flash_nand_w25n02.c \
../banux/02_device_drivers/flash/flash_nor_w25qxx.c \
../banux/02_device_drivers/flash/flash_test.c \
../banux/02_device_drivers/flash/psram_esp64h.c 

OBJS += \
./banux/02_device_drivers/flash/BG_FlashMgr.o \
./banux/02_device_drivers/flash/bg_flash_manager.o \
./banux/02_device_drivers/flash/flash_bus.o \
./banux/02_device_drivers/flash/flash_devices.o \
./banux/02_device_drivers/flash/flash_driver.o \
./banux/02_device_drivers/flash/flash_manager.o \
./banux/02_device_drivers/flash/flash_nand_w25n02.o \
./banux/02_device_drivers/flash/flash_nor_w25qxx.o \
./banux/02_device_drivers/flash/flash_test.o \
./banux/02_device_drivers/flash/psram_esp64h.o 

C_DEPS += \
./banux/02_device_drivers/flash/BG_FlashMgr.d \
./banux/02_device_drivers/flash/bg_flash_manager.d \
./banux/02_device_drivers/flash/flash_bus.d \
./banux/02_device_drivers/flash/flash_devices.d \
./banux/02_device_drivers/flash/flash_driver.d \
./banux/02_device_drivers/flash/flash_manager.d \
./banux/02_device_drivers/flash/flash_nand_w25n02.d \
./banux/02_device_drivers/flash/flash_nor_w25qxx.d \
./banux/02_device_drivers/flash/flash_test.d \
./banux/02_device_drivers/flash/psram_esp64h.d 


# Each subdirectory must supply rules for building sources it contributes
banux/02_device_drivers/flash/%.o: ../banux/02_device_drivers/flash/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/D/BanAirBundy/boot_app" -I"/cygdrive/D/BanAirBundy/boot_app/system_config" -I"/cygdrive/D/BanAirBundy/boot_app/src" -I"/cygdrive/D/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/D/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/D/BanAirBundy/boot_app/startup" -I"/cygdrive/D/BanAirBundy/boot_app/banux" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/adc" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/gpio" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/spi" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/sdio" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_vfs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_device_drivers/flash" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/event" -I"/cygdrive/D/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/firmware_upgrade" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/fat32" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/sys_param" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/sys_state" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/sys_led" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


