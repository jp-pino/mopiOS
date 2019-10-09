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


#include <stdint.h>
#include "OS.h"
#include "Joystick.h"
#include "ADCSWTrigger.h"
#include "Motor.h"
#include "ST7735.h"


#define JOYSTICK_MIN 2048.0
#define JOYSTICK_MAX 2047.0


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


int abs(int val) {
	if (val >= 0)
	 	return val;
	return -val;
}

int temp[2];

// int exp_scale(int val) {
// 	val = 0.9
// 	return (JOYSTICK_MAX) * (val/(-JOYSTICK_MAX)) * (val/(-JOYSTICK_MAX)) * (val/(-JOYSTICK_MAX)) * (val/(-JOYSTICK_MAX)) * (val/(-JOYSTICK_MAX)) * (val/(-JOYSTICK_MAX)) * (val/(-JOYSTICK_MAX));
// }

void Joystick_Main(void) {
	ADC_Init10();
	while(1) {
		// if (ADC_Init(JOYSTICK_X_ADC))
		// 	continue;
		// OS_Sleep(100);
		// nJoyX = ADC_In() - JOYSTICK_MIN;
		//
		// if ()
		// 	continue;
		// OS_Sleep(100);
		ADC_In10(temp);
		nJoyX = temp[0] - JOYSTICK_MIN;
		nJoyY = temp[1] - JOYSTICK_MIN;
		// nJoyX = -exp_scale(nJoyXL);
		// nJoyY = -exp_scale(nJoyYL);



		// Calculate Drive Turn output due to Joystick X input
		if (nJoyY >= 0) {
		  // Forward
		  nMotPremixL = (nJoyX>=0)? JOYSTICK_MAX : (JOYSTICK_MAX + nJoyX);
		  nMotPremixR = (nJoyX>=0)? (JOYSTICK_MAX - nJoyX) : JOYSTICK_MAX;
		} else {
		  // Reverse
		  nMotPremixL = (nJoyX>=0)? (JOYSTICK_MAX - nJoyX) : JOYSTICK_MAX;
		  nMotPremixR = (nJoyX>=0)? JOYSTICK_MAX : (JOYSTICK_MAX + nJoyX);
		}

		// Scale Drive output due to Joystick Y input (throttle)
		nMotPremixL = nMotPremixL * nJoyY/JOYSTICK_MIN;
		nMotPremixR = nMotPremixR * nJoyY/JOYSTICK_MIN;

		// Now calculate pivot amount
		// - Strength of pivot (nPivSpeed) based on Joystick X input
		// - Blending of pivot vs drive (fPivScale) based on Joystick Y input
		nPivSpeed = nJoyX;
		fPivScale = (abs(nJoyY)>fPivYLimit)? 0.0f : (1.0f - abs(nJoyY)/fPivYLimit);

		// Calculate final mix of Drive and Pivot
		nMotMixL = (1.0f-fPivScale)*nMotPremixL + fPivScale*( nPivSpeed);
		nMotMixR = (1.0f-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed);

		// Convert to Motor PWM range
		if (nMotMixL >= 2) {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, nMotMixL*2.0f/4095.0f * MOTOR_PWM_PERIOD, FORWARD);
			// ST7735_Message(ST7735_DISPLAY_TOP, 0, "MOTOR L: ", nMotMixL*2.0f/4095.0f * MOTOR_PWM_PERIOD);
		} else if (nMotMixL <= -2) {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, -nMotMixL*2.0f/4095.0f * MOTOR_PWM_PERIOD, BACKWARD);
			// ST7735_Message(ST7735_DISPLAY_TOP, 0, "MOTOR L: -", -nMotMixL*2.0f/4095.0f * MOTOR_PWM_PERIOD);
		} else {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, 2, COAST);
			// ST7735_Message(ST7735_DISPLAY_TOP, 0, "MOTOR L: -", 0);
		}

		if (nMotMixR >= 2) {
			DRV8848_RightInit(MOTOR_PWM_PERIOD, nMotMixR*2.0f/4095.0f * MOTOR_PWM_PERIOD, FORWARD);
			// ST7735_Message(ST7735_DISPLAY_TOP, 1, "MOTOR R: ", nMotMixR*2.0f/4095.0f * MOTOR_PWM_PERIOD);
		} else if (nMotMixR <= -2) {
			DRV8848_RightInit(MOTOR_PWM_PERIOD, -nMotMixR*2.0f/4095.0f * MOTOR_PWM_PERIOD, BACKWARD);
			// ST7735_Message(ST7735_DISPLAY_TOP, 1, "MOTOR R: -", -nMotMixR*2.0f/4095.0f * MOTOR_PWM_PERIOD);
		} else {
			DRV8848_RightInit(MOTOR_PWM_PERIOD, 2, COAST);
			// ST7735_Message(ST7735_DISPLAY_TOP, 1, "MOTOR R: -", 0);
		}

		OS_Sleep(150);
	}
}

void Joystick_Print(void) {
	while(1) {
		if (nMotMixL >= 2) {
			ST7735_Message(ST7735_DISPLAY_TOP, 0, "MOTOR L:  ", nMotMixL*2.0f/4095.0f * MOTOR_PWM_PERIOD);
		} else if (nMotMixL <= -2) {
			ST7735_Message(ST7735_DISPLAY_TOP, 0, "MOTOR L: -", -nMotMixL*2.0f/4095.0f * MOTOR_PWM_PERIOD);
		} else {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, 2, COAST);
			ST7735_Message(ST7735_DISPLAY_TOP, 0, "MOTOR L:  ", 0);
		}

		if (nMotMixR >= 2) {
			ST7735_Message(ST7735_DISPLAY_TOP, 1, "MOTOR R:  ", nMotMixR*2.0f/4095.0f * MOTOR_PWM_PERIOD);
		} else if (nMotMixR <= -2) {
			ST7735_Message(ST7735_DISPLAY_TOP, 1, "MOTOR R: -", -nMotMixR*2.0f/4095.0f * MOTOR_PWM_PERIOD);
		} else {
			ST7735_Message(ST7735_DISPLAY_TOP, 1, "MOTOR R:  ", 0);
		}
		OS_Sleep(300);
	}
}

void Joystick_Init(void) {
	OS_AddThread("joystick", &Joystick_Main, 256, 4);
	OS_AddThread("joy_print", &Joystick_Print, 256, 4);
}
