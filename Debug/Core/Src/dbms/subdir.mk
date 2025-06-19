################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/dbms/dbms.c 

OBJS += \
./Core/Src/dbms/dbms.o 

C_DEPS += \
./Core/Src/dbms/dbms.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/dbms/%.o Core/Src/dbms/%.su Core/Src/dbms/%.cyclo: ../Core/Src/dbms/%.c Core/Src/dbms/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-dbms

clean-Core-2f-Src-2f-dbms:
	-$(RM) ./Core/Src/dbms/dbms.cyclo ./Core/Src/dbms/dbms.d ./Core/Src/dbms/dbms.o ./Core/Src/dbms/dbms.su

.PHONY: clean-Core-2f-Src-2f-dbms

