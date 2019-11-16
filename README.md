# mopiOS

## Introduction
mopiOS is a Real Time Operating System for the TI TM4C123GH6PM Microcontroller.

## Features
- Dynamic allocation for tcbs.
- ROS Compatibility. For examples on using my rosserial port check [here](proj/lib/ROSSERIAL).
- API for [Interpreter](proj/lib/INTERPRETER).
- FPU enabled for float operations!

## Extras
Check out my other plugins for mopiOS:
- [mopiOS-Interpreter](https://github.com/jp-pino/mopiOS-PIC): an implementation of the mopiOS Interpreter for use with an ESP-01 module, through a Socket.io connection.
- [mopiOS-PIC](https://github.com/jp-pino/mopiOS-PIC): a position independent code builder for use with the mopiOS's elf loader.

## Instructions
Follow instructions [here](proj) for building and flashing.

## Installation (from ZeeLivermorium's repo)
### Mac OS
#### ARM GCC Tools
```bash
    brew tap PX4/homebrew-px4
    brew update
    brew install gcc-arm-none-eabi
```

Run
```bash
    arm-none-eabi-gcc --version
```
to confirm your installation.

You might run into problems when *arm-none-eabi-gcc* command is not found after installation. Try
```bash
    brew link gcc-arm-none-eabi
```
see what homebrew tells you to fix linking problem.

**NOTE**: The old magic command
```bash
    brew cask install gcc-arm-embedded
```
does not work anymore. They removed it from cask. Put it here in case you are wondering or you are trying to use [UT-Rasware](https://github.com/ut-ras/Rasware) installation guide.

#### Lm4tools
```bash
    brew install lm4tools
```
#### OpenOCD
```bash
    brew install open-ocd
```

## Acknowledgments
[Cole Morgan](https://github.com/coleamorgan) is the co-creator of mopiOS. This project would not have been possible without him.  
[ZeeLivermorium](https://github.com/ZeeLivermorium)'s GCC Tools for TI TM4C123GXL MCU are used in this repo for compiling on macOS. You can find his original tools and installation instructions [here](https://github.com/ZeeLivermorium/zEEware).
