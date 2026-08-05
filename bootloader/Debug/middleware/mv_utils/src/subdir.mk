################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../middleware/mv_utils/src/mcu_circular_buf.c 

OBJS += \
./middleware/mv_utils/src/mcu_circular_buf.o 

C_DEPS += \
./middleware/mv_utils/src/mcu_circular_buf.d 


# Each subdirectory must supply rules for building sources it contributes
middleware/mv_utils/src/%.o: ../middleware/mv_utils/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -I"/cygdrive/D/BanAirBundy/bootloader/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/bootloader/middleware/audio/inc" -I"/cygdrive/D/BanAirBundy/bootloader/otg/host/inc" -I"/cygdrive/D/BanAirBundy/bootloader/src" -I"/cygdrive/D/BanAirBundy/bootloader/otg/device/inc" -I"/cygdrive/D/BanAirBundy/bootloader/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/bootloader/middleware/mv_utils/inc" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


