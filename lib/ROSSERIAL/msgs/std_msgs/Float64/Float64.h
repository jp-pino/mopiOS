#ifndef __FLOAT64_H__
#define __FLOAT64_H__

#define ROS_FLOAT64_LEN sizeof(double)

#include "ros.h"

typedef struct {
	double data;
} rosfloat64_t;

char* ROS_Float64MD5(void);
char* ROS_Float64MSG(void);

unsigned char* ROS_Float64Serialize(rosfloat64_t *rosfloat64);
rosfloat64_t* ROS_Float64Deserialize(rospacket_t *message);

// Deallocate memory
void ROS_Float64Free(rosfloat64_t *rosfloat64);


#endif
