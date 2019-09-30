#ifndef __TIME_H__
#define __TIME_H__

#define ROS_TIME_LEN sizeof(unsigned int)

#include "ros.h"

typedef struct {
	unsigned int data;
} rostime_t;


char* ROS_TimeMD5(void);
char* ROS_TimeMSG(void);

unsigned char* ROS_TimeSerialize(rostime_t *rostime);
rostime_t* ROS_TimeDeserialize(rospacket_t *message);

// Deallocate memory
void ROS_TimeFree(rostime_t *rostime);


#endif
