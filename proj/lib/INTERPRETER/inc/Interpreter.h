#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#define IT_MAX_CMD_LEN 20
#define IT_MAX_PARAM_N 4

#include "OS.h"

typedef struct FLAG {
  struct FLAG *next;
  char flag;
  unsigned char params;
  char descr[IT_MAX_CMD_LEN * 3];
  void(*task)(void);
  char params_descr[IT_MAX_CMD_LEN * 3];
  unsigned int stack;
  unsigned int priority;
  sema_t semaphore;
} flag_t;

typedef struct CMD {
  struct CMD *next;
  flag_t *flags;
  char cmd[IT_MAX_CMD_LEN + 1];
  unsigned char params;
  char descr[IT_MAX_CMD_LEN * 3];
  void(*task)(void);
  char params_descr[IT_MAX_CMD_LEN * 3];
  unsigned int stack;
  unsigned int priority;
  sema_t semaphore;
} cmd_t;

void IT_SetDir(char* cwd);
void Interpreter(void);
int digits_only(const char*);
char* tolowercase(char *string);
void IT_Init(void);
void IT_setFAT(void);
void IT_Kill(void);
void IT_GetBuffer(char buffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN]);
cmd_t* IT_AddCommand(char *cmd, unsigned char params, char *params_descr, void(*task)(void), char *descr, unsigned int stack, unsigned int priority);
flag_t* IT_AddFlag(cmd_t *cmd, char flag, unsigned char params, char *params_descr, void(*task)(void), char *descr, unsigned int stack, unsigned int priority);


#endif
