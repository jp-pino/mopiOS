// Differential Steering Joystick Algorithm
// ========================================
//   by Calvin Hass
//   https://www.impulseadventure.com/elec/
//
// Converts a single dual-axis joystick into a differential
// drive motor control, with support for both drive, turn
// and pivot operations.
//

// This algorithm was edited by Juan Pablo Pino for compatibility with mopiOS


#include "tm4c123gh6pm.h"
#include <stdint.h>
#include "OS.h"
#include "Joystick.h"
#include "ADCSWTrigger.h"
#include "Motor.h"
#include "ST7735.h"
#include "teleop.h"
#include "Speed.h"

#define NVIC_EN0_INT2			          0x00000004  // Interrupt 2 enable

#define PF1 (*((volatile uint32_t *)0x40025008))
#define PF2 (*((volatile uint32_t *)0x40025010))

#define PC7 (*((volatile uint32_t *)0x40006200))
#define SW 	0x80 // joystick switch

#define JOYSTICK_MIN 2048.0
#define JOYSTICK_MAX 2047.0

sema_t sw_ready;

int joystick_enable = 1;
int joystick_idle = 0;

// INPUTS
int     nJoyXL;              // Joystick X input                     (-128..+127)
int     nJoyYL;              // Joystick Y input                     (-128..+127)

// SCALED INPUTS
float     nJoyX;              // Joystick X input                     (-128..+127)
float     nJoyY;

// OUTPUTS
int     nMotMixL;           // Motor (left)  mixed output           (-128..+127)
int     nMotMixR;           // Motor (right) mixed output           (-128..+127)

// CONFIG
// - fPivYLimt  : The threshold at which the pivot action starts
//                This threshold is measured in units on the Y-axis
//                away from the X-axis (Y=0). A greater value will assign
//                more of the joystick's range to pivot actions.
//                Allowable range: (0..+127)
float fPivYLimit = 1500.0f;

// TEMP VARIABLES
float   nMotPremixL;    // Motor (left)  premixed output        (-128..+127)
float   nMotPremixR;    // Motor (right) premixed output        (-128..+127)
int     nPivSpeed;      // Pivot Speed                          (-128..+127)
float   fPivScale;      // Balance scale b/w drive and pivot    (   0..1   )

float left, right;

int temp[2];

int abs(int val) {
	if (val >= 0)
	 	return val;
	return -val;
}

void triggerSW(void) {
	// Debounce
  OS_Sleep(50); // sleep 50ms
  if ((PC7) != 0) {
    GPIO_PORTC_IM_R |= SW;
    OS_Kill();
  }
  OS_bWait(&sw_ready);

	// Toggle joystick
	if (joystick_enable) {
		Joystick_Disable();
		ROSTeleop_Enable();
	} else {
		Joystick_Enable();
		ROSTeleop_Disable();
	}

	ST7735_Message(ST7735_DISPLAY_BOTTOM, 0, "STATUS", joystick_enable);

	// Debounce
  OS_Sleep(100); // sleep 100ms
  OS_bSignal(&sw_ready);

	// Clear Interrupt
  GPIO_PORTC_IM_R |= SW;
  OS_Kill();
}

// Interrupt handler for GPIO Port F (PF4 = SW1, PF0 = SW2)
void GPIOPortC_Handler(void) {
  // Check for SW press
  if ((GPIO_PORTC_RIS_R & SW) == SW) {
    GPIO_PORTC_ICR_R |= SW;
    GPIO_PORTC_IM_R &= ~SW;
    if (OS_AddThread("joystick_s", &triggerSW, 128, 0) == 0) {
      GPIO_PORTC_IM_R |= SW;
		}
  }
}

void Joystick_Enable(void) {
	joystick_enable = 1;
}

void Joystick_Disable(void) {
	joystick_enable = 0;
}

void Joystick_Main(void) {
	while(1) {
		ADC_In10(temp);

		nJoyX = (temp[0] - JOYSTICK_MIN)/JOYSTICK_MAX;
		nJoyY = (temp[1] - JOYSTICK_MIN)/JOYSTICK_MAX;

		if (joystick_enable) {
			Motor_SetSpeed(SPEED_MOTOR_LEFT, nJoyY - nJoyX * SPEED_WHEEL_DISTANCE / 2.0f);
			Motor_SetSpeed(SPEED_MOTOR_RIGHT, nJoyX * SPEED_WHEEL_DISTANCE / 2.0f + nJoyY);
		}

		OS_Sleep(150);
	}
}

void Joystick_Init(void) {
	// Initialize port for x and y axes
	ADC_Init10();
	// Initialize port for switch interrupt
	SYSCTL_RCGCGPIO_R |= (1 << 2); // 1) activate Port C
  while ((SYSCTL_PRGPIO_R & (1 << 2)) == 0); // ready?
  // 2a) unlock GPIO Port C Commit Register
  GPIO_PORTC_LOCK_R = GPIO_LOCK_KEY;
	// 2b) enable commit for SW
  GPIO_PORTC_CR_R |= SW;
  // 3) disable analog functionality on SW
  GPIO_PORTC_AMSEL_R &= ~SW;
  // 4) configure SW as GPIO
  GPIO_PORTC_PCTL_R &= ~SW;
	// 5) make SW in
  GPIO_PORTC_DIR_R &= ~SW;
  // 6) disable alt funct on SW
  GPIO_PORTC_AFSEL_R &= ~SW;
	// 7) enable digital I/O on SW
  GPIO_PORTC_DEN_R |= SW;

	// Enable weak pull-up on PF4, PF0
  GPIO_PORTC_PUR_R |= SW;

	// Enable Interrupt
  GPIO_PORTC_IM_R |= SW;

  // Set priority to 3
  NVIC_PRI0_R = (NVIC_PRI0_R & 0xFF00FFFF) | (3 << 21);
  NVIC_EN0_R |= NVIC_EN0_INT2;

	// Initialize semaphore
  OS_InitSemaphore("joy_ready", &sw_ready, 1);

	// Launch Joystick Thread
	OS_AddThread("joystick", &Joystick_Main, 256, 3);
}
