################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../source/gcckdm/GccKdmPlugin.cc \
../source/gcckdm/GccKdmUtilities.cc \
../source/gcckdm/KdmPredicate.cc 

OBJS += \
./source/gcckdm/GccKdmPlugin.o \
./source/gcckdm/GccKdmUtilities.o \
./source/gcckdm/KdmPredicate.o 

CC_DEPS += \
./source/gcckdm/GccKdmPlugin.d \
./source/gcckdm/GccKdmUtilities.d \
./source/gcckdm/KdmPredicate.d 


# Each subdirectory must supply rules for building sources it contributes
source/gcckdm/%.o: ../source/gcckdm/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++-4.5 -I"/home/kgirard/git/gcckdm/include" -I/usr/lib/gcc/x86_64-linux-gnu/4.5/plugin/include -I/usr/lib/x86_64-linux-gnu/gcc/x86_64-linux-gnu/4.5/plugin/include -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -Wno-deprecated -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


