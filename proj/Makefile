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
	${ROOT}/vendor/_tm4c		          \
	${ROOT}/vendor/ADC/inc            \
	${ROOT}/vendor/ELF/inc            \
	${ROOT}/vendor/FIFO/inc           \
	${ROOT}/vendor/Heap/inc           \
	${ROOT}/vendor/Interpreter/inc    \
	${ROOT}/vendor/LCD/ST7735/inc     \
	${ROOT}/vendor/OS/inc             \
	${ROOT}/vendor/PING/inc           \
	${ROOT}/vendor/PLL/inc            \
	${ROOT}/vendor/SD/inc             \
	${ROOT}/vendor/UART/inc           \
	$/inc           									\

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
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/_tm4c/startup.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/_tm4c/libdriver.a
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/_tm4c/tm4c123gxl.ld
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/PLL/src/PLL.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/OS/src/osasm_gnu.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/OS/src/OS.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/LCD/ST7735/src/ST7735.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/ADC/src/ADC.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/ADC/src/ADCSWTrigger.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/ELF/src/loader.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/HEAP/src/heap.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/INTERPRETER/src/Interpreter.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/UART/src/UART.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/PING/src/Ultrasonic.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/SD/src/diskio.o
${BUILDPATH}/$(PROJ_NAME).axf: ${ROOT}/vendor/SD/src/ff.o
SCATTERgcc_$(PROJ_NAME)=${ROOT}/vendor/_tm4c/tm4c123gxl.ld
ENTRY_$(PROJ_NAME)=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C123_RB1


# *****************************************************************************
# Add object files you need below
# *****************************************************************************
${BUILDPATH}/$(PROJ_NAME).axf: src/main.o
# *****************************************************************************


#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${BUILDPATH}/*.d} __dummy__
endif
