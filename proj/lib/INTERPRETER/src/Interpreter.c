#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "Interpreter.h"
#include "heap.h"
#include "UART.h"
#include "ST7735.h"
#include "OS.h"
#include "ff.h"

extern DIR cwd;

cmd_t *cmdList = 0;
char paramBuffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];
sema_t *semaphore;
sema_t IT_FREE;
sema_t IT_READY;
int fat = 0;

void IT_setFAT(void) {
	fat = 1;
}

int IT_isFAT(void) {
	return fat;
}

// Helper functions
int digits_only(const char *s) {
  while (*s) {
    if (isdigit(*s++) == 0) return 0;
  }
  return 1;
}

char* tolowercase(char *string) {
	char *aux;
	aux = string;
	while (*string) {
		if (*string >= 65 && *string <= 90)
			*string = *string + 32;
		string++;
	}
	return aux;
}

void printBanner() {
  UART_SetColor(RESET);
  UART_OutString("\r\n\r\n");
  UART_OutString("                                ██████╗ ███████╗\r\n");
  UART_OutString(" ███╗ ███╗  ██████╗ ██████╗ ██╗██╔═══██╗██╔════╝\r\n");
  UART_OutString("██████████╗██╔═══██╗██╔══██╗██║██║   ██║███████╗\r\n");
  UART_OutString("██╔═██╔═██║██║   ██║██████╔╝██║██║   ██║╚════██║\r\n");
  UART_OutString("██║ ╚═╝ ██║╚██████╔╝██╔═══╝ ██║╚██████╔╝███████║\r\n");
  UART_OutString("╚═╝     ╚═╝ ╚═════╝ ██║     ╚═╝ ╚═════╝ ╚══════╝\r\n");
  UART_OutString("                    ╚═╝                         \r\n");
}

void printPrompt(void) {
	static char aux[50];
	char *current, *next;

	f_getcwd(aux, 50);
	UART_OutStringColor("\r\nadmin:", GREEN);

	if (aux[0] == 0) {
		UART_OutStringColor("/$ ", GREEN);
		return;
	}

	current = aux;
	if (strcmp(aux,"/") != 0) {
		next = strtok(aux + 1, "/");
		while (next != NULL) {
			current = next;
			next = strtok(NULL, "/");
		}
	}
	UART_OutStringColor(tolowercase(current), GREEN);
	UART_OutStringColor("$ ", GREEN);
}

void IT_Init(void) {
	static char aux[50];
	if (IT_isFAT() == 0)
		return;
	f_getcwd(aux, 50);
	f_closedir(&cwd);
	f_opendir(&cwd, aux);
}

void IT_Kill(void) {
	if (IT_isFAT())
		f_readdir(&cwd, 0);
  OS_bSignal(&(*semaphore));
  OS_Kill();
}

// Help
void IT_Help(void) {
  char flag_cmd[2];
  cmd_t *cmd;
  flag_t *flag;
  cmd = cmdList;
  flag_cmd[1] = '\0';

  UART_OutStringColor("\r\nList of implemented commands:\r\n", CYAN);

  while (cmd != 0) {
    // Print command
    UART_OutString("  ");
    UART_OutStringColor(cmd->cmd, YELLOW);

    // Print parameters
    UART_OutString(" ");
    if (cmd->params > 0)
      UART_OutString(cmd->params_descr);

    // Print descriptions
    if (cmd->descr[0] != 0) {
      UART_OutString("\r\n    #");
      UART_OutString(cmd->descr);
    }

    UART_OutString("\r\n");

    // Print flags
    flag = cmd->flags;
    while(flag != 0) {
      flag_cmd[0] = flag->flag;
      UART_OutString("    -");
      UART_OutString(flag_cmd);
      if (flag->params > 0) {
        UART_OutString(" ");
        UART_OutString(flag->params_descr);
        UART_OutString("\r\n       #");
      } else {
        UART_OutString(" #");
      }
      UART_OutString(flag->descr);
      UART_OutString("\r\n");
      flag = flag->next;
    }
    cmd = cmd->next;
  }
  UART_OutStringColor("  clear\r\n", YELLOW);
  UART_OutString("    #clear screen\r\n");
  UART_OutStringColor("  banner\r\n", YELLOW);
  UART_OutString("    #print mopiOS banner\r\n");
  UART_OutStringColor("  help\r\n", YELLOW);
  UART_OutString("    #display help\r\n");
}


