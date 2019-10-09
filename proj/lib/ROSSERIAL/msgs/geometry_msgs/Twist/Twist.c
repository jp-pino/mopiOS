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
	unsigned long long serial, aux;
	unsigned char *data;

	data = message->data;

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
	if (message->length == ROS_TWIST_LEN){
		serial = 0;
		for (int i = 0; i < 8; i++){
			aux = *(data++);
			serial = (aux << (i * 8)) | serial;
		}
		parse->linear->x = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++){
			aux = *(data++);
			serial = (aux << (i * 8)) | serial;
		}
		parse->linear->y = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++){
			aux = *(data++);
			serial = (aux << (i * 8)) | serial;
		}
		parse->linear->z = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++){
			aux = *(data++);
			serial = (aux << (i * 8)) | serial;
		}
		parse->angular->x = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++){
			aux = *(data++);
			serial = (aux << (i * 8)) | serial;
		}
		parse->angular->y = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++){
			aux = *(data++);
			serial = (aux << (i * 8)) | serial;
		}
		parse->angular->z = *((double*)&(serial));

		// serial = (((unsigned long long)(message->data[8])));
		// serial = (((unsigned long long)(message->data[9])) << 8)   | serial;
		// serial = (((unsigned long long)(message->data[10])) << 16) | serial;
		// serial = (((unsigned long long)(message->data[11])) << 24) | serial;
		// serial = (((unsigned long long)(message->data[12])) << 32) | serial;
		// serial = (((unsigned long long)(message->data[13])) << 40) | serial;
		// serial = (((unsigned long long)(message->data[14])) << 48) | serial;
		// serial = (((unsigned long long)(message->data[15])) << 56) | serial;
		// parse->linear->y = *((double*)&(serial));
		//
		// serial = (((unsigned long long)(message->data[16])));
		// serial = (((unsigned long long)(message->data[17])) << 8)  | serial;
		// serial = (((unsigned long long)(message->data[18])) << 16) | serial;
		// serial = (((unsigned long long)(message->data[19])) << 24) | serial;
		// serial = (((unsigned long long)(message->data[20])) << 32) | serial;
		// serial = (((unsigned long long)(message->data[21])) << 40) | serial;
		// serial = (((unsigned long long)(message->data[22])) << 48) | serial;
		// serial = (((unsigned long long)(message->data[23])) << 56) | serial;
		// parse->linear->z = *((double*)&(serial));
		//
		// for (int i = 0; i < 8; i++)
		// 	ST7735_Message(ST7735_DISPLAY_BOTTOM, i, "x:", message->data[i + 24]);
		//
		// serial = (((unsigned long long)(message->data[24])));
		// serial = (((unsigned long long)(message->data[25])) << 8)  | serial;
		// serial = (((unsigned long long)(message->data[26])) << 16) | serial;
		// serial = (((unsigned long long)(message->data[27])) << 24) | serial;
		// serial = (((unsigned long long)(message->data[28])) << 32) | serial;
		// serial = (((unsigned long long)(message->data[29])) << 40) | serial;
		// serial = (((unsigned long long)(message->data[30])) << 48) | serial;
		// serial = (((unsigned long long)(message->data[31])) << 56) | serial;
		// parse->angular->x = *((double*)&(serial));
		//
		// serial = (((unsigned long long)(message->data[32])));
		// serial = (((unsigned long long)(message->data[33])) << 8)  | serial;
		// serial = (((unsigned long long)(message->data[34])) << 16) | serial;
		// serial = (((unsigned long long)(message->data[35])) << 24) | serial;
		// serial = (((unsigned long long)(message->data[36])) << 32) | serial;
		// serial = (((unsigned long long)(message->data[37])) << 40) | serial;
		// serial = (((unsigned long long)(message->data[38])) << 48) | serial;
		// serial = (((unsigned long long)(message->data[39])) << 56) | serial;
		// parse->angular->y = *((double*)&(serial));
		//
		// serial = (((unsigned long long)(message->data[40])));
		// serial = (((unsigned long long)(message->data[41])) << 8)  | serial;
		// serial = (((unsigned long long)(message->data[42])) << 16) | serial;
		// serial = (((unsigned long long)(message->data[43])) << 24) | serial;
		// serial = (((unsigned long long)(message->data[44])) << 32) | serial;
		// serial = (((unsigned long long)(message->data[45])) << 40) | serial;
		// serial = (((unsigned long long)(message->data[46])) << 48) | serial;
		// serial = (((unsigned long long)(message->data[47])) << 56) | serial;
		// parse->angular->z = *((double*)&(serial));

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
