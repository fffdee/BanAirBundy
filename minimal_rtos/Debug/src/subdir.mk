################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c 

OBJS += \
./src/main.o 

C_DEPS += \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/system_config" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/FreeRTOS/Source/include" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/startup" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/01_hal_drivers" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/01_hal_drivers/adc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/01_hal_drivers/gpio" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/01_hal_drivers/spi" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/01_hal_drivers/sdio" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/01_vfs" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/02_device_drivers/flash" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/03_driver_framework" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/03_driver_framework/core" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/03_driver_framework/drivers" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/03_driver_framework/event" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/04_shell_commands" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/05_component" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/05_component/firmware_upgrade" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/05_component/fat32" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/05_component/sys_param" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/05_component/sys_state" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/minimal_rtos/banux/05_component/sys_led" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


