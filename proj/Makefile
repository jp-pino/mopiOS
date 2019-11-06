#******************************************************************************
#
# Makefile - Makefile for building this project.
#
# Copyright (c) 2012-2017 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
#
# Texas Instruments (TI) is supplying this software for use solely and
# exclusively on TI's microcontroller products. The software is owned by
# TI and/or its suppliers, and is protected under applicable copyright
# laws. You may not combine this software with "viral" open-source
# software in order to form a larger program.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
# NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
# NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
# CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
# DAMAGES, FOR ANY REASON WHATSOEVER.
#
# This is part of revision 2.1.4.178 of the EK-TM4C123GXL Firmware Package.
#
#******************************************************************************

#******************************************************************************
#
# Modified by Zee Lv
# Nov 6, 2018
#
#******************************************************************************

#******************************************************************************
#
# Modified by Juan Pablo Pino
# Jan, 2019
#
#******************************************************************************

#
# Defines project name
#
PROJ_NAME=mopiOS

#
# Defines the part type that this project uses.
#
PART=TM4C123GH6PM

#
# The root directory.
#
ROOT=..

#
# Include the common make definitions.
#
include ${ROOT}/common.mk

#
# Where to find header files that do not live in the source directory.
#
IPATH += \
	${ROOT}/lib/_tm4c		           \
	${ROOT}/lib/ADC/inc            \
	${ROOT}/lib/ELF/inc            \
	${ROOT}/lib/FIFO/inc           \
	${ROOT}/lib/Heap/inc           \
	${ROOT}/lib/WATCHDOG/inc           \
	${ROOT}/proj/lib/INTERPRETER/inc    \
	${ROOT}/lib/LCD/ST7735/inc     \
	${ROOT}/lib/OS/inc             \
	${ROOT}/lib/MAILBOX/inc             \
	${ROOT}/proj/lib/PING/inc           \
	${ROOT}/lib/PLL/inc            \
	${ROOT}/lib/SD/inc             \
	${ROOT}/lib/UART/inc           \
	${ROOT}/lib/UART1/inc          \
	${ROOT}/lib/UART5/inc          \
	${ROOT}/lib/Motor/inc          \
	${ROOT}/proj/lib/WIFI/inc    \
	${ROOT}/proj/lib/printf/inc      \
	${ROOT}/proj/lib/ROSSERIAL/inc      \
	${ROOT}/proj/lib/JOYSTICK/inc      \
	${ROOT}/proj/lib/SPEED/inc      \
	${ROOT}/proj/lib/ROSSERIAL/msgs/rosserial_msgs/TopicInfo     \
	${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Bool      					\
	${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Float32      				\
	${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Float64      				\
	${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Int32	      				\
	${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/UInt32	      			\
	${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Time		      			\
	${ROOT}/proj/lib/ROSSERIAL/msgs/geometry_msgs/Vector3		      			\
	${ROOT}/proj/lib/ROSSERIAL/msgs/geometry_msgs/Twist		      			\
	$/inc           							 													\

#
# The default rule
#
all: ${BUILDPATH}
all: ${BUILDPATH}/$(PROJ_NAME).axf


#
# The rule to create the target directory.
#
${BUILDPATH}:
	@mkdir -p ${BUILDPATH}

#
# Rules for building.
#
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/_tm4c/startup.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/_tm4c/libdriver.a
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/_tm4c/tm4c123gxl.ld
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/PLL/src/PLL.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/OS/src/osasm_gnu.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/OS/src/OS.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/OS/src/OS_Functions.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/MAILBOX/src/mailbox.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/LCD/ST7735/src/ST7735.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/ADC/src/ADC.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/ADC/src/ADCSWTrigger.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/ELF/src/loader.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/HEAP/src/heap.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/UART/src/UART.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/UART1/src/UART1.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/UART5/src/UART5.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/Motor/src/Motor.o
# ${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/ESP8266/src/esp8266.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/SD/src/diskio.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/SD/src/ff.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/SD/src/ff.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/lib/WATCHDOG/src/watchdog.o
SCATTERgcc_$(PROJ_NAME)=${ROOT}/lib/_tm4c/tm4c123gxl.ld
ENTRY_$(PROJ_NAME)=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C123_RB1


# *****************************************************************************
# Add object files you need below
# *****************************************************************************
# Main file
${BUILDPATH}/$(PROJ_NAME).axf: src/main.o
${BUILDPATH}/$(PROJ_NAME).axf: src/teleop.o
# Speed controller
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/SPEED/src/Speed.o
# Joystick
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/JOYSTICK/src/joystick.o
# Interpreter
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/INTERPRETER/src/Interpreter.o
# Wifi
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/WIFI/src/wifi.o
# ROS compatibility
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/src/ros.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/rosserial_msgs/TopicInfo/TopicInfo.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Bool/Bool.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Float32/Float32.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Float64/Float64.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Int32/Int32.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/UInt32/UInt32.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/std_msgs/Time/Time.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/geometry_msgs/Vector3/Vector3.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/ROSSERIAL/msgs/geometry_msgs/Twist/Twist.o
# Ultrasonic
# ${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/PING/src/Ultrasonic.o
# printf
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/proj/lib/printf/src/printf.o
# *****************************************************************************


#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${BUILDPATH}/*.d} __dummy__
endif

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${BUILDPATH} ${wildcard *~} 2> /dev/null
	@find ../. -name "*.o" -type f -delete
	@find ../. -name "*.d" -type f -delete
