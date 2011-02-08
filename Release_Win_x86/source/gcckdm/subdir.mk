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
	g++ -D__WIN32__ -I"C:\workspace-c\MingW-kdm-dev\gcc-4.5.0\libcpp\include" -I"C:\workspace-c\MingW-kdm-dev\gcc-4.5.0\include" -I"C:\workspace-c\MingW-kdm-dev\build\gcc" -I"C:\workspace-c\MingW-kdm-dev\gcc-4.5.0\gcc" -I"C:\boost-1.43" -I"C:\workspace-c\GccKdmPlugin\include" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


