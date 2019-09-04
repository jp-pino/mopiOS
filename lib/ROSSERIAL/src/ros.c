#include <string.h>
#include "OS.h"
#include "ros.h"
#include "Heap.h"
#include "Interpreter.h"
#include "UART.h"
#include "UART1.h"
#include "TopicInfo.h"
#include "Time.h"

sema_t ROS_TIMER_READY;

// Limitations:
// 		- One subscriber per topic
// 		- One publisher per topic
// 		- Topic name len must be the same as TCB_NAME_LEN

// Mailbox functions
// rosmain:
// 		1. Allocate packet
// 		2. Send packet
// 		3. (Don't deallocate)

// Subscriber:
// 		1. Receive packet
// 			1. Allocate new packet
// 			2. Copy values
// 			3. Deallocate old packet (but not data)
// 		2. Do what you will with packet
// 		3. Deallocate packet and data

// Mailbox ---------------------------------------------------------------
// Initialize
void ROS_MailBox_Init(rosmailbox_t *mailbox) {
  OS_InitSemaphore("rosmail_f", &(mailbox->free), 1);
  OS_InitSemaphore("rosmail_v", &(mailbox->valid), 0);
}

// Send
void ROS_MailBox_Send(rosmailbox_t *mailbox, rospacket_t *message) {
  OS_bWait(&(mailbox->free));
  mailbox->message = message;
  OS_bSignal(&(mailbox->valid));
}

// Receive
rospacket_t* ROS_MailBox_Recv(rosmailbox_t *mailbox) {
	rospacket_t *message = Heap_Malloc(sizeof(rospacket_t));
  OS_bWait(&(mailbox->valid));
	message->length = mailbox->message->length;
	message->topic_id = mailbox->message->topic_id;
	message->data = mailbox->message->data;
	Heap_Free(mailbox->message);
  OS_bSignal(&(mailbox->free));
  return message;
}

// Deallocate memory
void ROS_PacketFree(rospacket_t *message) {
	if (message->data)
		Heap_Free(message->data);
	Heap_Free(message);
}

// Publishers and Subscribers --------------------------------------------
unsigned short topic_id = 101;
sema_t ROS_ADD_FREE;
rospublisher_t *ROSPublishers = 0;
rossubscriber_t *ROSSubscribers = 0;

rospublisher_t* ROS_AddPublisher(char *topic_name, char *message_type, char *md5sum, void(*task)(void), unsigned int stack, unsigned int priority) {
	rospublisher_t *publisher = Heap_Malloc(sizeof(rospublisher_t));


	OS_bWait(&ROS_ADD_FREE);

	publisher->topic_id = topic_id++;

	if (strlen(topic_name) > ROS_TOPIC_LEN) {
		Heap_Free(publisher);
		OS_bSignal(&ROS_ADD_FREE);
		return 0;
	}
	strcpy(publisher->topic_name, topic_name);

	publisher->message_type = message_type;

	publisher->md5sum = md5sum;

	OS_InitSemaphore("ros_start", &(publisher->start), 0);

	publisher->thread = OS_AddThread(topic_name, task, stack, priority);
	if (publisher->thread == 0) {
		Heap_Free(publisher);
		OS_bSignal(&ROS_ADD_FREE);
		return 0;
	}

	if (ROSPublishers == 0) {
		ROSPublishers = publisher;
		publisher->next = 0;
	} else {
		publisher->next = ROSPublishers;
		ROSPublishers = publisher;
	}

	OS_bSignal(&ROS_ADD_FREE);
	return publisher;
}

