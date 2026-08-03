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
	$(CROSS_COMPILE)gcc -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/src" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/include" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/portable" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/USB/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/audio/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/system_config" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio/otg/device/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/flashboot" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

FreeRTOS/Source/portable/%.o: ../FreeRTOS/Source/portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/src" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/include" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/portable" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/USB/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/audio/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/system_config" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio/otg/device/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/flashboot" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


