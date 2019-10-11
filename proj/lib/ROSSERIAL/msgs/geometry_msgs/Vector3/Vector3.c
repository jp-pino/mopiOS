#include "Vector3.h"
#include "Heap.h"
#include "ST7735.h"

char ROS_VECTOR3_MD5[] = "4a842b65f413084dc2b10fb484ea7f17";
char ROS_VECTOR3_MSG[] = "geometry_msgs/Vector3";

char* ROS_Vector3MD5(void) {
	return ROS_VECTOR3_MD5;
}

char* ROS_Vector3MSG(void) {
	return ROS_VECTOR3_MSG;
}

unsigned char* ROS_Vector3Serialize(rosvector3_t *rosvector3) {
	unsigned char *data;
	unsigned long long serialx = *((unsigned long long*)&(rosvector3->x));
	unsigned long long serialy = *((unsigned long long*)&(rosvector3->y));
	unsigned long long serialz = *((unsigned long long*)&(rosvector3->z));

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_VECTOR3_LEN);
	if (data == 0)
		return 0;

	for (int i = 0; i < 8; i++) {
		data[i] = (unsigned char)(serialx >> (8 * i)) & 0xFF;
	}
	for (int i = 0; i < 8; i++) {
		data[i+8] = (unsigned char)(serialy >> (8 * i)) & 0xFF;
	}
	for (int i = 0; i < 8; i++) {
		data[i+16] = (unsigned char)(serialz >> (8 * i)) & 0xFF;
	}
	return data;
}

rosvector3_t* ROS_Vector3Deserialize(rospacket_t *message) {
	rosvector3_t *parse;
	unsigned long long serial;
	unsigned char *aux = message->data;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rosvector3_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == ROS_VECTOR3_LEN) {
		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->x = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->y = *((double*)&(serial));

		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;
		parse->z = *((double*)&(serial));

		return parse;
	}

	// Error, free memory
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_Vector3Free(rosvector3_t *rosvector3) {
	Heap_Free(rosvector3);
}