rossubscriber_t* ROS_AddSubscriber(char *topic_name, char *message_type, char *md5sum, void(*task)(void), unsigned int stack, unsigned int priority) {
	rossubscriber_t *subscriber = Heap_Malloc(sizeof(rossubscriber_t));

	OS_bWait(&ROS_ADD_FREE);

	subscriber->topic_id = topic_id++;

	if (strlen(topic_name) > ROS_TOPIC_LEN) {
		Heap_Free(subscriber);
		OS_bSignal(&ROS_ADD_FREE);
		return 0;
	}
	strcpy(subscriber->topic_name, topic_name);

	subscriber->message_type = message_type;

	subscriber->md5sum = md5sum;

	OS_InitSemaphore("ros_start", &(subscriber->start), 0);

	ROS_MailBox_Init(&(subscriber->mailbox));

	subscriber->thread = OS_AddThread(topic_name, task, stack, priority);
	if (subscriber->thread == 0) {
		Heap_Free(subscriber);
		OS_bSignal(&ROS_ADD_FREE);
		return 0;
	}

	if (ROSSubscribers == 0) {
		ROSSubscribers = subscriber;
		subscriber->next = 0;
	} else {
		subscriber->next = ROSSubscribers;
		ROSSubscribers = subscriber;
	}

	OS_bSignal(&ROS_ADD_FREE);
	return subscriber;
}

// Look for subscribers
// Find subscriber by topic name
rossubscriber_t* ROS_FindSubscriberByTopic(unsigned short topic_id) {
	rossubscriber_t *current;
	current = ROSSubscribers;
	while (current) {
		if (current->topic_id == topic_id)
			return current;
		current = current->next;
	}
	return 0;
}

// Find subscriber from current thread
rossubscriber_t* ROS_FindSubscriber(void) {
	rossubscriber_t *current;
	current = ROSSubscribers;
	while (current) {
		if (current->thread->id == OS_Id())
			return current;
		current = current->next;
	}
	return 0;
}

// Find publisher from current thread
rospublisher_t* ROS_FindPublisher(void) {
		rospublisher_t *current;
		current = ROSPublishers;
		while (current) {
			if (current->thread->id == OS_Id())
				return current;
			current = current->next;
		}
		return 0;
}

// Checksums -------------------------------------------------------------

// Message Length Checksum = 255 - ((Message Length High Byte +
//                                   Message Length Low Byte) % 256 )
unsigned char checksum_length(unsigned short length) {
  return (unsigned char)(255 - ((((length >> 8) & 0xFF) + (length & 0xFF)) % 256) );
}

// Message Data Checksum = 255 - ((Topic ID Low Byte +
//                                 Topic ID High Byte +
//                                 Data byte values) % 256)
unsigned char checksum_data(unsigned short topic, unsigned short length, unsigned char *data) {
  unsigned char checksum = (topic & 0xFF) + ((topic >> 8) & 0xFF);
  for (int i = 0; i < length; i++) {
    checksum += data[i];
  }
  checksum = checksum % 256;
  checksum = 255 - checksum;
  return checksum;
}



// Protocol --------------------------------------------------------------

// 1st Byte - Sync Flag (Value: 0xff)
// 2nd Byte - Sync Flag / Protocol version
// 3rd Byte - Message Length (N) - Low Byte
// 4th Byte - Message Length (N) - High Byte
// 5th Byte - Checksum over message length
// 6th Byte - Topic ID - Low Byte
// 7th Byte - Topic ID - High Byte
// x Bytes  - Serialized Message Data
// Byte x+1 - Checksum over Topic ID and Message Data

// Send packet to PC

sema_t ROS_PUBLISH_FREE;

void ROS_Publish(rospacket_t *message) {
	OS_bWait(&ROS_PUBLISH_FREE);
  UART1_OutChar(SYNC_F);
  UART1_OutChar(PROT_F);

  UART1_OutChar((unsigned char)((message->length) & 0xFF));
  UART1_OutChar((unsigned char)((message->length >> 8) & 0xFF));
  UART1_OutChar((unsigned char)(checksum_length(message->length)));

  UART1_OutChar((unsigned char)((message->topic_id) & 0xFF));
  UART1_OutChar((unsigned char)((message->topic_id >> 8) & 0xFF));

  for (int i = 0; i < message->length; i++) {
    UART1_OutChar((unsigned char)(message->data[i]));
  }
  UART1_OutChar((unsigned char)(checksum_data(message->topic_id, message->length, message->data)));
	OS_bSignal(&ROS_PUBLISH_FREE);
}

