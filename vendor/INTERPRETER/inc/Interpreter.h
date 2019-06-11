#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#define IT_MAX_CMD_LEN 10
#define IT_MAX_PARAM_N 5

typedef struct FLAG {
  struct FLAG *next;
  char flag;
  unsigned char params;
  char descr[IT_MAX_CMD_LEN * 3];
  void(*task)(void);
  char params_descr[IT_MAX_CMD_LEN * 3];
} flag_t;

typedef struct CMD {
  struct CMD *next;
  flag_t *flags;
  char cmd[IT_MAX_CMD_LEN];
  unsigned char params;
  char descr[IT_MAX_CMD_LEN * 3];
  void(*task)(void);
  char params_descr[IT_MAX_CMD_LEN * 3];
} cmd_t;

void Interpreter(void);
int digits_only(const char*);
void IT_GetBuffer(char buffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN]);
cmd_t* IT_AddCommand(char *cmd, unsigned char params, char *params_descr, void(*task)(void), char *descr);
flag_t* IT_AddFlag(cmd_t *cmd, char flag, unsigned char params, char *params_descr, void(*task)(void), char *descr);


#endif
