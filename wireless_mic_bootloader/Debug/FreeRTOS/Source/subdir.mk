################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/Source/event_groups.c \
../FreeRTOS/Source/heap_5s.c \
../FreeRTOS/Source/list.c \
../FreeRTOS/Source/queue.c \
../FreeRTOS/Source/tasks.c \
../FreeRTOS/Source/timers.c 

OBJS += \
./FreeRTOS/Source/event_groups.o \
./FreeRTOS/Source/heap_5s.o \
./FreeRTOS/Source/list.o \
./FreeRTOS/Source/queue.o \
./FreeRTOS/Source/tasks.o \
./FreeRTOS/Source/timers.o 

C_DEPS += \
./FreeRTOS/Source/event_groups.d \
./FreeRTOS/Source/heap_5s.d \
./FreeRTOS/Source/list.d \
./FreeRTOS/Source/queue.d \
./FreeRTOS/Source/tasks.d \
./FreeRTOS/Source/timers.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/Source/%.o: ../FreeRTOS/Source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/src" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/include" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/portable" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/USB/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/audio/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/system_config" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio/otg/device/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/flashboot" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


