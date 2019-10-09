#ifndef __TWIST_H__
#define __TWIST_H__

#include "Vector3.h"
#define ROS_TWIST_LEN sizeof(rosvector3_t)*2

#include "ros.h"

typedef struct {
	rosvector3_t *linear;
	rosvector3_t *angular;
} rostwist_t;


char* ROS_TwistMD5(void);
char* ROS_TwistMSG(void);

unsigned char* ROS_TwistSerialize(rostwist_t *rostwist);
rostwist_t* ROS_TwistDeserialize(rospacket_t *message);

// Deallocate memory
void ROS_TwistFree(rostwist_t *rostwist);


#endif
