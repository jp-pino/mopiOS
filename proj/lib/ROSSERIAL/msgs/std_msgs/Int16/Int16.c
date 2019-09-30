#include "Int16.h"
#include "Heap.h"

char ROS_INT16_MD5[] = "8524586e34fbd7cb1c08c5f5f1ca0e57";
char ROS_INT16_MSG[] = "std_msgs/Int16";

char* ROS_Int16MD5(void) {
	return ROS_INT16_MD5;
}

char* ROS_Int16MSG(void) {
	return ROS_INT16_MSG;
}

unsigned char* ROS_Int16Serialize(rosint16_t *rosint16) {
	unsigned char *data;

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_INT16_LEN);
	if (data == 0)
		return 0;

	data[0] = rosint16->data & 0xFF;
	data[1] = (rosint16->data >> 8)  & 0xFF;
	return data;
}

rosint16_t* ROS_Int16Deserialize(rospacket_t *message) {
	rosint16_t *parse;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rosint16_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == ROS_INT16_LEN){
		parse->data = message->data[0] | (message->data[1] << 8);
		return parse;
	}

	// Error, free memory
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_Int16Free(rosint16_t *rosint16) {
	Heap_Free(rosint16);
}
