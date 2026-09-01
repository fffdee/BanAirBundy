################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../middleware/mv_utils/src/debug.c \
../middleware/mv_utils/src/heap.c \
../middleware/mv_utils/src/mcu_circular_buf.c 

OBJS += \
./middleware/mv_utils/src/debug.o \
./middleware/mv_utils/src/heap.o \
./middleware/mv_utils/src/mcu_circular_buf.o 

C_DEPS += \
./middleware/mv_utils/src/debug.d \
./middleware/mv_utils/src/heap.d \
./middleware/mv_utils/src/mcu_circular_buf.d 


# Each subdirectory must supply rules for building sources it contributes
middleware/mv_utils/src/%.o: ../middleware/mv_utils/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DCFG_APP_CONFIG -DCFG_CHIP_CONFIG -I"/cygdrive/D/BanAirBundy/boot_app" -I"/cygdrive/D/BanAirBundy/boot_app/system_config" -I"/cygdrive/D/BanAirBundy/boot_app/src" -I"/cygdrive/D/BanAirBundy/boot_app/wireless_mv/include" -I"/cygdrive/D/BanAirBundy/boot_app/otg/device/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver_api/inc" -I"/cygdrive/D/BanAirBundy/boot_app/driver/driver/inc" -I"/cygdrive/D/BanAirBundy/boot_app/middleware/mv_utils/inc" -I"/cygdrive/D/BanAirBundy/boot_app/FreeRTOS/Source/include" -I"/cygdrive/D/BanAirBundy/boot_app/startup" -I"/cygdrive/D/BanAirBundy/boot_app/banux" -I"/cygdrive/D/BanAirBundy/boot_app/banux/inc" -I"/cygdrive/D/BanAirBundy/boot_app/banux/00_core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_driver" -I"/cygdrive/D/BanAirBundy/boot_app/banux/01_driver/library" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework/core" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/driver_framework/vfs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/file_io" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/event" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/command_line" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/command_parser" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/app" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/src" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/src/drivers" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/fatfs/target" -I"/cygdrive/D/BanAirBundy/boot_app/banux/02_system_components/internal_flash_fs" -I"/cygdrive/D/BanAirBundy/boot_app/banux/03_application_components/firmware_upgrade" -Og -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -fsingle-precision-constant -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


