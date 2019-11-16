//*****************************************************************************
//
// Lab5.c - Barebones OS initialization
//*****************************************************************************

#include "tm4c123gh6pm.h"
#include "OS.h"
#include "joystick.h"
#include "teleop.h"

#define TIMESLICE TIME_2MS  //thread switch time in system time units


// OS and modules initialization
int main(void) {        // lab 4 real main
  // Initialize the OS
  OS_Init();           // initialize, disable interrupts

	// OS_AddThread("test", &test, 512, 5);
	// Joystick_Init();
	// Teleop_Init();
	Speed_Init();

  // Launch the OS
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}

// Kill thread by name
