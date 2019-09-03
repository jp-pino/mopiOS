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
// VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIAblE FOR spECIAL, INCIDENTAL,
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
  .global    freeTCB


// Returns highest PL with one thread in r0
// Returns 32 if no bits are set in the variable (no thread is available to run)

.thumb_func
SVC_Handler:
  ldr       r0, [sp, #24]
  ldrH      r0, [r0, #-2]
  and       r0, r0, #0xFF
  // Load arg0, arg1, arg2 into r1, r2, r3
  ldr       r1, [sp, #0]
  ldr       r2, [sp, #4]
  ldr       r3, [sp, #8]
  push      {lr}
  bl        doServiceCall
  pop       {lr}
  str       r0, [sp, #0]         // Store return value in r0 on stack
  bx        lr



.thumb_func
GetHighestPriority:
  ldr       r0, =threadActiveBitField   // Address of bit field => r0
  ldr       r0, [r0]                    // Value of bit field => r0
  rbit      r0, r0                      // Reverse bit order (32 bit word)
  clz       r0, r0                      // # of leading zeroes of word => r0
  bx        lr

.thumb_func
PendSV_Handler:
  cpsid     i                     // Disable interrupts
  push      {r4-r11}              // Push registers not pushed by ISR
  push      {lr}
  bl        GetHighestPriority    // Highest PL with a thread => r0
  pop       {lr}
  push      {r0, lr}
  mov       r0, #2
  bl        OS_LogEntry
  pop       {r0, lr}
  ldr       r1, =RunPt            // Address of RunPt => r1
  ldr       r2, [r1]              // RunPt => r2
  ldr       r4, [r1]
cont:
  ldr       r3, =roundRobin       // &roundRobin => r3
  ldr       r3, [r3]              // roundRobin => r3
  cmp       r3, #1                // if roundRobin, do round robin
  beq       DoRoundRobin
  ldr       r3, =tcbLists         // Address of tcbLists => r3
  push      {r0}
  add       r3, r3, r0, LSL #2    // Add priority (x4 bc each element is 4 bytes) to address to index into tcbLists
  pop       {r0}
  ldr       r3, [r3]              // Load address of next TCB into r3
  B         contextSwitch
DoRoundRobin:
  ldr       r3, [r2, #4]          // RunPt->next => r3
contextSwitch:                     // Expectations: &RunPt = r1, RunPt = r2, NextPt = r3
  str       sp, [r2]              // Save sp into current TCB
  str       r3, [r1]              // Store next TCB pointer to RunPt
  ldr       sp, [r3]              // Load next stack pointer into sp
  push      {r0, r1, r2, lr}
  ldr       r1, [r4, #32]         // RunPt->deleteThis => r3
  cmp       r1, #0
  mov       r0, r4
  beq       skipDelete
  bl        freeTCB
skipDelete:
  pop       {r0, r1, r2, lr}
  push      {r0, lr}
  mov       r0, #1
  bl        OS_LogEntry
  pop       {r0, lr}
  pop       {r4-r11}              // Pop registers not popped by ISR
  cpsie     i
  bx        lr                    // Return from interrupt

.thumb_func
StartOS:
	// CPACR is located at address 0xE000ED88
	ldr.W r0, =0xE000ED88
	// Read CPACR
	ldr r1, [r0]
	// Set bits 20-23 to enable CP10 and CP11 coprocessors
	orr r1, r1, #(0xF << 20)
	// Write back the modified value to the CPACR
	str r1, [r0]
	// wait for store to complete
	dsb
	//reset pipeline now the FPU is enabled
	isb
  ldr       r0, =RunPt            // currently running thread
  ldr       r2, [r0]              // r2 = value of RunPt
  ldr       sp, [r2]              // new thread sp sp = RunPt->stackPointer
  pop       {r4-r11}              // restore regs r4-11
  pop       {r0-r3}               // restore regs r0-3
  pop       {r12}
  pop       {lr}                  // discard lr from initial stack
  pop       {lr}                  // start location
  pop       {r1}                  // discard PSR
	pop				{r1}									// discard FPU
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
	pop				{r1}
  cpsie     i                     // Enable interrupts at processor level
  bx        lr                    // start first thread

  .align
  .end
