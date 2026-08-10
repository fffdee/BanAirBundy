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
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/d/BanAirBundy/boot_app" -I"/cygdrive/d/BanAirBundy/boot_app/system_config" -I"/cygdrive/d/BanAirBundy/boot_app/src" -I"/cygdrive/d/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/d/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/d/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/d/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/d/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/d/BanAirBundy/boot_app/startup" -I"/cygdrive/d/BanAirBundy/boot_app/banux" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers/adc" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers/gpio" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers/spi" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers/sdio" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_vfs" -I"/cygdrive/d/BanAirBundy/boot_app/banux/02_device_drivers/flash" -I"/cygdrive/d/BanAirBundy/boot_app/banux/03_driver_framework" -I"/cygdrive/d/BanAirBundy/boot_app/banux/03_driver_framework/core" -I"/cygdrive/d/BanAirBundy/boot_app/banux/03_driver_framework/drivers" -I"/cygdrive/d/BanAirBundy/boot_app/banux/03_driver_framework/event" -I"/cygdrive/d/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/firmware_upgrade" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/fat32" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/sys_param" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/sys_state" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/sys_led" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

FreeRTOS/Source/portable/%.o: ../FreeRTOS/Source/portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/d/BanAirBundy/boot_app" -I"/cygdrive/d/BanAirBundy/boot_app/system_config" -I"/cygdrive/d/BanAirBundy/boot_app/src" -I"/cygdrive/d/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/d/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/d/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/d/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/d/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/d/BanAirBundy/boot_app/startup" -I"/cygdrive/d/BanAirBundy/boot_app/banux" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers/adc" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers/gpio" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers/spi" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_hal_drivers/sdio" -I"/cygdrive/d/BanAirBundy/boot_app/banux/01_vfs" -I"/cygdrive/d/BanAirBundy/boot_app/banux/02_device_drivers/flash" -I"/cygdrive/d/BanAirBundy/boot_app/banux/03_driver_framework" -I"/cygdrive/d/BanAirBundy/boot_app/banux/03_driver_framework/core" -I"/cygdrive/d/BanAirBundy/boot_app/banux/03_driver_framework/drivers" -I"/cygdrive/d/BanAirBundy/boot_app/banux/03_driver_framework/event" -I"/cygdrive/d/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/firmware_upgrade" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/fat32" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/sys_param" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/sys_state" -I"/cygdrive/d/BanAirBundy/boot_app/banux/05_component/sys_led" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


