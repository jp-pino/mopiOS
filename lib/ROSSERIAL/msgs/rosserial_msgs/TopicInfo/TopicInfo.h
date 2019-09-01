#ifndef __TOPICINFO_H__
#define __TOPICINFO_H__

#define ROS_TOPICINFO_LEN 18

#include "ros.h"

// Special Topic IDs
#define ROS_ID_PUBLISHER 					0
#define ROS_ID_SUBSCRIBER 				1
#define ROS_ID_SERVICE_SERVER 		2
#define ROS_ID_SERVICE_CLIENT 		4
#define ROS_ID_PARAMETER_REQUEST 	6
#define ROS_ID_LOG 								7
#define ROS_ID_TIME 							10
#define ROS_ID_TX_STOP 						11

typedef struct {
	unsigned short topic_id;
	char *topic_name;
	char *message_type;
	char *md5sum;
	unsigned int buffer_size;
} rostopicinfo_t;

char* ROS_TopicInfoMD5(void);
char* ROS_TopicInfoMSG(void);

unsigned int ROS_TopicInfoLength(rostopicinfo_t *rostopic);
unsigned char* ROS_TopicInfoSerialize(rostopicinfo_t *rostopic);
rostopicinfo_t* ROS_TopicInfoDeserialize(rospacket_t *message);

// Deallocate memory
void ROS_TopicInfoFree(rostopicinfo_t *rostopic);

#endif
