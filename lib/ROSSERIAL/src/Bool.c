#include "Bool.h"
#include "Heap.h"

char ROS_BOOL_MD5[] = "8b94c1b53db61fb6aed406028ad6332a";
char ROS_BOOL_MSG[] = "std_msgs/Bool";

char* ROS_BoolMD5(void) {
	return ROS_BOOL_MD5;
}

char* ROS_BoolMSG(void) {
	return ROS_BOOL_MSG;
}

unsigned char* ROS_BoolSerialize(rosbool_t *rosbool) {
	unsigned char *data;

	//Allocate memory for serialization
	data = Heap_Malloc(sizeof(unsigned char) * ROS_BOOL_LEN);
	if (data == 0)
		return 0;

	data[0] = rosbool->data;
	return data;
}

rosbool_t* ROS_BoolDeserialize(rospacket_t *message) {
	rosbool_t *parse;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rosbool_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == 1){
		parse->data = message->data;
		return parse;
	}

	// Error, free memory
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_BoolFree(rosbool_t *rosbool) {
	Heap_Free(rosbool);
}
