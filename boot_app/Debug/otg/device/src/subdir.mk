################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../otg/device/src/otg_device_audio.c \
../otg/device/src/otg_device_cdc.c \
../otg/device/src/otg_device_standard_request.c 

OBJS += \
./otg/device/src/otg_device_audio.o \
./otg/device/src/otg_device_cdc.o \
./otg/device/src/otg_device_standard_request.o 

C_DEPS += \
./otg/device/src/otg_device_audio.d \
./otg/device/src/otg_device_cdc.d \
./otg/device/src/otg_device_standard_request.d 


# Each subdirectory must supply rules for building sources it contributes
otg/device/src/%.o: ../otg/device/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/system_config" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/src" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/startup" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/01_hal_drivers" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/01_hal_drivers/adc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/01_hal_drivers/gpio" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/01_hal_drivers/spi" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/01_hal_drivers/sdio" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/01_vfs" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/02_device_drivers/flash" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/03_driver_framework" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/03_driver_framework/core" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/03_driver_framework/drivers" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/03_driver_framework/event" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/04_shell_commands" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/05_component" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/05_component/firmware_upgrade" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/05_component/fat32" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/05_component/sys_param" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/05_component/sys_state" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/boot_app/banux/05_component/sys_led" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


