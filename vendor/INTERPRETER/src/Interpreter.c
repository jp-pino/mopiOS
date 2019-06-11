#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "Interpreter.h"
#include "heap.h"
#include "UART.h"
#include "ST7735.h"
#include "OS.h"


cmd_t *cmdList = 0;
char paramBuffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];

// Helper functions
int digits_only(const char *s) {
  while (*s) {
    if (isdigit(*s++) == 0) return 0;
  }
  return 1;
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
  UART_OutStringColor("\r\nadmin$ ", GREEN);
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

cmd_t* IT_AddCommand(char *cmd, unsigned char params, char *params_descr, void(*task)(void), char *descr) {
  cmd_t *new;
  long sr = OS_StartCritical();

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

    // Add to cmdList
    new->next = cmdList;
    cmdList = new;
    OS_EndCritical(sr);
    return new;
  }
  OS_EndCritical(sr);
  return 0;
}

flag_t* IT_AddFlag(cmd_t *cmd, char flag, unsigned char params, char *params_descr, void(*task)(void), char *descr) {
  flag_t *new;
  long sr = OS_StartCritical();

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
  int i;
  char cmd_input[IT_MAX_CMD_LEN * 4 + 1];
  char *word, *flag_input;
  flag_t *flag;
  cmd_t *cmd;

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
                  (*flag->task)();
                  // OS_AddThread(flag->task, 128, 4);
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
            if (cmd->task != NULL)
              (*cmd->task)();
              // OS_AddThread(cmd->task, 128, 4);
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
            if (cmd->task != NULL && i == cmd->params)
              (*cmd->task)();
              // OS_AddThread(cmd->task, 128, 4);
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
