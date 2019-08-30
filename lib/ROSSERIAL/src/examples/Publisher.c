// void main(void) {
// 	// Initialize ROS
// 	ROS_Init();
//
// 	// Add Publisher
// 	ROS_AddPublisher("pos_x", ROS_BOOL_MSG, &ROSBoolPublisherExample, 512, 2);
// 	ROS_AddSubscriber(char *topic_name, char *message_type, char *md5sum, void(*task)(void), unsigned int stack, unsigned int priority)
//
// 	// Launch ROS
// 	ROS_Launch();
// }
//
//
// void ROSBoolPublisherExample(void) {
// 	rospacket_t *message;
// 	rosbool_t *boolmessage;
// 	while(1) {
// 		// Setup data to transmit
// 		boolmessage = Heap_Malloc(sizeof(rosbool_t));
// 		boolmessage->data = 1;
//
// 		// Setup packet to transmit
// 		message = Heap_Malloc(sizeof(rospacket_t));
// 		message->lenght = ROS_BOOL_LEN;
// 		message->topic_id = ROS_FindPublisher()->topic_id;
// 		message->data = ROS_BoolSerialize(boolmessage);
//
// 		// Transmit packet
// 		ROS_Publish(message);
//
// 		// Free data memory
// 		ROS_BoolFree(boolmessage);
//
// 		// Free packet memory
// 		ROS_PacketFree(message);
// 	}
// }
//
// void ROSBoolSubscriberExample(void) {
// 	rospacket_t *message;
// 	rosbool_t *boolmessage;
// 	while(1) {
// 		// Wait for message
// 		message = ROS_MailBox_Recv(ROS_FindSubscriber()->mailbox);
// 		if (message) {
// 			// Deserialize data
// 			boolmessage = ROS_BoolDeserialize(message);
// 			if (boolmessage) {
// 				// Do whatever
// 				UART_OutChar(boolmessage->data);
// 				// Deallocate packet and data
// 				ROS_BoolFree(boolmessage);
// 				ROS_PacketFree(message);
// 			}
// 		}
// 	}
// }
