#include "tm4c123gh6pm.h"
#include "OS.h"
#include "ST7735.h"
#include "Speed.h"
#include "Motor.h"
#include "mailbox.h"

// Speed and reference structs
motor_ctrl_data_t motor_ctrl_data[2];
volatile int sign[] = {0, 0};

// Mailboxes
float_mailbox_t motor_speed[2];

// Interrupt handler for GPIO Port D (PD1, PD3)
void GPIOPortD_Handler(void) {
  // Check for PD1
  if ((GPIO_PORTD_RIS_R & 0x02) == 0x02) {
    GPIO_PORTD_ICR_R |= 0x02;
    GPIO_PORTD_IM_R &= ~0x02;
    if ((GPIO_PORTD_DATA_R & 0x01) == 0x01) {
			sign[SPEED_MOTOR_RIGHT] = 1;
		} else {
			sign[SPEED_MOTOR_RIGHT] = -1;
		}
		GPIO_PORTD_IM_R |= 0x02;
  }
	// Check for PD3
  if ((GPIO_PORTD_RIS_R & 0x08) == 0x08) {
    GPIO_PORTD_ICR_R |= 0x08;
    GPIO_PORTD_IM_R &= ~0x08;
    if ((GPIO_PORTD_DATA_R & 0x04) == 0x04) {
			sign[SPEED_MOTOR_LEFT] = -1;
		} else {
			sign[SPEED_MOTOR_LEFT] = 1;
		}
		GPIO_PORTD_IM_R |= 0x08;
  }
}

// Configure speed
void Motor_SetSpeed(int motor_id, float speed) {
	motor_ctrl_data[motor_id].speed = speed;
}

void motor_r_speed(void) {
	long start;
	float speed;
	while(1) {
		start = OS_Time();

		// Disable timers
		WTIMER2_CTL_R &= ~TIMER_CTL_TAEN;

		// Calculate speed
		speed = SPEED_WHEEL_RADIUS * SPEED_GEAR_RELATION * 2.0f * SPEED_PI * (float)WTIMER2_TAR_R / ((SPEED_N/1000.0f) * SPEED_PID_PERIOD);

		// Send speed data to PID
		MailBox_Float_Send(&motor_speed[SPEED_MOTOR_RIGHT], speed * sign[SPEED_MOTOR_RIGHT]);

		// Reset timers
		WTIMER2_TAV_R = 0;
		WTIMER2_CTL_R |= TIMER_CTL_TAEN;

		// Sleep for required time
		OS_Sleep(SPEED_PID_PERIOD - (OS_Time() - start));
	}
}

void motor_l_speed(void) {
	long start;
	static long last = 0;
	float speed;
	while(1) {
		start = OS_Time();

		// Disable timers
		WTIMER3_CTL_R &= ~TIMER_CTL_TAEN;

		// Calculate speed
		speed = SPEED_WHEEL_RADIUS * SPEED_GEAR_RELATION * 2.0f * SPEED_PI * (float)WTIMER3_TAR_R / ((SPEED_N/1000.0f) * (OS_Time() - last));

		// Send speed data to PID
		MailBox_Float_Send(&motor_speed[SPEED_MOTOR_LEFT], speed * sign[SPEED_MOTOR_LEFT]);

		// Reset timers
		WTIMER3_TAV_R = 0;
		last = OS_Time();
		WTIMER3_CTL_R |= TIMER_CTL_TAEN;

		// Sleep for required time
		OS_Sleep(SPEED_PID_PERIOD - (OS_Time() - start));
	}
}

