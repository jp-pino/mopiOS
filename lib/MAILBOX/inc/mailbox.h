#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#include "OS.h"

// Float Mailbox
typedef struct float_mailbox_t {
	sema_t free;
	sema_t valid;
	float data;
} float_mailbox_t;

void MailBox_Float_Init(float_mailbox_t *mailbox);
void MailBox_Float_Send(float_mailbox_t *mailbox, float data);
float MailBox_Float_Recv(float_mailbox_t *mailbox);

#endif
