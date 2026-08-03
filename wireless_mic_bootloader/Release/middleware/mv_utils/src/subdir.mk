################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
E:/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/src/debug.c \
E:/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/src/mcu_circular_buf.c 

OBJS += \
./middleware/mv_utils/src/debug.o \
./middleware/mv_utils/src/mcu_circular_buf.o 

C_DEPS += \
./middleware/mv_utils/src/debug.d \
./middleware/mv_utils/src/mcu_circular_buf.d 


# Each subdirectory must supply rules for building sources it contributes
middleware/mv_utils/src/debug.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/src/debug.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/mv_utils/src/mcu_circular_buf.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/wireless_mic_unified_sdk/middleware/mv_utils/src/mcu_circular_buf.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