// void motor_r_speed(void) {
// 	long start;
// 	ST7735_Message(ST7735_DISPLAY_BOTTOM, 7, "Test:", 1.3);
// 	while(1) {
// 		start = OS_Time();
//
// 		// Send speed data to PID
// 		while(WTIMER2_TAR_R == WTIMER2_TBR_R);
//
// 		// mailbox_l_send();
// 		// ST7735_Message(ST7735_DISPLAY_BOTTOM, 3, "Motor RA:", (float)(6.5f * SPEED_GEAR_RELATION * 2.0f * SPEED_PI * (float)WTIMER2_TAR_R / ((float)(SPEED_N/1000.0f) * SPEED_PID_PERIOD)));
// 		// WTIMER2_TAV_R = 0;
// 		// ST7735_Message(ST7735_DISPLAY_BOTTOM, 4, "Motor RB:", (float)(6.5f * SPEED_GEAR_RELATION * 2.0f * SPEED_PI * (float)WTIMER2_TBR_R / ((float)(SPEED_N/1000.0f) * SPEED_PID_PERIOD)));
// 		// WTIMER2_TBV_R = 0;
//
// 		// Sleep for required time
// 		OS_Sleep(SPEED_PID_PERIOD - (OS_Time() - start));
// 	}
// }

// void motor_r_pid(void) {
// 	long start;
// 	int error;
//
// 	// Initialize variables
// 	while(1) {
// 		// Store start time
// 		start = OS_Time();
// 		// Read Current Speed
// 		speed_c = 6.5f * SPEED_GEAR_RELATION * 2.0f * SPEED_PI * (float)WTIMER2_TAR_R / ((float)(SPEED_N/1000.0f) * SPEED_PID_PERIOD);
// 		// Read User Speed (Reference)
// 		speed_r = mailbox_r_recv();
// 		// Calculate Error
// 		error = speed_r - speed_c;
//
//
// 		// Calculate PID
// 		// Proportional
// 		pwm = speed_c * K_P_RIGHT;
// 		// Check if it maxed out
// 		if (pwm > MOTOR_PWM_PERIOD) {
// 			pwm = MOTOR_PWM_PERIOD;
// 		} else if (pwm < -MOTOR_PWM_PERIOD) {
// 			pwm = -MOTOR_PWM_PERIOD
// 		}
//
// 		// Integral
// 		error_integral += (speed_c - speed_p) * SPEED_PID_PERIOD;
// 		pwm += error_integral * K_I_RIGHT;
//
// 		// Derivative
// 		pwm +=
//
//
//
// 		// Set new motor speed
// 		if (pwm > 2) {
// 			DRV8848_RightInit(MOTOR_PWM_PERIOD, pwm, FORWARD);
// 		} else if (pwm < -2) {
// 			DRV8848_RightInit(MOTOR_PWM_PERIOD, -pwm, BACKWARD);
// 		} else {
// 			DRV8848_RightInit(MOTOR_PWM_PERIOD, 0, COAST);
// 		}
//
// 		// Store new previous speed
// 		speed_p = speed_c;
//
// 		// Sleep for required time
// 		OS_Sleep(SPEED_PID_PERIOD - (OS_Time() - start));
// 	}
// }

// WT2CCP0 -> PD0 -> ENC1A
// WT2CCP1 -> PD1 -> ENC1B
// WT3CCP0 -> PD2 -> ENC2A
// WT3CCP1 -> PD3 -> ENC2B

void motors_print(void) {
	while(1) {
		ST7735_Message(ST7735_DISPLAY_BOTTOM, 0, "Motor L:", MailBox_Float_Recv(&motor_speed[SPEED_MOTOR_LEFT])*100);
		ST7735_Message(ST7735_DISPLAY_BOTTOM, 1, "Motor R:", MailBox_Float_Recv(&motor_speed[SPEED_MOTOR_RIGHT])*100);
	}
}