void IT_GetBuffer(char buffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN]) {
  int i, j;
  for (i = 0; i < IT_MAX_PARAM_N; i++)
    for (j = 0; j < IT_MAX_CMD_LEN; j++)
      buffer[i][j] = paramBuffer[i][j];
}

cmd_t* IT_AddCommand(char *cmd, unsigned char params, char *params_descr, void(*task)(void), char *descr, unsigned int stack, unsigned int priority) {
  cmd_t *new;
  long sr = OS_StartCritical();

  new = cmdList;
  while (new != 0) {
    if (strcmp(new->cmd, cmd) == 0) {
      OS_EndCritical(sr);
      return 0;
    }
    new = new->next;
  }

  new = Heap_Malloc(sizeof(cmd_t));
  if (new != 0) {
    // Add command
    strcpy(new->cmd, cmd);

    // Add parameter number
    new->params = params;

    // Add parameter descriptions
    if (params > 0)
      strcpy(new->params_descr, params_descr);

    // Add function to execute
    new->task = task;

    // Add description
    strcpy(new->descr, descr);

    // Set flags to 0
    new->flags = 0;

    // Set stack size
    new->stack = stack;

    // Set priority
    new->priority = priority;

    // Initialize sempahore
    OS_InitSemaphore(cmd, &new->semaphore, 0);

    // Add to cmdList
    new->next = cmdList;
    cmdList = new;
    OS_EndCritical(sr);
    return new;
  }
  OS_EndCritical(sr);
  return 0;
}

flag_t* IT_AddFlag(cmd_t *cmd, char flag, unsigned char params, char *params_descr, void(*task)(void), char *descr, unsigned int stack, unsigned int priority) {
  flag_t *new;
  long sr = OS_StartCritical();
	char name[IT_MAX_CMD_LEN + 3];

  new = Heap_Malloc(sizeof(flag_t));
  if (new != 0) {
    // Set flag
    new->flag = flag;

    // Add parameter number
    new->params = params;

    // Add parameter descriptions
    if (params > 0)
      strcpy(new->params_descr, params_descr);

    // Add function to execute
    new->task = task;

    // Add description
    strcpy(new->descr, descr);

    // Set stack size
    new->stack = stack;

    // Set priority
    new->priority = priority;

		// Prepare semaphore name
		strcpy(name, cmd->cmd);
		name[strlen(cmd->cmd)] = '_';
		name[strlen(cmd->cmd) + 1] = flag;
		name[strlen(cmd->cmd) + 2] = 0;

    // Initialize sempahore
    OS_InitSemaphore(name, &new->semaphore, 0);


    // Add flag
    new->next = cmd->flags;
    cmd->flags = new;
    OS_EndCritical(sr);
    return new;
  }
  OS_EndCritical(sr);
  return 0;
}

