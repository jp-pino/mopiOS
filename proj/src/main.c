//*****************************************************************************
//
// Lab5.c - Barebones OS initialization
//*****************************************************************************

#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "OS.h"
#include "Interpreter.h"
#include "UART.h"
#include "ros.h"
#include "Bool.h"
#include "Int32.h"
#include "Heap.h"

#define TIMESLICE TIME_1MS  // thread switch time in system time units

#define PF2 (*((volatile uint32_t *)0x40025010))

void ROSBoolPublisherExample(void) {
	rospacket_t *message;
	rosbool_t *boolmessage;

	// Run ROS_PublisherInit() once for synchronization purposes
	// ROS_FindPublisher() returns this publisher
	ROS_PublisherInit(ROS_FindPublisher());

	while(1) {
		// Setup data to transmit
		boolmessage = Heap_Malloc(sizeof(rosbool_t));
		boolmessage->data = 1;

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

void ROSInt32PublisherExample(void) {
	rospacket_t *message;
	rosint32_t *int32message;
	int val = 1;

	// Run ROS_PublisherInit() once for synchronization purposes
	// ROS_FindPublisher() returns this publisher
	ROS_PublisherInit(ROS_FindPublisher());

	while(1) {
		// Setup data to transmit
		int32message = Heap_Malloc(sizeof(rosint32_t));
		int32message->data = val;

		// Setup packet to transmit
		message = Heap_Malloc(sizeof(rospacket_t));
		message->length = ROS_INT32_LEN;
		message->topic_id = ROS_FindPublisher()->topic_id;

		// Serialize data
		message->data = ROS_Int32Serialize(int32message);

		// Transmit packet
		ROS_Publish(message);
		OS_Sleep(1000);

		// Free data memory
		ROS_Int32Free(int32message);

		// Free packet memory
		ROS_PacketFree(message);
		val++;
	}
}

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

// OS and modules initialization
int main(void){        // lab 4 real main
  // Initialize the OS
  OS_Init();           // initialize, disable interrupts
  // Initialize other modules

  // Add Threads
  // OS_AddThread(..., ..., ...);

	// Add Topics
	ROS_AddPublisher("pos_x", ROS_BoolMSG(), ROS_BoolMD5(), &ROSBoolPublisherExample, 512, 2);
	ROS_AddPublisher("int_t", ROS_Int32MSG(), ROS_Int32MD5(), &ROSInt32PublisherExample, 512, 2);
	ROS_AddSubscriber("test", ROS_BoolMSG(), ROS_BoolMD5(), &ROSBoolSubscriberExample, 512, 2);

  // Launch the OS
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}

// Kill thread by name
