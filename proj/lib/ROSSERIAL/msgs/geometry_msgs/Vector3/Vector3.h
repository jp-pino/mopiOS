#ifndef __VECTOR3_H__
#define __VECTOR3_H__

#define ROS_VECTOR3_LEN sizeof(double)*3

#include "ros.h"

typedef struct {
	volatile double x;
	volatile double y;
	volatile double z;
} rosvector3_t;


char* ROS_Vector3MD5(void);
char* ROS_Vector3MSG(void);

unsigned char* ROS_Vector3Serialize(rosvector3_t *rosvector3);
rosvector3_t* ROS_Vector3Deserialize(rospacket_t *message);

// Deallocate memory
void ROS_Vector3Free(rosvector3_t *rosvector3);


#endif