// Receive packet from PC
rospacket_t* message_in(void) {
  rospacket_t *message;
  unsigned char csum_length, csum_data;
  unsigned short length, topic;

	// Allocate memory for message
  message = Heap_Malloc(sizeof(rospacket_t));
  if (message == 0)
    return 0;

	// Sync message
	while (UART1_InChar() != SYNC_F);
	if (UART1_InChar() != PROT_F) {
    Heap_Free(message);
		return 0;
	}

	// Read message length
  length = UART1_InChar();
	length = UART1_InChar() << 8 | length;
	message->length = length;

	// Read checksum and verify
  csum_length = UART1_InChar();
  if (csum_length != checksum_length(length)) {
    Heap_Free(message);
    return 0;
	}

	// Allocate memory for message data
  message->data = Heap_Malloc(sizeof(unsigned char) * length);
  if (message->data == 0) {
    Heap_Free(message);
    return 0;
  }

	topic = UART1_InChar();
	topic = UART1_InChar() << 8 | topic;
	message->topic_id = topic;

  for (int i = 0; i < length; i++) {
    message->data[i] = UART1_InChar();
  }

  csum_data = UART1_InChar();
  if (csum_data != checksum_data(topic, length, message->data)) {
    Heap_Free(message->data);
    Heap_Free(message);
    return 0;
  }
  return message;
}

// Topic negotiation
void rosnegotiate(void) {
	rospacket_t *message;
	rostopicinfo_t *topic;
	rossubscriber_t *subscriber;
	rospublisher_t *publisher;
	int i = 0;


	// Register all subscribers
	subscriber = ROSSubscribers;
	while (subscriber) {
		// Allocate memory
		message = Heap_Malloc(sizeof(rospacket_t));
		if (message == 0)
			continue;
		topic = Heap_Malloc(sizeof(rostopicinfo_t));
		if (topic == 0) {
			Heap_Free(message);
			continue;
		}

		// Write values
		topic->topic_id = subscriber->topic_id;
		topic->topic_name = subscriber->topic_name;
		topic->message_type = subscriber->message_type;
		topic->md5sum = subscriber->md5sum;
		topic->buffer_size = 280;

		// Prepare message
		message->length = ROS_TopicInfoLength(topic);
		message->topic_id = ROS_ID_SUBSCRIBER;

		// Serialize topic
		message->data = ROS_TopicInfoSerialize(topic);

		// Publish
		if (message->data) {
			ROS_Publish(message);
		}

		// Free memory for topic
		ROS_TopicInfoFree(topic);

		// Free memory for message
		ROS_PacketFree(message);

		// Advance pointer
		subscriber = subscriber->next;
	}

	// Register all publishers
	publisher = ROSPublishers;
	while (publisher) {
		// Allocate memory
		message = Heap_Malloc(sizeof(rospacket_t));
		if (message == 0)
			continue;
		topic = Heap_Malloc(sizeof(rostopicinfo_t));
		if (topic == 0) {
			Heap_Free(message);
			continue;
		}

		// Write values
		topic->topic_id = publisher->topic_id;
		topic->topic_name = publisher->topic_name;
		topic->message_type = publisher->message_type;
		topic->md5sum = publisher->md5sum;
		topic->buffer_size = 280;

		// Prepare message
		message->length = ROS_TopicInfoLength(topic);
		message->topic_id = ROS_ID_PUBLISHER;

		// Serialize topic
		message->data = ROS_TopicInfoSerialize(topic);

		// Publish
		if (message->data) {
			ROS_Publish(message);

		}

		// Free memory for topic
		ROS_TopicInfoFree(topic);

		// Free memory for message
		ROS_PacketFree(message);

		// Advance pointer
		publisher = publisher->next;
	}
}

