################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/lib/crc.c \
../Core/Src/lib/exp_arr.c 

OBJS += \
./Core/Src/lib/crc.o \
./Core/Src/lib/exp_arr.o 

C_DEPS += \
./Core/Src/lib/crc.d \
./Core/Src/lib/exp_arr.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/lib/%.o Core/Src/lib/%.su Core/Src/lib/%.cyclo: ../Core/Src/lib/%.c Core/Src/lib/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -USIM -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-lib

clean-Core-2f-Src-2f-lib:
	-$(RM) ./Core/Src/lib/crc.cyclo ./Core/Src/lib/crc.d ./Core/Src/lib/crc.o ./Core/Src/lib/crc.su ./Core/Src/lib/exp_arr.cyclo ./Core/Src/lib/exp_arr.d ./Core/Src/lib/exp_arr.o ./Core/Src/lib/exp_arr.su

.PHONY: clean-Core-2f-Src-2f-lib

