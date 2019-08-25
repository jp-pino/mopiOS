//*****************************************************************************
//
// Lab5.c - Barebones OS initialization
//*****************************************************************************

#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "OS.h"
#include "Interpreter.h"
#include "UART.h"

#define TIMESLICE TIME_1MS  // thread switch time in system time units

void seconds(void) {
  ST7735_Message(ST7735_DISPLAY_TOP, 0, "Thread: ", OS_Id());
  ST7735_Message(ST7735_DISPLAY_TOP, 1, "Time: ", OS_Time()/1000);
  OS_Sleep(10000);
  ST7735_Message(ST7735_DISPLAY_BOTTOM, 0, "Thread: ", OS_Id());
  ST7735_Message(ST7735_DISPLAY_BOTTOM, 1, "Time: ", OS_Time()/1000);
  OS_Kill();
}


void test(void) {
  OS_AddThread(&seconds, 128, 1);
}

void setup(void) {
  OS_Sleep(5000);
  UART_OutString("\n\r  Adding command");
  IT_AddCommand("test", 0, "", &test, "perform test");
  UART_OutString("\n\r  Command added successfully!");
  OS_Kill();
}

// OS and modules initialization
int main(void){        // lab 4 real main
  // Initialize the OS
  OS_Init();           // initialize, disable interrupts
  // Initialize other modules

  // Add Threads
  // OS_AddThread(..., ..., ...);
  OS_AddThread(&setup, 128, 5);

  // Launch the OS
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}
