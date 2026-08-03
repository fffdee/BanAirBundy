################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/croutine.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/event_groups.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/heap_5s.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/list.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/port.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/portISR.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/queue.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/shell.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/tasks.c \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/timers.c 

S_UPPER_SRCS += \
E:/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/os_cpu_a.S 

OBJS += \
./middleware/rtos/freertos/src/croutine.o \
./middleware/rtos/freertos/src/event_groups.o \
./middleware/rtos/freertos/src/heap_5s.o \
./middleware/rtos/freertos/src/list.o \
./middleware/rtos/freertos/src/os_cpu_a.o \
./middleware/rtos/freertos/src/port.o \
./middleware/rtos/freertos/src/portISR.o \
./middleware/rtos/freertos/src/queue.o \
./middleware/rtos/freertos/src/shell.o \
./middleware/rtos/freertos/src/tasks.o \
./middleware/rtos/freertos/src/timers.o 

C_DEPS += \
./middleware/rtos/freertos/src/croutine.d \
./middleware/rtos/freertos/src/event_groups.d \
./middleware/rtos/freertos/src/heap_5s.d \
./middleware/rtos/freertos/src/list.d \
./middleware/rtos/freertos/src/port.d \
./middleware/rtos/freertos/src/portISR.d \
./middleware/rtos/freertos/src/queue.d \
./middleware/rtos/freertos/src/shell.d \
./middleware/rtos/freertos/src/tasks.d \
./middleware/rtos/freertos/src/timers.d 

S_UPPER_DEPS += \
./middleware/rtos/freertos/src/os_cpu_a.d 


# Each subdirectory must supply rules for building sources it contributes
middleware/rtos/freertos/src/croutine.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/croutine.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/event_groups.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/event_groups.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/heap_5s.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/heap_5s.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/list.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/list.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/os_cpu_a.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/os_cpu_a.S
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/port.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/port.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/portISR.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/portISR.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/queue.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/queue.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/shell.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/shell.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/tasks.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/tasks.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

middleware/rtos/freertos/src/timers.o: /cygdrive/E/project_and_dataset/project/wireless_mic_1532/BG_Audio_Looper/MVsB1_Base_SDK/middleware/rtos/freertos/src/timers.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -Os -mcmodel=medium -Wall -mcpu=d1088-spu -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


