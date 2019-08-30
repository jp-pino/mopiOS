#ifndef __BOOL_H__
#define __BOOL_H__

#define ROS_BOOL_LEN 1

#include "ros.h"

typedef struct {
	unsigned char data;
} rosbool_t;


char* ROS_BoolMD5(void);
char* ROS_BoolMSG(void);

unsigned char* ROS_BoolSerialize(rosbool_t *rosbool);
rosbool_t* ROS_BoolDeserialize(rospacket_t *message);

// Deallocate memory
void ROS_BoolFree(rosbool_t *rosbool);


#endif
