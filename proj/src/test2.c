//*****************************************************************************
//
// Lab5.c - Barebones OS initialization
//*****************************************************************************

#include "tm4c123gh6pm.h"
#include "OS.h"
#include "Motor.h"
#include "Joystick.h"

#define TIMESLICE TIME_2MS  // thread switch time in system time units


void test(void) {
	int i = 0;
	while(1) {
		DRV8848_LeftInit(MOTOR_PWM_PERIOD, i, FORWARD);
		DRV8848_RightInit(MOTOR_PWM_PERIOD, i++, FORWARD);
		if (i == MOTOR_PWM_PERIOD)
			i = 0;
		OS_Sleep(10);
	}
}


// OS and modules initialization
int main(void){        // lab 4 real main
  // Initialize the OS
  OS_Init();           // initialize, disable interrupts
  // Initialize other modules

	// Initialize motors
	// DRV8848_LeftInit(MOTOR_PWM_PERIOD, 500, FORWARD);
	// DRV8848_RightInit(MOTOR_PWM_PERIOD, 500, FORWARD);

  // Add Threads
  // OS_AddThread(..., ..., ..., ...);

	OS_AddThread("joystick", &test, 128, 5);
	OS_AddThread("joystick", &Joystick_Main, 1024, 5);
	// OS_AddThread("joystick", &test, 256, 5);


	// Add Topics
	// ROS_AddPublisher(..., ..., ..., ..., ..., ...);
	// ROS_AddSubscriber(..., ..., ..., ..., ..., ...);

	// Add commands
	// IT_AddCommand("float", 0, "", &print_val, "print subscriber value", 128, 4);


  // Launch the OS
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}

// Kill thread by name
