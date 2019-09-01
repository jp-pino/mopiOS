#include "UInt32.h"
#include "Heap.h"

char ROS_UINT32_MD5[] = "da5909fbe378aeaf85e547e830cc1bb7";
char ROS_UINT32_MSG[] = "std_msgs/UInt32";

char* ROS_UInt32MD5(void) {
	return ROS_UINT32_MD5;
}

char* ROS_UInt32MSG(void) {
	return ROS_UINT32_MSG;
}

unsigned char* ROS_UInt32Serialize(rosuint32_t *rosuint32) {
	unsigned char *data;

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_UINT32_LEN);
	if (data == 0)
		return 0;

	data[0] = rosuint32->data & 0xFF;
	data[1] = (rosuint32->data >> 8)  & 0xFF;
	data[2] = (rosuint32->data >> 16) & 0xFF;
	data[3] = (rosuint32->data >> 24) & 0xFF;
	return data;
}

rosuint32_t* ROS_UInt32Deserialize(rospacket_t *message) {
	rosuint32_t *parse;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rosuint32_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == ROS_UINT32_LEN){
		parse->data = message->data[0] | (message->data[1] << 8) | (message->data[2] << 16) | (message->data[3] << 24);
		return parse;
	}

	// Error, free memory
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_UInt32Free(rosuint32_t *rosuint32) {
	Heap_Free(rosuint32);
}
