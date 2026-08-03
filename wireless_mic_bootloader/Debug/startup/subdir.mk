################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../startup/init-default.c \
../startup/interrupt.c \
../startup/retarget.c 

S_UPPER_SRCS += \
../startup/crt0.S 

OBJS += \
./startup/crt0.o \
./startup/init-default.o \
./startup/interrupt.o \
./startup/retarget.o 

C_DEPS += \
./startup/init-default.d \
./startup/interrupt.d \
./startup/retarget.d 

S_UPPER_DEPS += \
./startup/crt0.d 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/src" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/include" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/portable" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/USB/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/audio/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/system_config" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio/otg/device/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/flashboot" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

startup/%.o: ../startup/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/src" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/include" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/portable" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/USB/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/audio/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/system_config" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio/otg/device/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/flashboot" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


