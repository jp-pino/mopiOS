# mopiOS-rosserial

## Introduction
This is my own personal implementation of the rosserial protocol.

## Supported messages
Not all message types are supported yet. The following have been tested to work:
- std_msgs/Bool
- rosserial_msgs/TopicInfo

## Instructions
Before launching `rosmain` using `ROS_Launch()`, first you must register your publishers and subscribers.

For example, to register a publisher using `std_msgs/Bool` and **status** as topic name run:
```c
ROS_AddPublisher("status", ROS_BoolMSG(), ROS_BoolMD5(), &ROSBoolPublisherExample, 512, 2);
```
Here `&ROSBoolPublisherExample` is the thread that will be sending these kinds of messages. All publishers must run `ROS_PublisherInit(ROS_FindPublisher())` to synchronize with the `rosmain`. For example:
```c
void ROSBoolPublisherExample(void) {
	rospacket_t *message;
	rosbool_t *boolmessage;

	// Run ROS_PublisherInit() once for synchronization purposes
	// ROS_FindPublisher() returns this publisher
	ROS_PublisherInit(ROS_FindPublisher());

	while(1) {
		// Setup data to transmit
		boolmessage = Heap_Malloc(sizeof(rosbool_t));
		boolmessage->data = 1; // For Bool can only be 1 or 0

		// Setup packet to transmit
		message = Heap_Malloc(sizeof(rospacket_t));
		message->length = ROS_BOOL_LEN;
		message->topic_id = ROS_FindPublisher()->topic_id;

		// Serialize data
		message->data = ROS_BoolSerialize(boolmessage);

		// Transmit packet
		ROS_Publish(message);
		OS_Sleep(1000);

		// Free data memory
		ROS_BoolFree(boolmessage);

		// Free packet memory
		ROS_PacketFree(message);
	}
}
```

To register a subscriber the prototype is basically the same. For example, to register a subscriber that listens for messages of type `std_msgs/Bool` and **action** as topic name run:
```c
ROS_AddSubscriber("action", ROS_BoolMSG(), ROS_BoolMD5(), &ROSBoolSubscriberExample, 512, 2);
```

The callback function `&ROSBoolSubscriberExample` must also synchronize with `rosmain` using `ROS_SubscriberInit(ROS_FindSubscriber())` and also must listen for messages from `rosmain` on the mailbox:
```c
void ROSBoolSubscriberExample(void) {
	rospacket_t *message;
	rosbool_t *boolmessage;
	ROS_SubscriberInit(ROS_FindSubscriber());
	while(1) {
		// Wait for message
		message = ROS_MailBox_Recv(&(ROS_FindSubscriber()->mailbox));
		if (message) {
			// Deserialize data
			boolmessage = ROS_BoolDeserialize(message);
			if (boolmessage) {
				// Do whatever
				PF2 ^= 0x04;
				// Deallocate packet and data
				ROS_BoolFree(boolmessage);
				ROS_PacketFree(message);
			}
		}
	}
}
```

Now that we've set up our publishers and subscribers, we can run `OS_Launch()` from another thread to initialize our environment and connect to the `rosserial_python` server.
