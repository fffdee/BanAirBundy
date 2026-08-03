################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c \
../src/otg_device_cdc.c \
../src/otg_device_standard_request.c \
../src/otg_fifo.c \
../src/uarts_interface.c \
../src/upgrade.c \
../src/usb_audio_stub.c 

OBJS += \
./src/main.o \
./src/otg_device_cdc.o \
./src/otg_device_standard_request.o \
./src/otg_fifo.o \
./src/uarts_interface.o \
./src/upgrade.o \
./src/usb_audio_stub.o 

C_DEPS += \
./src/main.d \
./src/otg_device_cdc.d \
./src/otg_device_standard_request.d \
./src/otg_fifo.d \
./src/uarts_interface.d \
./src/upgrade.d \
./src/usb_audio_stub.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/src" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/include" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_bootloader/FreeRTOS/Source/portable" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/banux/02_device_drivers/USB/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/audio/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/system_config" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio/otg/device/inc" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/audio" -I"/cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_rx_sdk/flashboot" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


