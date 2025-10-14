################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/dbms/blackbox.c \
../Core/Src/dbms/charging.c \
../Core/Src/dbms/crc.c \
../Core/Src/dbms/current_meter.c \
../Core/Src/dbms/data.c \
../Core/Src/dbms/dbms.c \
../Core/Src/dbms/eswap.c \
../Core/Src/dbms/fault_handler.c \
../Core/Src/dbms/led_controller.c \
../Core/Src/dbms/lut.c \
../Core/Src/dbms/ma.c \
../Core/Src/dbms/model.c \
../Core/Src/dbms/sched.c \
../Core/Src/dbms/settings.c \
../Core/Src/dbms/sim.c \
../Core/Src/dbms/stack_controller.c \
../Core/Src/dbms/storage.c \
../Core/Src/dbms/vehicle_interface.c 

OBJS += \
./Core/Src/dbms/blackbox.o \
./Core/Src/dbms/charging.o \
./Core/Src/dbms/crc.o \
./Core/Src/dbms/current_meter.o \
./Core/Src/dbms/data.o \
./Core/Src/dbms/dbms.o \
./Core/Src/dbms/eswap.o \
./Core/Src/dbms/fault_handler.o \
./Core/Src/dbms/led_controller.o \
./Core/Src/dbms/lut.o \
./Core/Src/dbms/ma.o \
./Core/Src/dbms/model.o \
./Core/Src/dbms/sched.o \
./Core/Src/dbms/settings.o \
./Core/Src/dbms/sim.o \
./Core/Src/dbms/stack_controller.o \
./Core/Src/dbms/storage.o \
./Core/Src/dbms/vehicle_interface.o 

C_DEPS += \
./Core/Src/dbms/blackbox.d \
./Core/Src/dbms/charging.d \
./Core/Src/dbms/crc.d \
./Core/Src/dbms/current_meter.d \
./Core/Src/dbms/data.d \
./Core/Src/dbms/dbms.d \
./Core/Src/dbms/eswap.d \
./Core/Src/dbms/fault_handler.d \
./Core/Src/dbms/led_controller.d \
./Core/Src/dbms/lut.d \
./Core/Src/dbms/ma.d \
./Core/Src/dbms/model.d \
./Core/Src/dbms/sched.d \
./Core/Src/dbms/settings.d \
./Core/Src/dbms/sim.d \
./Core/Src/dbms/stack_controller.d \
./Core/Src/dbms/storage.d \
./Core/Src/dbms/vehicle_interface.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/dbms/%.o Core/Src/dbms/%.su Core/Src/dbms/%.cyclo: ../Core/Src/dbms/%.c Core/Src/dbms/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -USIM -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-dbms

clean-Core-2f-Src-2f-dbms:
	-$(RM) ./Core/Src/dbms/blackbox.cyclo ./Core/Src/dbms/blackbox.d ./Core/Src/dbms/blackbox.o ./Core/Src/dbms/blackbox.su ./Core/Src/dbms/charging.cyclo ./Core/Src/dbms/charging.d ./Core/Src/dbms/charging.o ./Core/Src/dbms/charging.su ./Core/Src/dbms/crc.cyclo ./Core/Src/dbms/crc.d ./Core/Src/dbms/crc.o ./Core/Src/dbms/crc.su ./Core/Src/dbms/current_meter.cyclo ./Core/Src/dbms/current_meter.d ./Core/Src/dbms/current_meter.o ./Core/Src/dbms/current_meter.su ./Core/Src/dbms/data.cyclo ./Core/Src/dbms/data.d ./Core/Src/dbms/data.o ./Core/Src/dbms/data.su ./Core/Src/dbms/dbms.cyclo ./Core/Src/dbms/dbms.d ./Core/Src/dbms/dbms.o ./Core/Src/dbms/dbms.su ./Core/Src/dbms/eswap.cyclo ./Core/Src/dbms/eswap.d ./Core/Src/dbms/eswap.o ./Core/Src/dbms/eswap.su ./Core/Src/dbms/fault_handler.cyclo ./Core/Src/dbms/fault_handler.d ./Core/Src/dbms/fault_handler.o ./Core/Src/dbms/fault_handler.su ./Core/Src/dbms/led_controller.cyclo ./Core/Src/dbms/led_controller.d ./Core/Src/dbms/led_controller.o ./Core/Src/dbms/led_controller.su ./Core/Src/dbms/lut.cyclo ./Core/Src/dbms/lut.d ./Core/Src/dbms/lut.o ./Core/Src/dbms/lut.su ./Core/Src/dbms/ma.cyclo ./Core/Src/dbms/ma.d ./Core/Src/dbms/ma.o ./Core/Src/dbms/ma.su ./Core/Src/dbms/model.cyclo ./Core/Src/dbms/model.d ./Core/Src/dbms/model.o ./Core/Src/dbms/model.su ./Core/Src/dbms/sched.cyclo ./Core/Src/dbms/sched.d ./Core/Src/dbms/sched.o ./Core/Src/dbms/sched.su ./Core/Src/dbms/settings.cyclo ./Core/Src/dbms/settings.d ./Core/Src/dbms/settings.o ./Core/Src/dbms/settings.su ./Core/Src/dbms/sim.cyclo ./Core/Src/dbms/sim.d ./Core/Src/dbms/sim.o ./Core/Src/dbms/sim.su ./Core/Src/dbms/stack_controller.cyclo ./Core/Src/dbms/stack_controller.d ./Core/Src/dbms/stack_controller.o ./Core/Src/dbms/stack_controller.su ./Core/Src/dbms/storage.cyclo ./Core/Src/dbms/storage.d ./Core/Src/dbms/storage.o ./Core/Src/dbms/storage.su ./Core/Src/dbms/vehicle_interface.cyclo ./Core/Src/dbms/vehicle_interface.d ./Core/Src/dbms/vehicle_interface.o ./Core/Src/dbms/vehicle_interface.su

.PHONY: clean-Core-2f-Src-2f-dbms

