#include <string.h>
#include "TopicInfo.h"
#include "Heap.h"

char ROS_TOPICINFO_MD5[] = "0ad51f88fc44892f8c10684077646005";
char ROS_TOPICINFO_MSG[] = "rosserial_msgs/TopicInfo";

char* ROS_TopicInfoMD5(void) {
	return ROS_TOPICINFO_MD5;
}

char* ROS_TopicInfoMSG(void) {
	return ROS_TOPICINFO_MSG;
}

unsigned int ROS_TopicInfoLength(rostopicinfo_t *rostopic) {
	return ROS_TOPICINFO_LEN + strlen(rostopic->topic_name) + 1 + strlen(rostopic->message_type) + 1 + strlen(rostopic->md5sum) + 1;
}

unsigned char* ROS_TopicInfoSerialize(rostopicinfo_t *rostopic) {
	unsigned char *data;
	unsigned int i, start;

	//Allocate memory for serialization
	data = Heap_Malloc(sizeof(unsigned char) * ROS_TopicInfoLength(rostopic));
	if (data == 0)
		return 0;

	data[0] = (unsigned char)(rostopic->topic_id) & 0xFF;
	data[1] = (unsigned char)(rostopic->topic_id >> 8) & 0xFF;
	start = 2;

	for (i = 0; i < 4; i++) {
		data[i + start] = (unsigned char)(strlen(rostopic->topic_name) >> (8 * i));
	}
	start += 4;

	for (i = 0; i <= strlen(rostopic->topic_name); i++) {
		data[i + start] = (unsigned char)rostopic->topic_name[i];
	}
	start += strlen(rostopic->topic_name);;

	for (i = 0; i < 4; i++) {
		data[i + start] = (unsigned char)(strlen(rostopic->message_type) >> (8 * i));
	}
	start += 4;

	for (i = 0; i <= strlen(rostopic->message_type); i++) {
		data[i + start] = (unsigned char)rostopic->message_type[i];
	}
	start += strlen(rostopic->message_type);

	for (i = 0; i < 4; i++) {
		data[i + start] = (unsigned char)(strlen(rostopic->md5sum) >> (8 * i));
	}
	start += 4;

	for (i = 0; i <= strlen(rostopic->md5sum); i++) {
		data[i + start] = (unsigned char)rostopic->md5sum[i];
	}
	start += strlen(rostopic->md5sum);

	data[start + 3] = (unsigned char)(rostopic->buffer_size >> 24) & 0xFF;
	data[start + 2] = (unsigned char)(rostopic->buffer_size >> 16) & 0xFF;
	data[start + 1] = (unsigned char)(rostopic->buffer_size >> 8) & 0xFF;
	data[start] = (unsigned char)rostopic->buffer_size & 0xFF;
	return data;
}

// rostopicinfo_t* ROS_TopicInfoDeserialize(rospacket_t *message) {
// 	rostopicinfo_t *parse;
//
// 	// Allocate memory for parse
// 	parse = Heap_Malloc(sizeof(rostopicinfo_t));
// 	if (parse == 0)
// 		return 0;
//
// 	// Parse message
// 	if (message->length == 1){
// 		parse->data = message->data;
// 		return parse;
// 	}
//
// 	// Error, free memory
// 	Heap_Free(parse);
// 	return 0;
// }


// Deallocate memory
void ROS_TopicInfoFree(rostopicinfo_t *rostopic) {
	Heap_Free(rostopic);
}
