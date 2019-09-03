#include "Float32.h"
#include "Heap.h"

char ROS_FLOAT32_MD5[] = "73fcbf46b49191e672908e50842a83d4";
char ROS_FLOAT32_MSG[] = "std_msgs/Float32";

char* ROS_Float32MD5(void) {
	return ROS_FLOAT32_MD5;
}

char* ROS_Float32MSG(void) {
	return ROS_FLOAT32_MSG;
}

unsigned char* ROS_Float32Serialize(rosfloat32_t *rosfloat32) {
	unsigned char *data;
	unsigned int serial = *((unsigned int*)&(rosfloat32->data));

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_FLOAT32_LEN);
	if (data == 0)
		return 0;

	data[0] = (unsigned char)(serial) & 0xFF;
	data[1] = (unsigned char)((serial) >> 8)  & 0xFF;
	data[2] = (unsigned char)((serial) >> 16) & 0xFF;
	data[3] = (unsigned char)((serial) >> 24) & 0xFF;
	return data;
}

rosfloat32_t* ROS_Float32Deserialize(rospacket_t *message) {
	rosfloat32_t *parse;
	unsigned int serial;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rosfloat32_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == ROS_FLOAT32_LEN){
		serial = (unsigned int)message->data[0];
		serial = ((unsigned int)message->data[1] << 8)  | serial;
		serial = ((unsigned int)message->data[2] << 16) | serial;
		serial = ((unsigned int)message->data[3] << 24) | serial;

		parse->data = *((float*)&(serial));

		return parse;
	}

	// Error, free memory
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_Float32Free(rosfloat32_t *rosfloat32) {
	Heap_Free(rosfloat32);
}
