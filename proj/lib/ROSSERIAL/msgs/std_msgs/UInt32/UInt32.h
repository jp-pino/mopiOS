#ifndef __UINT32_H__
#define __UINT32_H__

#define ROS_UINT32_LEN sizeof(unsigned int)

#include "ros.h"

typedef struct {
	unsigned int data;
} rosuint32_t;


char* ROS_UInt32MD5(void);
char* ROS_UInt32MSG(void);

unsigned char* ROS_UInt32Serialize(rosuint32_t *rosuint32);
rosuint32_t* ROS_UInt32Deserialize(rospacket_t *message);

// Deallocate memory
void ROS_UInt32Free(rosuint32_t *rosuint32);


#endif
