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

  .global OS_AddThread_Test
  .global OS_Suspend_Test
  .global OS_Kill_Test
  .global OS_Time_Test
  .global OS_TimeDifference_Test
  .global OS_MsTime_Test
  .global OS_ClearMsTime_Test
  .global OS_Sleep_Test
  .global OS_Wait_Test
  .global OS_bWait_Test
  .global OS_Signal_Test
  .global OS_bSignal_Test

.thumb_func
OS_AddThread_Test:
  SVC #0
  BX LR

.thumb_func
OS_Suspend_Test:
  SVC #1
  BX LR

.thumb_func
OS_Kill_Test:
  SVC #2
  BX LR

.thumb_func
OS_Time_Test:
  SVC #3
  BX LR

.thumb_func
OS_TimeDifference_Test:
  SVC #4
  BX LR

.thumb_func
OS_MsTime_Test:
  SVC #5
  BX LR

.thumb_func
OS_ClearMsTime_Test:
  SVC #6
  BX LR

.thumb_func
OS_Sleep_Test:
  SVC #7
  BX LR

.thumb_func
OS_Wait_Test:
  SVC #8
  BX LR

.thumb_func
OS_bWait_Test:
  SVC #9
  BX LR

.thumb_func
OS_Signal_Test:
  SVC #10
  BX LR

.thumb_func
OS_bSignal_Test:
  SVC #11
  BX LR

.thumb_func
OS_Id_Test:
  SVC #12
  BX LR

  .align
  .end
