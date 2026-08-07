################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c \
../src/usb_audio_stub.c 

OBJS += \
./src/main.o \
./src/usb_audio_stub.o 

C_DEPS += \
./src/main.d \
./src/usb_audio_stub.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -I"/cygdrive/D/BanAirBundy/bootloader/src" -I"/cygdrive/D/BanAirBundy/bootloader/src/firmware_upgrade" -I"/cygdrive/D/BanAirBundy/bootloader/otg/device/inc" -I"/cygdrive/D/BanAirBundy/bootloader/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/bootloader/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/bootloader/middleware/mv_utils/inc" -I"/cygdrive/D/BanAirBundy/bootloader/middleware/audio/inc" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


