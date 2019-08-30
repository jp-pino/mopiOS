#ifndef __FLOAT64_H__
#define __FLOAT64_H__

#include "ros.h"

char ROS_DOUBLE_MD5[] = "8b94c1b53db61fb6aed406028ad6332a";

typedef struct {
	double data;
} rosdouble_t;

rosdouble_t* DoubleParse(rospacket_t *message);


#endif
