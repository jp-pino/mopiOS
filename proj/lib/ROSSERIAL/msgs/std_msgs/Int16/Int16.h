#ifndef __INT32_H__
#define __INT32_H__

#define ROS_INT16_LEN sizeof(short)

#include "ros.h"

typedef struct {
	short data;
} rosint16_t;


char* ROS_Int16MD5(void);
char* ROS_Int16MSG(void);

unsigned char* ROS_Int16Serialize(rosint16_t *rosint16);
rosint16_t* ROS_Int16Deserialize(rospacket_t *message);

// Deallocate memory
void ROS_Int16Free(rosint16_t *rosint16);


#endif
