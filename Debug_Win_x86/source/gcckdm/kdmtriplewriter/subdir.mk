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
	g++-4.5 -D__WIN32__ -I"C:\boost-1.43" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


