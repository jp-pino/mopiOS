#include "Int32.h"
#include "Heap.h"

char ROS_INT32_MD5[] = "da5909fbe378aeaf85e547e830cc1bb7";
char ROS_INT32_MSG[] = "std_msgs/Int32";

char* ROS_Int32MD5(void) {
	return ROS_INT32_MD5;
}

char* ROS_Int32MSG(void) {
	return ROS_INT32_MSG;
}

unsigned char* ROS_Int32Serialize(rosint32_t *rosint32) {
	unsigned char *data;

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_INT32_LEN);
	if (data == 0)
		return 0;

	data[0] = rosint32->data & 0xFF;
	data[1] = (rosint32->data >> 8)  & 0xFF;
	data[2] = (rosint32->data >> 16) & 0xFF;
	data[3] = (rosint32->data >> 24) & 0xFF;
	return data;
}

rosint32_t* ROS_Int32Deserialize(rospacket_t *message) {
	rosint32_t *parse;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rosint32_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == ROS_INT32_LEN){
		parse->data = message->data[0] | (message->data[1] << 8) | (message->data[2] << 16) | (message->data[3] << 24);
		return parse;
	}

	// Error, free memory
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_Int32Free(rosint32_t *rosint32) {
	Heap_Free(rosint32);
}
