################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../startup/flash_boot.c \
../startup/init-default.c \
../startup/interrupt.c \
../startup/retarget.c 

S_UPPER_SRCS += \
../startup/crt0.S 

OBJS += \
./startup/crt0.o \
./startup/flash_boot.o \
./startup/init-default.o \
./startup/interrupt.o \
./startup/retarget.o 

C_DEPS += \
./startup/flash_boot.d \
./startup/init-default.d \
./startup/interrupt.d \
./startup/retarget.d 

S_UPPER_DEPS += \
./startup/crt0.d 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/D/BanAirBundy/boot_app" -I"/cygdrive/D/BanAirBundy/boot_app/system_config" -I"/cygdrive/D/BanAirBundy/boot_app/src" -I"/cygdrive/D/BanAirBundy/boot_app/wireless_mv/include" -I"/cygdrive/D/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/D/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/D/BanAirBundy/boot_app/startup" -I"/cygdrive/D/BanAirBundy/boot_app/banux" -I"/cygdrive/D/BanAirBundy/boot_app/banux/inc" -I"/cygdrive/D/BanAirBundy/boot_app/banux/00_core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_driver" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_driver/library" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework/core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework/vfs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/file_io" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/event" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/command_line" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/command_parser" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/app" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/src" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/src/drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/target" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/internal_flash_fs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_application_components/firmware_upgrade" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

startup/%.o: ../startup/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/D/BanAirBundy/boot_app" -I"/cygdrive/D/BanAirBundy/boot_app/system_config" -I"/cygdrive/D/BanAirBundy/boot_app/src" -I"/cygdrive/D/BanAirBundy/boot_app/wireless_mv/include" -I"/cygdrive/D/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/D/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/D/BanAirBundy/boot_app/startup" -I"/cygdrive/D/BanAirBundy/boot_app/banux" -I"/cygdrive/D/BanAirBundy/boot_app/banux/inc" -I"/cygdrive/D/BanAirBundy/boot_app/banux/00_core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_driver" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_driver/library" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework/core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework/vfs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/file_io" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/event" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/command_line" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/command_parser" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/app" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/src" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/src/drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/target" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/internal_flash_fs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_application_components/firmware_upgrade" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


