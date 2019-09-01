#ifndef __INT32_H__
#define __INT32_H__

#define ROS_INT32_LEN sizeof(int)

#include "ros.h"

typedef struct {
	int data;
} rosint32_t;


char* ROS_Int32MD5(void);
char* ROS_Int32MSG(void);

unsigned char* ROS_Int32Serialize(rosint32_t *rosint32);
rosint32_t* ROS_Int32Deserialize(rospacket_t *message);

// Deallocate memory
void ROS_Int32Free(rosint32_t *rosint32);


#endif
