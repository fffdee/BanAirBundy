################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../otg/host/src/otg_host_standard_enum.c \
../otg/host/src/otg_host_udisk.c 

OBJS += \
./otg/host/src/otg_host_standard_enum.o \
./otg/host/src/otg_host_udisk.o 

C_DEPS += \
./otg/host/src/otg_host_standard_enum.d \
./otg/host/src/otg_host_udisk.d 


# Each subdirectory must supply rules for building sources it contributes
otg/host/src/%.o: ../otg/host/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -I"/cygdrive/D/BanAirBundy/bootloader/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/bootloader/middleware/audio/inc" -I"/cygdrive/D/BanAirBundy/bootloader/otg/host/inc" -I"/cygdrive/D/BanAirBundy/bootloader/src" -I"/cygdrive/D/BanAirBundy/bootloader/otg/device/inc" -I"/cygdrive/D/BanAirBundy/bootloader/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/bootloader/middleware/mv_utils/inc" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


