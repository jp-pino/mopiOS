#include "tm4c123gh6pm.h"
#include "OS.h"
#include "ST7735.h"
#include "Speed.h"
#include "Motor.h"
#include "mailbox.h"

// Speed and reference structs
motor_ctrl_data_t motor_ctrl_data[2];
volatile int sign[2] = {1, 1};

volatile float speeds[2];

// Mailboxes
float_mailbox_t motor_speed_l, motor_speed_r;

// Interrupt handler for GPIO Port D (PD1, PD3)
void GPIOPortD_Handler(void) {
  // Check for PD1
  if ((GPIO_PORTD_RIS_R & 0x02) == 0x02) {
    GPIO_PORTD_ICR_R |= 0x02;
    GPIO_PORTD_IM_R &= ~0x02;
		if ((GPIO_PORTD_DATA_R & 0x02) == 0x02) {
			if ((GPIO_PORTD_DATA_R & 0x01) == 0x01) {
				sign[SPEED_MOTOR_RIGHT] = -1;
				PF1 |= 0x02;
			} else {
				sign[SPEED_MOTOR_RIGHT] = 1;
				PF1 &= ~0x02;
			}
		}
		GPIO_PORTD_IM_R |= 0x02;
  }

	// Check for PD3
  if ((GPIO_PORTD_RIS_R & 0x08) == 0x08) {
    GPIO_PORTD_ICR_R |= 0x08;
    GPIO_PORTD_IM_R &= ~0x08;
		if ((GPIO_PORTD_DATA_R & 0x08) == 0x08) {
			if ((GPIO_PORTD_DATA_R & 0x04) == 0x04) {
				sign[SPEED_MOTOR_LEFT] = 1;
				PF2 &= ~0x4;
			} else {
				sign[SPEED_MOTOR_LEFT] = -1;
				PF2 |= 0x4;
			}
		}
		GPIO_PORTD_IM_R |= 0x08;
  }
}

// Configure speed
void Motor_SetSpeed(int motor_id, float speed) {
	if (speed != motor_ctrl_data[motor_id].speed) {
		motor_ctrl_data[motor_id].speed = speed;
		motor_ctrl_data[motor_id].integral = 0;
	}
}

void motor_r_speed(void) {
	long start;
	static long last = 0;
	float speed;
	while(1) {
		start = OS_Time();

		// Disable timers
		WTIMER2_CTL_R &= ~TIMER_CTL_TAEN;

		// Calculate speed
		speed = (2.0f * SPEED_PI * ((float)WTIMER2_TAR_R)) / (SPEED_N * ((float)(OS_Time() - last)/1000.0f));
		// speed = (2.0f * SPEED_PI * ((float)WTIMER2_TAR_R)) / (SPEED_N * ((float)(SPEED_PID_PERIOD)/1000.0f));
		speed = SPEED_GEAR_RELATION * speed;
		// Tangential
		speed = SPEED_WHEEL_RADIUS * speed * 4.0f;
		// Sign
		speed = speed * ((float)sign[SPEED_MOTOR_RIGHT]);

		// Send speed data to PID
		MailBox_Float_Send(&motor_speed_r, speed);
		// speeds[SPEED_MOTOR_RIGHT] = speed;

		// Reset timers
		WTIMER2_TAV_R = 0;
		last = OS_Time();
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
		// Angular
		speed = (2.0f * SPEED_PI * ((float)WTIMER3_TAR_R)) / (SPEED_N * ((float)(OS_Time() - last)/1000.0f));
		speed = SPEED_GEAR_RELATION * speed;
		// Tangential
		speed = SPEED_WHEEL_RADIUS * speed * 4.0f;
		// Sign
		speed = speed * ((float)sign[SPEED_MOTOR_LEFT]);

		// Send speed data to PID
		MailBox_Float_Send(&motor_speed_l, speed);
		// speeds[SPEED_MOTOR_LEFT] = speed;

		// Reset timers
		WTIMER3_TAV_R = 0;
		last = OS_Time();
		WTIMER3_CTL_R |= TIMER_CTL_TAEN;

		// Sleep for required time
		OS_Sleep(SPEED_PID_PERIOD - (OS_Time() - start));
	}
}