void Interpreter(void) {
  static int i;
  static char cmd_input[IT_MAX_CMD_LEN * 4 + 1];
  static char *word, *flag_input;
  static flag_t *flag;
  static cmd_t *cmd;

  OS_InitSemaphore("it_free", &IT_FREE, 1);

  // Clear screen
  UART_OutString("\033[2J\033[H");
  printBanner();


  while (1) {
    // Cmd line begins
    printPrompt();

    // Get the user's input
    UART_InCMD(cmd_input);
    word = strtok(cmd_input, " ");

    // Check for help or clear function
    if (strcmp(word, "help") == 0) {
      IT_Help();
      continue;
    } else if (strcmp(word, "clear") == 0) {
      UART_OutString("\033[2J\033[H");
      continue;
    } else if (strcmp(word, "banner") == 0) {
      UART_OutString("\033[2J\033[H");
      printBanner();
      continue;
    } else if (strcmp(word, "reboot") == 0) {
			NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
			continue;
		}

    cmd = cmdList;
    // Go through all the commands in the array
    while (cmd != 0) {
      // Check if it is a valid command
      if (strcmp(cmd->cmd, word) == 0) {
        // Check if it contains valid flags
        word = strtok(NULL, " ");
        if (word[0] == '-') {
          // The first flag is after the -
          flag_input = word + 1;
          // Read the flag string one char at a time
          while (flag_input != 0 && *flag_input != '\0') {
            // Compare to every flag in memory for the command
            flag = cmd->flags;
            while(flag != 0) {
              // If it matches, read parameters and execute
              if (*flag_input == flag->flag) {
                // Read the required parameters from the input
                for (i = 0; i < flag->params; i++) {
                  word = strtok(NULL, " ");
                  // If the number of parameters is wrong
                  if (word == NULL) {
                    UART_OutError("\r\nERROR: COMMAND EXPECTED ");
                    UART_SetColor(RED);
                    UART_OutUDec(flag->params);
                    UART_SetColor(RESET);
                    UART_OutError(" PARAMETERS");
                    flag_input = 0;
                    break;
                  }
                  strcpy(paramBuffer[i], word);
                }
                // If we have all the parameters, call the function
                // The function has to read the buffer inside to get the parameters
                if (flag->task != NULL && i == flag->params) {
                  // (*flag->task)();
                  semaphore = &(flag->semaphore);
                  if(OS_AddThread(cmd->cmd, flag->task, flag->stack, flag->priority))
                    OS_bWait(&(flag->semaphore));
                  break;
                }
              }
              flag = flag->next;
              if (flag == 0 && flag_input != 0) {
                UART_OutError("\r\nERROR: FLAG ");
                UART_OutError(flag_input);
                UART_OutError(" NOT FOUND");
                flag_input = 0;
								word = 0;
                break;
              }
            }
            if (flag_input == 0)
              break;
						flag_input++;
          }
        } else {
          // The cmd didn't receive flags, check if it takes parameters
          if (cmd->params == 0) {
            // If it doesn't take any parameters, execute
            if (cmd->task != NULL){
              // (*cmd->task)();
              semaphore = &(cmd->semaphore);
              if(OS_AddThread(cmd->cmd, cmd->task, cmd->stack, cmd->priority))
                OS_bWait(&(cmd->semaphore));
            }
          } else {
            // The command takes parameters. Load them in buffer and execute
            for (i = 0; i < cmd->params; i++) {
              if (word == NULL) {
                UART_OutError("\r\nERROR: COMMAND EXPECTED ");
                UART_SetColor(RED);
                UART_OutUDec(cmd->params);
                UART_SetColor(RESET);
                UART_OutError(" PARAMETERS");
                break;
              }
              strcpy(paramBuffer[i], word);
              word = strtok(NULL, " ");
            }
            // If we have all the parameters, call the function
            // The function has to read the buffer inside to get the parameters
            if (cmd->task != NULL && i == cmd->params) {
              // (*cmd->task)();
              semaphore = &(cmd->semaphore);
              if (OS_AddThread(cmd->cmd, cmd->task, cmd->stack, cmd->priority))
                OS_bWait(&(cmd->semaphore));
            }
          }
        }
        // If it found the command break
        break;
      }
      cmd = cmd->next;
    }
    // If no command was found, then print error
    if (cmd == NULL && word != NULL ) {
      UART_OutError("\r\nERROR: COMMAND \"");
      UART_OutError(word);
      UART_OutError("\" NOT FOUND");
    }
  }
}
