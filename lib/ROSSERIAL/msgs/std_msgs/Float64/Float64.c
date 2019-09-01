#include "Float64.h"
#include "Heap.h"

char ROS_FLOAT64_MD5[] = "8b94c1b53db61fb6aed406028ad6332a";
char ROS_FLOAT64_MSG[] = "std_msgs/Float64";

typedef struct {
	unsigned int high;
	unsigned int low;
} int_double_t;

char* ROS_Float64MD5(void) {
	return ROS_FLOAT64_MD5;
}

char* ROS_Float64MSG(void) {
	return ROS_FLOAT64_MSG;
}

unsigned char* ROS_Float64Serialize(rosfloat64_t *rosfloat64) {
	unsigned char *data;
	unsigned int serial1 = *((unsigned int*)&(rosfloat64->data));
	unsigned int serial2 = *((unsigned int*)(&(rosfloat64->data) + sizeof(unsigned int)));

	//Allocate memory for serialization
	data = Heap_Malloc(ROS_FLOAT64_LEN);
	if (data == 0)
		return 0;

	data[0] = (unsigned char)((serial1)) 			 & 0xFF;
	data[1] = (unsigned char)((serial1) >> 8)  & 0xFF;
	data[2] = (unsigned char)((serial1) >> 16) & 0xFF;
	data[3] = (unsigned char)((serial1) >> 24) & 0xFF;
	data[4] = (unsigned char)((serial2)) 			 & 0xFF;
	data[5] = (unsigned char)((serial2) >> 8)  & 0xFF;
	data[6] = (unsigned char)((serial2) >> 16) & 0xFF;
	data[7] = (unsigned char)((serial2) >> 24) & 0xFF;
	return data;
}

rosfloat64_t* ROS_Float64Deserialize(rospacket_t *message) {
	rosfloat64_t *parse;
	int_double_t serial;

	// Allocate memory for parse
	parse = Heap_Malloc(sizeof(rosfloat64_t));
	if (parse == 0)
		return 0;

	// Parse message
	if (message->length == ROS_FLOAT64_LEN){
		serial.high =  message->data[0];
		serial.high = (message->data[1] << 8)  | serial.high;
		serial.high = (message->data[2] << 16) | serial.high;
		serial.high = (message->data[3] << 24) | serial.high;
		serial.low  =  message->data[4];
		serial.low  = (message->data[5] << 8)  | serial.low;
		serial.low  = (message->data[6] << 16) | serial.low;
		serial.low  = (message->data[7] << 24) | serial.low;


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
