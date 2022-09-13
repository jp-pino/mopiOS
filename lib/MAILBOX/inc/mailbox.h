#ifndef __MAILBOX_H__
#define __MAILBOX_H__

#include "OS.h"

// Float Mailbox
typedef struct float_mailbox_t {
	float data;
	sema_t free;
	sema_t valid;
} float_mailbox_t;

void MailBox_Float_Init(float_mailbox_t *mailbox);
void MailBox_Float_Send(float_mailbox_t *mailbox, float data);
float MailBox_Float_Recv(float_mailbox_t *mailbox);

// Int Mailbox
typedef struct int_mailbox_t {
	int data;
	sema_t free;
	sema_t valid;
} int_mailbox_t;

void MailBox_Int_Init(int_mailbox_t *mailbox);
void MailBox_Int_Send(int_mailbox_t *mailbox, int data);
int MailBox_Int_Recv(int_mailbox_t *mailbox);

#endif
