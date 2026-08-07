################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../banux/03_driver_framework/drv_init.c 

OBJS += \
./banux/03_driver_framework/drv_init.o 

C_DEPS += \
./banux/03_driver_framework/drv_init.d 


# Each subdirectory must supply rules for building sources it contributes
banux/03_driver_framework/%.o: ../banux/03_driver_framework/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/D/BanAirBundy/boot_app" -I"/cygdrive/D/BanAirBundy/boot_app/system_config" -I"/cygdrive/D/BanAirBundy/boot_app/src" -I"/cygdrive/D/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/D/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/D/BanAirBundy/boot_app/startup" -I"/cygdrive/D/BanAirBundy/boot_app/banux" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/adc" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/gpio" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/spi" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/sdio" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_vfs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_device_drivers/flash" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/event" -I"/cygdrive/D/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/firmware_upgrade" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/fat32" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/sys_param" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/sys_state" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/sys_led" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


