#include "Float64.h"
#include "Heap.h"

char ROS_FLOAT64_MD5[] = "fdb28210bfa9d7c91146260178d9a584";
char ROS_FLOAT64_MSG[] = "std_msgs/Float64";

char* ROS_Float64MD5(void) {
	return ROS_FLOAT64_MD5;
}

char* ROS_Float64MSG(void) {
	return ROS_FLOAT64_MSG;
}

unsigned char* ROS_Float64Serialize(rosfloat64_t *rosfloat64) {
	unsigned char *data;
	unsigned long long serial = *((unsigned long long*)&(rosfloat64->data));

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_FLOAT64_LEN);
	if (data == 0)
		return 0;

	data[0] = (unsigned char)(serial) & 0xFF;
	data[1] = (unsigned char)(serial >> 8)  & 0xFF;
	data[2] = (unsigned char)(serial >> 16) & 0xFF;
	data[3] = (unsigned char)(serial >> 24) & 0xFF;
	data[4] = (unsigned char)(serial >> 32) & 0xFF;
	data[5] = (unsigned char)(serial >> 40) & 0xFF;
	data[6] = (unsigned char)(serial >> 48) & 0xFF;
	data[7] = (unsigned char)(serial >> 56) & 0xFF;
	return data;
}

rosfloat64_t* ROS_Float64Deserialize(rospacket_t *message) {
	rosfloat64_t *parse;
	unsigned long long serial;
	unsigned char *aux = message->data;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rosfloat64_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == ROS_FLOAT64_LEN){
		serial = 0;
		for (int i = 0; i < 8; i++)
			serial = (((unsigned long long)(*(aux++))) << (8 * i)) | serial;

		parse->data = *((double*)&(serial));
		return parse;
	}

	// Error, free memory
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_Float64Free(rosfloat64_t *rosfloat64) {
	Heap_Free(rosfloat64);
}
