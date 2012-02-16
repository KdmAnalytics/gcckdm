################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../source/gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.cc \
../source/gcckdm/kdmtriplewriter/KdmTripleWriter.cc 

OBJS += \
./source/gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.o \
./source/gcckdm/kdmtriplewriter/KdmTripleWriter.o 

CC_DEPS += \
./source/gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.d \
./source/gcckdm/kdmtriplewriter/KdmTripleWriter.d 


# Each subdirectory must supply rules for building sources it contributes
source/gcckdm/kdmtriplewriter/%.o: ../source/gcckdm/kdmtriplewriter/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++-4.5 -I../include -I/usr/lib/gcc/x86_64-linux-gnu/4.5/plugin/include -I/usr/lib/x86_64-linux-gnu/gcc/x86_64-linux-gnu/4.5/plugin/include -O3 -Wall -c -fmessage-length=0 -fPIC -Wno-deprecated -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


