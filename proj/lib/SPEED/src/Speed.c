#include "OS.h"
#include "Speed.h"
#include "Motor.h"


// Mailboxes
void mailbox_left_recv(void) {

}

void mailbox_left_send(void) {

}

void mailbox_right_recv(void) {

}

void mailbox_right_send(void) {

}

void motor_l_speed(void) {
	while(1) {

	}
}

void motor_r_speed(void) {
	while(1) {

	}
}

void motor_l_pid(void) {
	// Initialize variables
	while(1) {

	}
}

void motor_r_pid(void) {
	long start;
	int error;

	// Initialize variables
	while(1) {
		// Store start time
		start = OS_Time();
		// Read Current Speed
		speed_c = mailbox_right_recv();
		// Read User Speed (Reference)
		speed_r = mailbox_right_recv();
		// Calculate Error
		error = speed_r - speed_c;


		// Calculate PID
		// Proportional
		pwm = speed_c * K_P_RIGHT;
		// Check if it maxed out
		if (pwm > MOTOR_PWM_PERIOD) {
			pwm = MOTOR_PWM_PERIOD;
		} else if (pwm < -MOTOR_PWM_PERIOD) {
			pwm = -MOTOR_PWM_PERIOD
		}

		// Integral
		error_integral += (speed_c - speed_p) * SPEED_PID_PERIOD;
		pwm += error_integral * K_I_RIGHT;

		// Derivative
		pwm +=



		// Set new motor speed
		if (pwm > 2) {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, pwm, FORWARD);
		} else if (pwm < -2) {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, -pwm, BACKWARD);
		} else {
			DRV8848_LeftInit(MOTOR_PWM_PERIOD, 0, COAST);
		}

		// Store new previous speed
		speed_p = speed_c;

		// Sleep for required time
		OS_Sleep(SPEED_PID_PERIOD - (OS_Time() - start));
	}
}

void Speed_Init(void) {
	// Initialize Mailboxes

	// Add Threads
	OS_AddThread("motor_l_speed", &motor_l_speed, 128, 3);
	OS_AddThread("motor_l_pid", &motor_l_pid, 128, 3);
	OS_AddThread("motor_r_speed", &motor_r_speed, 128, 3);
	OS_AddThread("motor_r_pid", &motor_r_pid, 128, 3);
}
