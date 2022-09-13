#include "OS.h"
#include "mailbox.h"

// Float Mailbox
void MailBox_Float_Init(float_mailbox_t *mailbox) {
  OS_InitSemaphore("mf", &(mailbox->free), 1);
  OS_InitSemaphore("mv", &(mailbox->valid), 0);
	mailbox->data = 0;
}

void MailBox_Float_Send(float_mailbox_t *mailbox, float data) {
  OS_bWait(&(mailbox->free));
  mailbox->data = data;
  OS_bSignal(&(mailbox->valid));
}

float MailBox_Float_Recv(float_mailbox_t *mailbox) {
  float data;
  OS_bWait(&(mailbox->valid));
  data = mailbox->data;
  OS_bSignal(&(mailbox->free));
  return data;
}

// Int Mailbox
void MailBox_Int_Init(int_mailbox_t *mailbox) {
  OS_InitSemaphore("mf", &(mailbox->free), 1);
  OS_InitSemaphore("mv", &(mailbox->valid), 0);
	mailbox->data = 0;
}

void MailBox_Int_Send(int_mailbox_t *mailbox, int data) {
  OS_bWait(&(mailbox->free));
  mailbox->data = data;
  OS_bSignal(&(mailbox->valid));
}

int MailBox_Int_Recv(int_mailbox_t *mailbox) {
  int data;
  OS_bWait(&(mailbox->valid));
  data = mailbox->data;
  OS_bSignal(&(mailbox->free));
  return data;
}
