################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../source/gcckdm/GccKdmPlugin.cc \
../source/gcckdm/GccKdmUtilities.cc \
../source/gcckdm/KdmPredicate.cc \
../source/gcckdm/KdmType.cc 

OBJS += \
./source/gcckdm/GccKdmPlugin.o \
./source/gcckdm/GccKdmUtilities.o \
./source/gcckdm/KdmPredicate.o \
./source/gcckdm/KdmType.o 

CC_DEPS += \
./source/gcckdm/GccKdmPlugin.d \
./source/gcckdm/GccKdmUtilities.d \
./source/gcckdm/KdmPredicate.d \
./source/gcckdm/KdmType.d 


# Each subdirectory must supply rules for building sources it contributes
source/gcckdm/%.o: ../source/gcckdm/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++-4.5 -I../include -I/usr/lib/gcc/x86_64-linux-gnu/4.5/plugin/include -O3 -Wall -c -fmessage-length=0 -fPIC -Wno-deprecated -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


