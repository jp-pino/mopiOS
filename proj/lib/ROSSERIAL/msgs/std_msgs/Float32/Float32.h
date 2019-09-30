#ifndef __FLOAT32_H__
#define __FLOAT32_H__

#define ROS_FLOAT32_LEN sizeof(float)

#include "ros.h"

typedef struct {
	float data;
} rosfloat32_t;

char* ROS_Float32MD5(void);
char* ROS_Float32MSG(void);

unsigned char* ROS_Float32Serialize(rosfloat32_t *rosfloat32);
rosfloat32_t* ROS_Float32Deserialize(rospacket_t *message);

// Deallocate memory
void ROS_Float32Free(rosfloat32_t *rosfloat32);


#endif