void motor_r_pid(void) {
	long start;
	float speed_r, speed_c, speed_p, error;
	float pwm;

	static long last = 0;

	// Initialize variables
	motor_ctrl_data[SPEED_MOTOR_RIGHT].speed = 0.0f;
	motor_ctrl_data[SPEED_MOTOR_RIGHT].speed_p = 0;
	motor_ctrl_data[SPEED_MOTOR_RIGHT].integral = 0;
	while(1) {
		// Store start time
		start = OS_Time();


		// Read Current Speed
		speed_c = MailBox_Float_Recv(&motor_speed_r);
		// speed_c = speeds[SPEED_MOTOR_RIGHT];
		// Read previous speed
		speed_p = motor_ctrl_data[SPEED_MOTOR_RIGHT].speed_p;
		// Read Reference Speed
		speed_r = motor_ctrl_data[SPEED_MOTOR_RIGHT].speed;


		// Calculate Error
		error = speed_r - speed_c;


		// Calculate PID

		// Proportional
		pwm = error * K_P;

		// Integral
		motor_ctrl_data[SPEED_MOTOR_RIGHT].integral += error * (SPEED_PID_PERIOD);
		// Limit Integral Error
		if (motor_ctrl_data[SPEED_MOTOR_RIGHT].integral > MOTOR_PWM_PERIOD) {
			motor_ctrl_data[SPEED_MOTOR_RIGHT].integral = MOTOR_PWM_PERIOD;
		} else if (motor_ctrl_data[SPEED_MOTOR_RIGHT].integral < -MOTOR_PWM_PERIOD) {
			motor_ctrl_data[SPEED_MOTOR_RIGHT].integral = -MOTOR_PWM_PERIOD;
		}
		pwm += motor_ctrl_data[SPEED_MOTOR_RIGHT].integral * K_I;

		// Derivative
		pwm += K_D * (speed_c - speed_p) / (float)(SPEED_PID_PERIOD);



		// Check if it maxed out
		if (pwm > MOTOR_PWM_PERIOD) {
			pwm = MOTOR_PWM_PERIOD;
		} else if (pwm < -MOTOR_PWM_PERIOD) {
			pwm = -MOTOR_PWM_PERIOD;
		}


		// Set new motor speed
		// DRV8848_RightDuty((long)pwm);
		if (pwm > 0) {
	 		DRV8848_RightInit(MOTOR_PWM_PERIOD, pwm, FORWARD);
		} else if (pwm < 0) {
	 		DRV8848_RightInit(MOTOR_PWM_PERIOD, -pwm, BACKWARD);
		} else {
	 		DRV8848_RightInit(MOTOR_PWM_PERIOD, 0, COAST);
		}

		// Store new previous speed
		motor_ctrl_data[SPEED_MOTOR_RIGHT].speed_p = speed_c;
		last = OS_Time();

		// Sleep for required time
		// OS_Sleep(SPEED_PID_PERIOD - (OS_Time() - start));
	}
}

void motor_l_pid(void) {
	long start;
	float speed_r, speed_c, speed_p, error;
	float pwm;

	static long last = 0;

	// Initialize variables
	motor_ctrl_data[SPEED_MOTOR_LEFT].speed = 0.0f;
	motor_ctrl_data[SPEED_MOTOR_LEFT].speed_p = 0;
	motor_ctrl_data[SPEED_MOTOR_LEFT].integral = 0;
	while(1) {
		// Store start time
		start = OS_Time();


		// Read Current Speed
		speed_c = MailBox_Float_Recv(&motor_speed_l);
		// speed_c = speeds[SPEED_MOTOR_LEFT];
		// Read previous speed
		speed_p = motor_ctrl_data[SPEED_MOTOR_LEFT].speed_p;
		// Read Reference Speed
		speed_r = motor_ctrl_data[SPEED_MOTOR_LEFT].speed;


		// Calculate Error
		error = speed_r - speed_c;


		// Calculate PID

		// Proportional
		pwm = error * K_P;

		// Integral
		motor_ctrl_data[SPEED_MOTOR_LEFT].integral += error * (SPEED_PID_PERIOD);

		// Limit integral error
		if (motor_ctrl_data[SPEED_MOTOR_LEFT].integral > MOTOR_PWM_PERIOD) {
			motor_ctrl_data[SPEED_MOTOR_LEFT].integral = MOTOR_PWM_PERIOD;
		} else if (motor_ctrl_data[SPEED_MOTOR_LEFT].integral < -MOTOR_PWM_PERIOD) {
			motor_ctrl_data[SPEED_MOTOR_LEFT].integral = -MOTOR_PWM_PERIOD;
		}
		pwm += motor_ctrl_data[SPEED_MOTOR_LEFT].integral * K_I;

		// Derivative
		pwm += K_D * (speed_c - speed_p) / (float)(SPEED_PID_PERIOD);


		// Check if it maxed out
		if (pwm > MOTOR_PWM_PERIOD) {
			pwm = MOTOR_PWM_PERIOD;
		} else if (pwm < -MOTOR_PWM_PERIOD) {
			pwm = -MOTOR_PWM_PERIOD;
		}

		// Set new motor speed
		// DRV8848_RightDuty((long)pwm);
		if (pwm > 0) {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, pwm, FORWARD);
		} else if (pwm < 0) {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, -pwm, BACKWARD);
		} else {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, 0, COAST);
		}

		// Store new previous speed
		motor_ctrl_data[SPEED_MOTOR_LEFT].speed_p = speed_c;
		last = OS_Time();

		// Sleep for required time
		// OS_Sleep(SPEED_PID_PERIOD - (OS_Time() - start));
	}
}

