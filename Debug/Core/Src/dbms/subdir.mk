################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/dbms/blackbox.c \
../Core/Src/dbms/can.c \
../Core/Src/dbms/dbms.c \
../Core/Src/dbms/elcon.c \
../Core/Src/dbms/isense.c \
../Core/Src/dbms/ledctl.c \
../Core/Src/dbms/pwm.c \
../Core/Src/dbms/sched.c \
../Core/Src/dbms/settings.c \
../Core/Src/dbms/storage.c \
../Core/Src/dbms/vinterface.c 

OBJS += \
./Core/Src/dbms/blackbox.o \
./Core/Src/dbms/can.o \
./Core/Src/dbms/dbms.o \
./Core/Src/dbms/elcon.o \
./Core/Src/dbms/isense.o \
./Core/Src/dbms/ledctl.o \
./Core/Src/dbms/pwm.o \
./Core/Src/dbms/sched.o \
./Core/Src/dbms/settings.o \
./Core/Src/dbms/storage.o \
./Core/Src/dbms/vinterface.o 

C_DEPS += \
./Core/Src/dbms/blackbox.d \
./Core/Src/dbms/can.d \
./Core/Src/dbms/dbms.d \
./Core/Src/dbms/elcon.d \
./Core/Src/dbms/isense.d \
./Core/Src/dbms/ledctl.d \
./Core/Src/dbms/pwm.d \
./Core/Src/dbms/sched.d \
./Core/Src/dbms/settings.d \
./Core/Src/dbms/storage.d \
./Core/Src/dbms/vinterface.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/dbms/%.o Core/Src/dbms/%.su Core/Src/dbms/%.cyclo: ../Core/Src/dbms/%.c Core/Src/dbms/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -USIM -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-dbms

clean-Core-2f-Src-2f-dbms:
	-$(RM) ./Core/Src/dbms/blackbox.cyclo ./Core/Src/dbms/blackbox.d ./Core/Src/dbms/blackbox.o ./Core/Src/dbms/blackbox.su ./Core/Src/dbms/can.cyclo ./Core/Src/dbms/can.d ./Core/Src/dbms/can.o ./Core/Src/dbms/can.su ./Core/Src/dbms/dbms.cyclo ./Core/Src/dbms/dbms.d ./Core/Src/dbms/dbms.o ./Core/Src/dbms/dbms.su ./Core/Src/dbms/elcon.cyclo ./Core/Src/dbms/elcon.d ./Core/Src/dbms/elcon.o ./Core/Src/dbms/elcon.su ./Core/Src/dbms/isense.cyclo ./Core/Src/dbms/isense.d ./Core/Src/dbms/isense.o ./Core/Src/dbms/isense.su ./Core/Src/dbms/ledctl.cyclo ./Core/Src/dbms/ledctl.d ./Core/Src/dbms/ledctl.o ./Core/Src/dbms/ledctl.su ./Core/Src/dbms/pwm.cyclo ./Core/Src/dbms/pwm.d ./Core/Src/dbms/pwm.o ./Core/Src/dbms/pwm.su ./Core/Src/dbms/sched.cyclo ./Core/Src/dbms/sched.d ./Core/Src/dbms/sched.o ./Core/Src/dbms/sched.su ./Core/Src/dbms/settings.cyclo ./Core/Src/dbms/settings.d ./Core/Src/dbms/settings.o ./Core/Src/dbms/settings.su ./Core/Src/dbms/storage.cyclo ./Core/Src/dbms/storage.d ./Core/Src/dbms/storage.o ./Core/Src/dbms/storage.su ./Core/Src/dbms/vinterface.cyclo ./Core/Src/dbms/vinterface.d ./Core/Src/dbms/vinterface.o ./Core/Src/dbms/vinterface.su

.PHONY: clean-Core-2f-Src-2f-dbms