// Main thread
void rosmain(void) {
	rospacket_t *message;
	rostime_t *time;
	rossubscriber_t *subscriber;
	rospublisher_t *publisher;

	// Get topic request
	while (UART1_InChar() != SYNC_F);
	if (UART1_InChar() != PROT_F) {
		OS_AddThread("ros", &rosmain, 1536, 1);
		OS_Kill();
	}
	if (UART1_InChar() != 0x00) {
		OS_AddThread("ros", &rosmain, 1536, 1);
		OS_Kill();
	}
	if (UART1_InChar() != 0x00) {
		OS_AddThread("ros", &rosmain, 1536, 1);
		OS_Kill();
	}
	if (UART1_InChar() != 0xFF) {
		OS_AddThread("ros", &rosmain, 1536, 1);
		OS_Kill();
	}
	if (UART1_InChar() != 0x00) {
		OS_AddThread("ros", &rosmain, 1536, 1);
		OS_Kill();
	}
	if (UART1_InChar() != 0x00) {
		OS_AddThread("ros", &rosmain, 1536, 1);
		OS_Kill();
	}
	if (UART1_InChar() != 0xFF) {
		OS_AddThread("ros", &rosmain, 1536, 1);
		OS_Kill();
	}

	// Topic negotiation
	rosnegotiate();

	// Request time
	message = Heap_Malloc(sizeof(rospacket_t));
	if (message) {
		message->length = 0;
		message->topic_id = ROS_ID_TIME;
		message->data = 0;
		ROS_Publish(message);
		Heap_Free(message);
	}

	// Launch all subscribers
	subscriber = ROSSubscribers;
	while (subscriber) {
		OS_bSignal(&(subscriber->start));
		subscriber = subscriber->next;
	}

	// Launch all publishers
	publisher = ROSPublishers;
	while (publisher) {
		OS_bSignal(&(publisher->start));
		publisher = publisher->next;
	}

	// Start timer to keep sync
	OS_bSignal(&ROS_TIMER_READY);

	// Listen for topics
	while(1) {
		message = message_in();
		if (message == 0) continue;
	 	if (message->topic_id > 100) {
			subscriber = ROS_FindSubscriberByTopic(message->topic_id);
			if (subscriber)
				ROS_MailBox_Send(&(subscriber->mailbox), message);
		} else if (message->topic_id == 0) {
			rosnegotiate();
		} else if (message->topic_id == ROS_ID_TIME) {
			time = ROS_TimeDeserialize(message);
		}
		ROS_TimeFree(time);
		ROS_PacketFree(message);
	}
}

void rostimer(void) {
	rospacket_t *message;
	float val = 1.0f;

	// Run ROS_PublisherInit() once for synchronization purposes
	// ROS_FindPublisher() returns this publisher
	OS_bWait(&ROS_TIMER_READY);
	while(1) {
		// Setup packet to transmit
		message = Heap_Malloc(sizeof(rospacket_t));
		message->length = ROS_TIME_LEN;
		message->topic_id = ROS_ID_TIME;

		// Serialize data
		message->data = 0;

		// Transmit packet
		ROS_Publish(message);
		OS_Sleep(1000);

		// Free packet memory
		ROS_PacketFree(message);
	}
}

// Initialize ------------------------------------------------------------
// Called after declaring Publishers and Subscribers
void ROS_Init(void) {
	// Port initialization
	UART1_Init();
	// SEMAPHORE INITIALIZATION
	OS_InitSemaphore("rosout", &ROS_PUBLISH_FREE, 1);
	OS_InitSemaphore("rosadd", &ROS_ADD_FREE, 1);
	OS_InitSemaphore("rostimer", &ROS_TIMER_READY, 0);

	// Add timer thread
	OS_AddThread("rostimer", &rostimer, 128, 5);
}

void ROS_Launch(void) {
	IT_Init();
	// Launch main thread
	OS_AddThread("ros", &rosmain, 512, 1); // Same priority as interpreter
	IT_Kill();
}

void ROS_SubscriberInit(rossubscriber_t *subscriber) {
	OS_bWait(&(subscriber->start));
}

void ROS_PublisherInit(rospublisher_t *publisher) {
	OS_bWait(&(publisher->start));
}