// WT2CCP0 -> PD0 -> ENC1A
// WT2CCP1 -> PD1 -> ENC1B
// WT3CCP0 -> PD2 -> ENC2A
// WT3CCP1 -> PD3 -> ENC2B

void motors_print(void) {
	static int i = 0;
	while(1) {
		ST7735_Message(ST7735_DISPLAY_TOP, 1, "Motor R:", motor_ctrl_data[1].speed_p * 100);
		ST7735_Message(ST7735_DISPLAY_TOP, 2, "PWM R:", motor_ctrl_data[1].speed_p * K_P * 100);
		ST7735_Message(ST7735_DISPLAY_TOP, 3, "Error:", (motor_ctrl_data[1].speed_p - motor_ctrl_data[1].speed) * 100);
		ST7735_Message(ST7735_DISPLAY_TOP, 4, "Reference:", motor_ctrl_data[1].speed * 100);
		ST7735_Message(ST7735_DISPLAY_TOP, 5, "Motor wL:", motor_ctrl_data[1].speed_p / SPEED_WHEEL_RADIUS * 30.0f / SPEED_PI);

		ST7735_Message(ST7735_DISPLAY_BOTTOM, 1, "Motor L:", motor_ctrl_data[0].speed_p * 100);
		ST7735_Message(ST7735_DISPLAY_BOTTOM, 2, "PWM L:", motor_ctrl_data[0].speed_p * K_P * 100);
		ST7735_Message(ST7735_DISPLAY_BOTTOM, 3, "Error:", (motor_ctrl_data[0].speed_p - motor_ctrl_data[0].speed) * 100);
		ST7735_Message(ST7735_DISPLAY_BOTTOM, 4, "Reference:", motor_ctrl_data[0].speed * 100);
		ST7735_Message(ST7735_DISPLAY_BOTTOM, 5, "Motor wL:", motor_ctrl_data[0].speed_p / SPEED_WHEEL_RADIUS * 30.0f / SPEED_PI);
		ST7735_Message(ST7735_DISPLAY_BOTTOM, 6, "I:", i++);
		OS_Sleep(1000);
	}
}

void motors_test(void) {
	// OS_Sleep(5000);
	// Motor_SetSpeed(SPEED_MOTOR_LEFT, -0.2f);
	// Motor_SetSpeed(SPEED_MOTOR_RIGHT, 0.2f);
	// OS_Kill();
	while(1) {
		Motor_SetSpeed(SPEED_MOTOR_LEFT, -0.35f);
		Motor_SetSpeed(SPEED_MOTOR_RIGHT, 0.35f);
		OS_Sleep(10000);
		Motor_SetSpeed(SPEED_MOTOR_LEFT, -0.15f);
		Motor_SetSpeed(SPEED_MOTOR_RIGHT, 0.15f);
		OS_Sleep(10000);
		Motor_SetSpeed(SPEED_MOTOR_LEFT, 0.0f);
		Motor_SetSpeed(SPEED_MOTOR_RIGHT, 0.0f);
		OS_Sleep(10000);
		Motor_SetSpeed(SPEED_MOTOR_LEFT, 0.15f);
		Motor_SetSpeed(SPEED_MOTOR_RIGHT, -0.15f);
		OS_Sleep(10000);
		Motor_SetSpeed(SPEED_MOTOR_LEFT, 0.35f);
		Motor_SetSpeed(SPEED_MOTOR_RIGHT, -0.35f);
		OS_Sleep(10000);
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
	// Make PD0-3 in
	GPIO_PORTD_DIR_R &= ~0x0F;
	// Enable alt funct on PD0, PD2
	GPIO_PORTD_AFSEL_R |= 0x05;
	GPIO_PORTD_AFSEL_R &= ~0x0A;
	// Enable digital I/O on PD0-3
	GPIO_PORTD_DEN_R |= 0x0F;
	// Enable interrupts on PD1, PD3
	GPIO_PORTD_IM_R |= 0x0A;
	GPIO_PORTD_IEV_R |= 0x0A;
	// Set interrupt priority to 3
  NVIC_PRI0_R = (NVIC_PRI0_R & 0x00FFFFFF) | (0 << 29);
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
	MailBox_Float_Init(&motor_speed_l);
	MailBox_Float_Init(&motor_speed_r);

	// Add Threads
	OS_AddThread("m_l_speed", &motor_l_speed, 128, 3);
	OS_AddThread("m_r_speed", &motor_r_speed, 128, 3);
	OS_AddThread("motor_l_pid", &motor_l_pid, 128, 2);
	OS_AddThread("motor_r_pid", &motor_r_pid, 128, 2);

	OS_AddThread("m_print", &motors_print, 128, 5);
	// OS_AddThread("m_test", &motors_test, 128, 5);
}
