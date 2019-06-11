///*****************************************************************************/
// OSasm.s: low-level OS commands, written in assembly
// Runs on LM4F120/TM4C123
// A very simple real time operating system with minimal features.
// Daniel Valvano
// January 29, 2015
//
// This example accompanies the book
//    "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
//    ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
//
//    Programs 4.4 through 4.12, section 4.2
//
//Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
//        You may use, edit, run or distribute this file
//        as long as the above copyright notice remains
// THIS SOFTWARE IS PROVIDED "AS IS".    NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
// OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
// For more information about my classes, my research, and my books, see
// http://users.ece.utexas.edu/~valvano/
//


  .section .text, "ax"
  .balign 4
  .thumb
  .syntax unified


  .global    RunPt
  .global    SleepPt
  .global    threadActiveBitField
  .global    tcbLists
  .global    roundRobin
  .global    StartOS
  .global    PendSV_Handler
  .global    GetHighestPriority
  .global    sleep_decrement
  .global    measureIntEn
  .global    measureIntDis
  .global    OS_LogEntry
  .global    SVC_Handler
  .global    doServiceCall


// Returns highest PL with one thread in R0
// Returns 32 if no bits are set in the variable (no thread is available to run)

.thumb_func
SVC_Handler:
  LDR       R0, [SP, #24]
  LDRH      R0, [R0, #-2]
  AND       R0, R0, #0xFF
  // Load arg0, arg1, arg2 into R1, R2, R3
  LDR       R1, [SP, #0]
  LDR       R2, [SP, #4]
  LDR       R3, [SP, #8]
  PUSH      {LR}
  BL        doServiceCall
  POP       {LR}
  STR       R0, [SP, #0]         // Store return value in R0 on stack
  BX        LR



.thumb_func
GetHighestPriority:
  LDR       R0, =threadActiveBitField   // Address of bit field => R0
  LDR       R0, [R0]                    // Value of bit field => R0
  RBIT      R0, R0                      // Reverse bit order (32 bit word)
  CLZ       R0, R0                      // # of leading zeroes of word => R0
  BX        LR

.thumb_func
PendSV_Handler:
  CPSID     I                     // Disable interrupts
  PUSH      {R4-R11}              // Push registers not pushed by ISR
  PUSH      {LR}
  BL        GetHighestPriority    // Highest PL with a thread => R0
  POP       {LR}
  PUSH      {LR, R0}
  MOV       R0, #2
  BL        OS_LogEntry
  POP       {LR, R0}
  LDR       R1, =RunPt            // Address of RunPt => R1
  LDR       R2, [R1]              // RunPt => R2
  LDR       R3, [R2, #32]         // RunPt->deleteThis => R3
  CMP       R3, #0
  BEQ       cont
  MOV       R3, #0
  STR       R3, [R2, #20]         // RunPt->inUse = 0
  STR       R3, [R2, #32]         // RunPt->deleteThis = 0
cont:
  LDR       R3, =roundRobin       // &roundRobin => R3
  LDR       R3, [R3]              // roundRobin => R3
  CMP       R3, #1                // if roundRobin, do round robin
  BEQ       DoRoundRobin
  LDR       R3, =tcbLists         // Address of tcbLists => R3
  ADD       R3, R3, R0, LSL #2    // Add priority (x4 bc each element is 4 bytes) to address to index into tcbLists
  LDR       R3, [R3]              // Load address of next TCB into R3
  B         contextSwitch
DoRoundRobin:
  LDR       R3, [R2, #4]          // RunPt->next => R3
contextSwitch:                     // Expectations: &RunPt = R1, RunPt = R2, NextPt = R3
  STR       SP, [R2]              // Save SP into current TCB
  STR       R3, [R1]              // Store next TCB pointer to RunPt
  LDR       SP, [R3]              // Load next stack pointer into SP
  // PUSH      {LR, R0}
  // MOV       R0, #1
  // BL        OS_LogEntry
  // POP       {LR, R0}
  POP       {R4-R11}              // Pop registers not popped by ISR
  CPSIE     I
  BX        LR                    // Return from interrupt

.thumb_func
StartOS:
  LDR       R0, =RunPt            // currently running thread
  LDR       R2, [R0]              // R2 = value of RunPt
  LDR       SP, [R2]              // new thread SP SP = RunPt->stackPointer
  POP       {R4-R11}              // restore regs r4-11
  POP       {R0-R3}               // restore regs r0-3
  POP       {R12}
  POP       {LR}                  // discard LR from initial stack
  POP       {LR}                  // start location
  POP       {R1}                  // discard PSR
  CPSIE     I                     // Enable interrupts at processor level
  BX        LR                    // start first thread

  .align
  .end
