#ifndef __ROS_H__
#define __ROS_H__

#define SYNC_F 0xFF
#define PROT_F 0xFE

#include "OS.h"

#define ROS_TOPIC_LEN TCB_NAME_LEN

// Serialized messages
typedef struct {
  unsigned short length;
  unsigned short topic_id;
  unsigned char *data;
} rospacket_t;

// ROS Subscriber Mailbox
typedef struct rosmailbox_t {
	sema_t free;
	sema_t valid;
	rospacket_t *message;
} rosmailbox_t;

// Publisher
typedef struct rospublisher_t {
	unsigned short topic_id;
	char topic_name[ROS_TOPIC_LEN + 1];
	char *message_type;
	char *md5sum;
	struct rospublisher_t *next;
	sema_t start;
	tcb_t *thread;
} rospublisher_t;

// Subscriber
typedef struct rossubscriber_t {
	unsigned short topic_id;
	char topic_name[ROS_TOPIC_LEN + 1];
	char *message_type;
	char *md5sum;
	struct rossubscriber_t *next;
	sema_t start;
	rosmailbox_t mailbox;
	tcb_t *thread;
} rossubscriber_t;

// Mailbox functions
void ROS_MailBox_Init(rosmailbox_t *mailbox);
void ROS_MailBox_Send(rosmailbox_t *mailbox, rospacket_t *message);
rospacket_t* ROS_MailBox_Recv(rosmailbox_t *mailbox);
void ROS_PacketFree(rospacket_t *message);

// Setup publishers and subscribers
rossubscriber_t* ROS_AddSubscriber(char *topic_name, char *message_type, char *md5sum, void(*task)(void), unsigned int stack, unsigned int priority);
rospublisher_t* ROS_AddPublisher(char *topic_name, char *message_type, char *md5sum, void(*task)(void), unsigned int stack, unsigned int priority);

// Look for subscribers
rossubscriber_t* ROS_FindSubscriberByTopic(unsigned short topic_id);
rossubscriber_t* ROS_FindSubscriber(void);
rospublisher_t* ROS_FindPublisher(void);

// Send serialized message
void ROS_Publish(rospacket_t *message);

// Setup ROS Environment
void ROS_Init(void);
void ROS_Launch(void);
void ROS_SubscriberInit(rossubscriber_t *subscriber);
void ROS_PublisherInit(rospublisher_t *publisher);


#endif
