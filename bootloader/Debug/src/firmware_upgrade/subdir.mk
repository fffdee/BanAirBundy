################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/firmware_upgrade/app_upgrade.c \
../src/firmware_upgrade/boot_decision.c \
../src/firmware_upgrade/cdc_upgrade.c 

OBJS += \
./src/firmware_upgrade/app_upgrade.o \
./src/firmware_upgrade/boot_decision.o \
./src/firmware_upgrade/cdc_upgrade.o 

C_DEPS += \
./src/firmware_upgrade/app_upgrade.d \
./src/firmware_upgrade/boot_decision.d \
./src/firmware_upgrade/cdc_upgrade.d 


# Each subdirectory must supply rules for building sources it contributes
src/firmware_upgrade/%.o: ../src/firmware_upgrade/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/bootloader/src" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/bootloader/src/firmware_upgrade" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/bootloader/otg/device/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/bootloader/driver/driver/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/bootloader/driver/driver_api/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/bootloader/middleware/mv_utils/inc" -I"/cygdrive/E/project_and_dataset/project/BanAirBundy/bootloader/middleware/audio/inc" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