void Speed_Init(void) {
	// Initialize
  SYSCTL_RCGCWTIMER_R |= 0x04;						// activate timer2
	SYSCTL_RCGCWTIMER_R |= 0x08;						// activate timer3
  SYSCTL_RCGCGPIO_R |= 0x08; 							// activate port D
  while((SYSCTL_PRGPIO_R&0x0008) == 0){};	// ready?


	// Configure Port D (0-3)
	// Unlock GPIO Port D Commit Register
	GPIO_PORTD_LOCK_R = GPIO_LOCK_KEY;
	GPIO_PORTD_CR_R |= 0x0F;
	// Disable analog functionality on PD0-3
	GPIO_PORTD_AMSEL_R &= ~0x0F;
	// Configure PD0, PD2 as WTCCP
	GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0xFFFF0000) + 0x00000707;
	// Make PD0-3 in (built-in buttons)
	GPIO_PORTD_DIR_R &= ~0x0F;
	// Enable alt funct on PD0, PD2
	GPIO_PORTD_AFSEL_R |= 0x05;
	GPIO_PORTD_AFSEL_R &= ~0x0A;
	// Enable digital I/O on PD0-3
	GPIO_PORTD_DEN_R |= 0x0F;
	// Enable interrupts on PD1, PD3
	GPIO_PORTD_IM_R |= 0x0A;
	// Set interrupt priority to 7
  NVIC_PRI0_R = (NVIC_PRI0_R & 0x00FFFFFF) | (7 << 29);
  NVIC_EN0_R |= 1 << 3;
	// Lock GPIO Port D
	GPIO_PORTD_LOCK_R = 0x00;



	// Initialize Timers in Edge-Detect Mode WT2 & WT3
	// 1. Ensure the timer is disabled
	WTIMER2_CTL_R &= ~TIMER_CTL_TAEN;

	WTIMER3_CTL_R &= ~TIMER_CTL_TAEN;

	// 2. Write the GPTM Configuration (GPTMCFG)
	// 		register with a value of 0x0000.0004
	WTIMER2_CFG_R = 0x00000004;

	WTIMER3_CFG_R = 0x00000004;

	// 3. In the GPTM Timer Mode (GPTMTnMR) register,
	// 		write the TnCMR field to 0x0 and the TnMR field to 0x3
	WTIMER2_TAMR_R = 0x13;

	WTIMER3_TAMR_R = 0x13;

	// 4. Configure the type of event(s) that the timer captures by writing
	//		the TnEVENT field of the GPTM Control (GPTMCTL) register.
	WTIMER2_CTL_R &= ~(0x0C);

	WTIMER3_CTL_R &= ~(0x0C);

	// 5. Program registers for up-count mode
	WTIMER2_TAILR_R = 0xFFFFFFFF;
	WTIMER2_TAPR_R = 0xFFFF;
	WTIMER2_TAMATCHR_R = 0xFFFFFFFF;
	WTIMER2_TAPMR_R = 0xFFFF;

	WTIMER3_TAILR_R = 0xFFFFFFFF;
	WTIMER3_TAPR_R = 0xFFFF;
	WTIMER3_TAMATCHR_R = 0xFFFFFFFF;
	WTIMER3_TAPMR_R = 0xFFFF;

	// 6. If interrupts set the CnMIM bit in the GPTM
	//		Interrupt Mask (GPTMIMR) register

	// 7. Set the TnEN bit in the GPTMCTL register to enable the timer
	//    and begin waiting for edge events
	WTIMER2_CTL_R |= TIMER_CTL_TAEN;

	WTIMER3_CTL_R |= TIMER_CTL_TAEN;

	// 8. Poll the CnMRIS bit in the GPTMRIS register or wait for the
	//		interrupt to be generated (if enabled). In both cases, the status flags
	//    are cleared by writing a 1 to the CnMCINT bit of the GPTMICR register.


	// Initialize Mailboxes
	MailBox_Float_Init(&motor_speed[0]);
	MailBox_Float_Init(&motor_speed[1]);

	// Add Threads
	OS_AddThread("m_l_speed", &motor_l_speed, 128, 3);
	OS_AddThread("m_r_speed", &motor_r_speed, 128, 3);
	OS_AddThread("m_r_speed", &motors_print, 128, 5);
	// OS_AddThread("motor_l_pid", &motor_l_pid, 128, 3);
	// OS_AddThread("motor_r_pid", &motor_r_pid, 128, 3);
}
