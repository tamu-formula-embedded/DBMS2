################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/dbms/current_meter.c \
../Core/Src/dbms/dbms.c \
../Core/Src/dbms/led_controller.c \
../Core/Src/dbms/sim.c \
../Core/Src/dbms/stack_controller.c \
../Core/Src/dbms/vehicle_interface.c 

OBJS += \
./Core/Src/dbms/current_meter.o \
./Core/Src/dbms/dbms.o \
./Core/Src/dbms/led_controller.o \
./Core/Src/dbms/sim.o \
./Core/Src/dbms/stack_controller.o \
./Core/Src/dbms/vehicle_interface.o 

C_DEPS += \
./Core/Src/dbms/current_meter.d \
./Core/Src/dbms/dbms.d \
./Core/Src/dbms/led_controller.d \
./Core/Src/dbms/sim.d \
./Core/Src/dbms/stack_controller.d \
./Core/Src/dbms/vehicle_interface.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/dbms/%.o Core/Src/dbms/%.su Core/Src/dbms/%.cyclo: ../Core/Src/dbms/%.c Core/Src/dbms/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-dbms

clean-Core-2f-Src-2f-dbms:
	-$(RM) ./Core/Src/dbms/current_meter.cyclo ./Core/Src/dbms/current_meter.d ./Core/Src/dbms/current_meter.o ./Core/Src/dbms/current_meter.su ./Core/Src/dbms/dbms.cyclo ./Core/Src/dbms/dbms.d ./Core/Src/dbms/dbms.o ./Core/Src/dbms/dbms.su ./Core/Src/dbms/led_controller.cyclo ./Core/Src/dbms/led_controller.d ./Core/Src/dbms/led_controller.o ./Core/Src/dbms/led_controller.su ./Core/Src/dbms/sim.cyclo ./Core/Src/dbms/sim.d ./Core/Src/dbms/sim.o ./Core/Src/dbms/sim.su ./Core/Src/dbms/stack_controller.cyclo ./Core/Src/dbms/stack_controller.d ./Core/Src/dbms/stack_controller.o ./Core/Src/dbms/stack_controller.su ./Core/Src/dbms/vehicle_interface.cyclo ./Core/Src/dbms/vehicle_interface.d ./Core/Src/dbms/vehicle_interface.o ./Core/Src/dbms/vehicle_interface.su

.PHONY: clean-Core-2f-Src-2f-dbms

