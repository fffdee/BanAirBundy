################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/Source/portable/port.c \
../FreeRTOS/Source/portable/portISR.c 

S_UPPER_SRCS += \
../FreeRTOS/Source/portable/os_cpu_a.S 

OBJS += \
./FreeRTOS/Source/portable/os_cpu_a.o \
./FreeRTOS/Source/portable/port.o \
./FreeRTOS/Source/portable/portISR.o 

C_DEPS += \
./FreeRTOS/Source/portable/port.d \
./FreeRTOS/Source/portable/portISR.d 

S_UPPER_DEPS += \
./FreeRTOS/Source/portable/os_cpu_a.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/Source/portable/%.o: ../FreeRTOS/Source/portable/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -DBOOT_APP_WIRELESS_EN=1 -DBOOT_APP_WIRELESS_ROLE_TX=0 -I"/cygdrive/D/BanAirBundy/boot_app" -I"/cygdrive/D/BanAirBundy/boot_app/system_config" -I"/cygdrive/D/BanAirBundy/boot_app/src" -I"/cygdrive/D/BanAirBundy/wireless_lib" -I"/cygdrive/D/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/D/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/D/BanAirBundy/boot_app/startup" -I"/cygdrive/D/BanAirBundy/boot_app/banux" -I"/cygdrive/D/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/adc" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/gpio" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/spi" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/sdio" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_vfs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_device_drivers/flash" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/event" -I"/cygdrive/D/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/cdc_debug" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/sys" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/firmware_upgrade" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/fat32" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

FreeRTOS/Source/portable/%.o: ../FreeRTOS/Source/portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -DBOOT_APP_WIRELESS_EN=1 -DBOOT_APP_WIRELESS_ROLE_TX=0 -I"/cygdrive/D/BanAirBundy/boot_app" -I"/cygdrive/D/BanAirBundy/boot_app/system_config" -I"/cygdrive/D/BanAirBundy/boot_app/src" -I"/cygdrive/D/BanAirBundy/wireless_lib" -I"/cygdrive/D/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/D/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/D/BanAirBundy/boot_app/startup" -I"/cygdrive/D/BanAirBundy/boot_app/banux" -I"/cygdrive/D/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/adc" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/gpio" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/spi" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_hal_drivers/sdio" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_vfs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_device_drivers/flash" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_driver_framework/event" -I"/cygdrive/D/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/cdc_debug" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/sys" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/firmware_upgrade" -I"/cygdrive/D/BanAirBundy/boot_app/banux/05_component/fat32" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


