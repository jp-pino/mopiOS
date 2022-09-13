#ifndef __SPEED_H__
#define __SPEED_H__

#define SPEED_PID_L_KC	 1
#define SPEED_PID_PERIOD 80


#define SPEED_N 							64.0f
#define SPEED_PI 							3.14159265f
#define SPEED_GEAR_RELATION 	(1.0f/70.0f)
#define SPEED_WHEEL_RADIUS 		0.06f
#define SPEED_WHEEL_DISTANCE	0.42f

// PID Constants
#define K_P 110000.0f
#define K_I 45.0f
#define K_D 500.0f

#define SPEED_MOTOR_LEFT 	0
#define SPEED_MOTOR_RIGHT 1

typedef struct motor_ctrl_data_t {
	float speed;				// Reference
	float speed_p;			// Previous speed (for differential)
  float integral;			// Integral error
} motor_ctrl_data_t;

void Speed_Init(void);
void Motor_SetSpeed(int motor_id, float speed);


#endif
