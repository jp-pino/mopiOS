#ifndef __SPEED_H__
#define __SPEED_H__

#define SPEED_PID_L_KC	 1
#define SPEED_PID_PERIOD 100


#define SPEED_N 64.0f
#define SPEED_PI 3.14159265f
#define SPEED_GEAR_RELATION ((float)1.0f/70.0f)
#define SPEED_WHEEL_RADIUS 0.065f

enum {
	SPEED_MOTOR_LEFT = 0,
	SPEED_MOTOR_RIGHT = 1
};

typedef struct motor_ctrl_data_t {
  float integral;
	float speed;
	float speed_p;
} motor_ctrl_data_t;

void Speed_Init(void);
void Motor_SetSpeed(int motor_id, float speed);


#endif
