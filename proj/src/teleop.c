#include "teleop.h"
#include "ros.h"
#include "Twist.h"
#include "Motor.h"

// Linear.x> 0 and angular.z =0 -> MOVE STRAIGHT TO THE FRONT
// Linear.x <0 and angular.z=0 -> Move to the back
// Linear.x = 0 and angular.z>0 -> rotate about its own axis counter clock wise.
// Linear.x = 0 and angular.z <0 -> rotate about its own axis clockwise.

int teleop_enable = 0;

void ROSTeleop_Disable(void) {
	teleop_enable = 0;
}

void ROSTeleop_Enable(void) {
	teleop_enable = 1;
}

void ROSTeleop(void) {
	rospacket_t *message;
	rostwist_t *twistmessage;
	ROS_SubscriberInit(ROS_FindSubscriber());
	while(1) {
		// Wait for message
		message = ROS_MailBox_Recv(&(ROS_FindSubscriber()->mailbox));
		if (message) {
			// Deserialize data
			twistmessage = ROS_TwistDeserialize(message);
			if (twistmessage) {
				// Send data to main
				if (teleop_enable) {
					if (twistmessage->linear->x > 0 && twistmessage->angular->z == 0) {
						DRV8848_RightInit(MOTOR_PWM_PERIOD,
							twistmessage->linear->x/100.0f * MOTOR_PWM_PERIOD, FORWARD);
						DRV8848_LeftInit(MOTOR_PWM_PERIOD,
							twistmessage->linear->x/100.0f * MOTOR_PWM_PERIOD, FORWARD);
					} else if (twistmessage->linear->x < 0 && twistmessage->angular->z == 0) {
						DRV8848_RightInit(MOTOR_PWM_PERIOD,
							twistmessage->linear->x/100.0f * MOTOR_PWM_PERIOD, BACKWARD);
						DRV8848_LeftInit(MOTOR_PWM_PERIOD,
							twistmessage->linear->x/100.0f * MOTOR_PWM_PERIOD, BACKWARD);
					} else if (twistmessage->linear->x == 0 && twistmessage->angular->z > 0) {
						DRV8848_RightInit(MOTOR_PWM_PERIOD,
							twistmessage->angular->z/100.0f * MOTOR_PWM_PERIOD, FORWARD);
						DRV8848_LeftInit(MOTOR_PWM_PERIOD,
							twistmessage->angular->z/100.0f * MOTOR_PWM_PERIOD, BACKWARD);
					} else if (twistmessage->linear->x == 0 && twistmessage->angular->z < 0) {
						DRV8848_RightInit(MOTOR_PWM_PERIOD,
							twistmessage->angular->z/100.0f * MOTOR_PWM_PERIOD, BACKWARD);
						DRV8848_LeftInit(MOTOR_PWM_PERIOD,
							twistmessage->angular->z/100.0f * MOTOR_PWM_PERIOD, FORWARD);
					}
				}
				// Deallocate data
				ROS_TwistFree(twistmessage);
			}
			// Deallocate packet
			ROS_PacketFree(message);
		}
	}
}

void Teleop_Init(void) {
	ROS_AddSubscriber("cmd_vel", ROS_TwistMSG(), ROS_TwistMD5(), &ROSTeleop, 512, 4);
}
