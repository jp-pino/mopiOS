#include "ros.h"
#include "Vector3.h"
#include "Twist.h"
#include "Heap.h"
#include "ST7735.h"

char ROS_TWIST_MD5[] = "9f195f881246fdfa2798d1d3eebca84a";
char ROS_TWIST_MSG[] = "geometry_msgs/Twist";

char* ROS_TwistMD5(void) {
	return ROS_TWIST_MD5;
}

char* ROS_TwistMSG(void) {
	return ROS_TWIST_MSG;
}

unsigned char* ROS_TwistSerialize(rostwist_t *rostwist) {
	unsigned char *data, *linear, *angular;

	// Serialize Vectors
	linear = ROS_Vector3Serialize(rostwist->linear);
	if (linear == 0)
		return 0;
	angular = ROS_Vector3Serialize(rostwist->angular);
	if (angular == 0) {
		Heap_Free(linear);
		return 0;
	}

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_TWIST_LEN);
	if (data == 0) {
		Heap_Free(linear);
		Heap_Free(angular);
		return 0;
	}

	// Copy vector data
	for (int i = 0; i < ROS_VECTOR3_LEN; i++)
		data[i] = linear[i];
	for (int i = 0; i < ROS_VECTOR3_LEN; i++)
		data[i+ROS_VECTOR3_LEN] = angular[i];

	Heap_Free(linear);
	Heap_Free(angular);

	return data;
}

rostwist_t* ROS_TwistDeserialize(rospacket_t *message) {
	rostwist_t *parse;
	unsigned long long serial;
	unsigned char *aux = message->data;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rostwist_t));
	if (parse == 0)
		return 0;

	parse->linear = Heap_Malloc(sizeof(rosvector3_t));
	if (parse->linear == 0) {
		Heap_Free(parse);
		return 0;
	}

	parse->angular = Heap_Malloc(sizeof(rosvector3_t));
	if (parse->angular == 0) {
		Heap_Free(parse->linear);
		Heap_Free(parse);
		return 0;
	}

	// Parse message
	if (message->length == ROS_TWIST_LEN) {
		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->linear->x = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->linear->y = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->linear->z = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->angular->x = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->angular->y = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->angular->z = *((double*)&(serial));

		aux = 0;

		return parse;
	}

	// Error, free memory
	ROS_Vector3Free(parse->linear);
	ROS_Vector3Free(parse->angular);
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_TwistFree(rostwist_t *rostwist) {
	ROS_Vector3Free(rostwist->linear);
	ROS_Vector3Free(rostwist->angular);
	Heap_Free(rostwist);
}
