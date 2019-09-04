#include "Time.h"
#include "Heap.h"

char ROS_TIME_MD5[] = "cd7166c74c552c311fbcc2fe5a7bc289";
char ROS_TIME_MSG[] = "std_msgs/Time";

char* ROS_TimeMD5(void) {
	return ROS_TIME_MD5;
}

char* ROS_TimeMSG(void) {
	return ROS_TIME_MSG;
}

unsigned char* ROS_TimeSerialize(rostime_t *rostime) {
	unsigned char *data;

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_TIME_LEN);
	if (data == 0)
		return 0;

	data[0] = rostime->data & 0xFF;
	data[1] = (rostime->data >> 8)  & 0xFF;
	data[2] = (rostime->data >> 16) & 0xFF;
	data[3] = (rostime->data >> 24) & 0xFF;
	return data;
}

rostime_t* ROS_TimeDeserialize(rospacket_t *message) {
	rostime_t *parse;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rostime_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == ROS_TIME_LEN){
		parse->data = message->data[0] | (message->data[1] << 8) | (message->data[2] << 16) | (message->data[3] << 24);
		return parse;
	}

	// Error, free memory
	Heap_Free(parse);
	return 0;
}


// Deallocate memory
void ROS_TimeFree(rostime_t *rostime) {
	Heap_Free(rostime);
}
