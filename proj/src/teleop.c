#include "teleop.h"
#include "ros.h"
#include "Twist.h"
#include "Motor.h"
#include "Speed.h"

// Linear.x> 0 and angular.z =0 -> MOVE STRAIGHT TO THE FRONT
// Linear.x <0 and angular.z=0 -> Move to the back
// Linear.x = 0 and angular.z>0 -> rotate about its own axis counter clock wise.
// Linear.x = 0 and angular.z <0 -> rotate about its own axis clockwise.

int teleop_enable = 1;

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
					Motor_SetSpeed(SPEED_MOTOR_LEFT, twistmessage->linear->x - twistmessage->angular->z * SPEED_WHEEL_DISTANCE / 2.0f);
					Motor_SetSpeed(SPEED_MOTOR_RIGHT, twistmessage->angular->z * SPEED_WHEEL_DISTANCE / 2.0f -  twistmessage->linear->x);
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
