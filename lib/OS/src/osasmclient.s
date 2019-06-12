;/*****************************************************************************/
; OSasm.s: low-level OS commands, written in assembly
; Runs on LM4F120/TM4C123
; A very simple real time operating system with minimal features.
; Daniel Valvano
; January 29, 2015
;
; This example accompanies the book
;    "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
;    ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
;
;    Programs 4.4 through 4.12, section 4.2
;
;Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
;        You may use, edit, run or distribute this file
;        as long as the above copyright notice remains
; THIS SOFTWARE IS PROVIDED "AS IS".    NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
; OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; For more information about my classes, my research, and my books, see
; http://users.ece.utexas.edu/~valvano/
;

  AREA |.text|, CODE, READONLY, ALIGN=2
  THUMB
  REQUIRE8
  PRESERVE8
    
  EXPORT OS_AddThread_Test
  EXPORT OS_Suspend_Test
  EXPORT OS_Kill_Test
  EXPORT OS_Time_Test
  EXPORT OS_TimeDifference_Test
  EXPORT OS_MsTime_Test
  EXPORT OS_ClearMsTime_Test
  EXPORT OS_Sleep_Test
  EXPORT OS_Wait_Test
  EXPORT OS_bWait_Test
  EXPORT OS_Signal_Test
  EXPORT OS_bSignal_Test
  EXPORT OS_Id_Test
    
OS_AddThread_Test
  SVC #0
  BX LR
    
OS_Suspend_Test
  SVC #1
  BX LR
  
OS_Kill_Test
  SVC #2
  BX LR
    
OS_Time_Test
  SVC #3
  BX LR
  
OS_TimeDifference_Test
  SVC #4
  BX LR
  
OS_MsTime_Test
  SVC #5
  BX LR
  
OS_ClearMsTime_Test
  SVC #6
  BX LR
  
OS_Sleep_Test
  SVC #7
  BX LR
  
OS_Wait_Test
  SVC #8
  BX LR
  
OS_bWait_Test
  SVC #9
  BX LR
  
OS_Signal_Test
  SVC #10
  BX LR
  
OS_bSignal_Test
  SVC #11
  BX LR  
  
OS_Id_Test
  SVC #12
  BX LR
  
  END
  