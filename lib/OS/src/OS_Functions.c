#include "OS_Functions.h"
#include "OS.h"
#include "UART.h"
#include "diskio.h"
#include "loader.h"
#include "ff.h"
#include "Interpreter.h"
#include "ST7735.h"

extern char paramBuffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];
extern sema_t OPEN_FREE;
extern int activeThreads;
FIL file;
extern int disk_set;
char readBuffer[512];
extern tcb_t *tcbLists[NUM_PRIORITIES];
extern tcb_t *SleepPt;

extern unsigned long JitterHistogram1[JITTERSIZE];
extern unsigned long JitterHistogram2[JITTERSIZE];


// Symbol table
static const ELFSymbol_t symtab[] = {
    {"ST7735_Message", ST7735_Message},
    {"UART_OutString", UART_OutString},
    {"UART_InString", UART_InString},
    {"f_open", f_open},
    {"f_close", f_close},
    {"f_read", f_read},
    {"f_write", f_write},
    {"f_lseek", f_lseek},
};

// Launch an ELF process
void open(void) {
  ELFEnv_t env = {symtab, 8};
  IT_GetBuffer(paramBuffer);
  OS_bWait(&OPEN_FREE);
  UART_OutString("\n\r");
  // Loader funtions
  UART_SetColor(CYAN);
  if (exec_elf(paramBuffer[0], &env) == -1) {
    UART_OutError("\n\rERROR: file not found");
  }
  UART_SetColor(RESET);
  OS_bSignal(&OPEN_FREE);
  IT_Kill();
}

// List directories
void ls(void) {
  DIR cwd;
  FILINFO f;
  int i;
  UART_OutString("\n\r");
  f_opendir(&cwd, "/");
  f_readdir(&cwd, &f);
  while (f.fname[0] != 0) {
    UART_OutString(f.fname);
    UART_OutString("\t");
    f_readdir(&cwd, &f);
  }
  f_closedir(&cwd);
  IT_Kill();
}

// List direcotires in long format
void ls_l(void) {
  DIR cwd;
  FILINFO f;
  int i = 0;

  f_opendir(&cwd, "/");
  f_readdir(&cwd, &f);
  while (f.fname[0] != 0) {
    i++;
    f_readdir(&cwd, &f);
  }
  UART_OutString("\n\rtotal ");
  UART_OutUDec(i);

  f_opendir(&cwd, "/");
  f_readdir(&cwd, &f);
  while (f.fname[0] != 0) {
    // Attributes
    if (f.fattrib & AM_DIR == AM_DIR) {
      UART_OutString("\n\rd");
    } else {
      UART_OutString("\n\r-");
    }
    if (f.fattrib & AM_RDO == AM_RDO) {
      UART_OutString("r-");
    } else {
      UART_OutString("rw");
    }
    if (strstr(f.fname, ".AXF") != NULL) {
      UART_OutString("x\t");
    } else {
      UART_OutString("-\t");
    }

    // Size
    UART_OutUDec(f.fsize);
    UART_OutString("\t");

    // Date
    // Month
    switch ((f.fdate >> 5) && 0x0F) {
      case 1:
        UART_OutString("Jan ");
        break;
      case 2:
        UART_OutString("Feb ");
        break;
      case 3:
        UART_OutString("Mar ");
        break;
      case 4:
        UART_OutString("Apr ");
        break;
      case 5:
        UART_OutString("May ");
        break;
      case 6:
        UART_OutString("Jun ");
        break;
      case 7:
        UART_OutString("Jul ");
        break;
      case 8:
        UART_OutString("Aug ");
        break;
      case 9:
        UART_OutString("Sep ");
        break;
      case 10:
        UART_OutString("Oct ");
        break;
      case 11:
        UART_OutString("Nov ");
        break;
      case 12:
        UART_OutString("Dec ");
        break;
    }
    // Day
    UART_OutUDec(f.fdate & 0x1F);
    UART_OutString("\t");
    // Year
    UART_OutUDec((f.fdate >> 9) + 1980);
    UART_OutString("\t");

    //Time
    UART_OutUDec(f.ftime >> 11);
    UART_OutString(":");
    UART_OutUDec((f.ftime >> 5) & 0x1F);
    UART_OutString("\t");

    UART_OutString(f.fname);
    f_readdir(&cwd, &f);
  }
  f_closedir(&cwd);
  IT_Kill();
}

// Create new file
void touch(void) {
  int code;
  IT_GetBuffer(paramBuffer);
  code = f_open(&file, paramBuffer[0], FA_CREATE_ALWAYS);
  if (code) {
    UART_OutError("\n\rERROR ");
    UART_SetColor(RED);
    UART_OutUDec(code);
    UART_SetColor(RESET);
    UART_OutError(": File not created");
  }
  f_close(&file);
  IT_Kill();
}

// Remove file
void rm(void) {
  int code;
  IT_GetBuffer(paramBuffer);
  code = f_unlink(paramBuffer[0]);
  if (code) {
    UART_OutError("\n\rERROR ");
    UART_SetColor(RED);
    UART_OutUDec(code);
    UART_SetColor(RESET);
    UART_OutError(": File not found");
    IT_Kill();
  }
  IT_Kill();
}

// Print file
void cat(void) {
  int code, i;
  char out;
  unsigned int bytesRead = 0;
  IT_GetBuffer(paramBuffer);
  for (i = 0; i < 512; i++) {
    readBuffer[i] = 0;
  }

  code = f_open(&file, paramBuffer[0], FA_READ);
  if (code) {
    UART_OutError("\n\rERROR ");
    UART_SetColor(RED);
    UART_OutUDec(code);
    UART_SetColor(RESET);
    UART_OutError(": File not found");
    IT_Kill();
  }
  UART_OutError("\n\r");
  while (f_read(&file, &out, 1, &bytesRead) == FR_OK && bytesRead == 1) {
    if (bytesRead != 1)
      break;
    UART_OutChar2(out);
  }
  f_close(&file);
  IT_Kill();
}

// Make directory
void mkdir(void) {
  IT_GetBuffer(paramBuffer);
  if (paramBuffer[0] == 0) {
    UART_OutError("\n\r  ERROR: must specify name");
  }
  if (f_mkdir(paramBuffer[0]) == 0) {
    UART_OutError("\n\r  ERROR: directory could not be created");
  }
  IT_Kill();
}

// Format disk
void df(void) {
  int code;
  // NO CLUE HOW TO DO THIS
  code = f_mkfs("", 1, 512);
  if (code) {
    UART_OutError("\n\rERROR ");
    UART_SetColor(RED);
    UART_OutUDec(code);
    UART_SetColor(RESET);
    UART_OutError(": SD not formatted");
    IT_Kill();
  }
  IT_Kill();
}

// Print system time
void systime(void) {
  UART_OutString("\n\r  Time: ");
  UART_OutUDec(OS_Time());
  UART_OutString(" ms ");
  UART_OutString("\n\r        ");
  UART_OutUDec(OS_Time() / 1000 / 60 / 60 / 24);
  UART_OutString("d:");
  UART_OutUDec((OS_Time() / 1000 / 60 / 60) % 24);
  UART_OutString("h:");
  UART_OutUDec((OS_Time() / 1000 / 60) % 60);
  UART_OutString("m:");
  UART_OutUDec((OS_Time() / 1000) % 60);
  UART_OutString("s");
  IT_Kill();
}

// Mount filesystem
void mount(void) {
  OS_AddThread("fsinit", &OS_FsInit, 128, 0);
  IT_Kill();
}

// Force mount filesystem
void mount_f(void) {
  if (disk_set) {
    OS_RemovePeriodicThread(disk_set);
    disk_set = 0;
  }
  OS_AddThread("fsinit", &OS_FsInit, 128, 0);
  IT_Kill();
}

void tcb(void) {
  tcb_t *thread;
  UART_OutString("\n\r");
  for (int i = 0; i < NUM_PRIORITIES; i++) {
    thread = tcbLists[i];
    if (thread) {
      do {
        UART_OutString(thread->name);
        UART_OutString("\t");
        thread = thread->next;
      } while (thread != tcbLists[i]);
    }
  }
  IT_Kill();
}

void tcb_l(void) {
  tcb_t *thread;
  UART_OutString("\n\rid\tname\tstack\tpri\ts");
  for (int i = 0; i < NUM_PRIORITIES; i++) {
    thread = tcbLists[i];
    if (thread) {
      do {
        UART_OutString("\n\r");
        UART_OutUDec(thread->id);
        UART_OutString("\t");
        UART_OutString(thread->name);
        UART_OutString("\t");
        UART_OutUDec(thread->stackSize);
        UART_OutString("\t");
        UART_OutUDec(thread->priority);
        thread = thread->next;
      } while (thread != tcbLists[i]);
    }
  }
  thread = SleepPt;
  if (thread) {
    if (thread == SleepPt->prev) {
      UART_OutString("\n\r");
      UART_OutUDec(thread->id);
      UART_OutString("\t");
      UART_OutString(thread->name);
      UART_OutString("\t");
      UART_OutUDec(thread->stackSize);
      UART_OutString("\t");
      UART_OutUDec(thread->priority);
      UART_OutString("\ts");
    } else {
      while (thread != SleepPt->prev) {
        UART_OutString("\n\r");
        UART_OutUDec(thread->id);
        UART_OutString("\t");
        UART_OutString(thread->name);
        UART_OutString("\t");
        UART_OutUDec(thread->stackSize);
        UART_OutString("\t");
        UART_OutUDec(thread->priority);
        UART_OutString("\ts");
        thread = thread->next;
      }
    }
  }
  IT_Kill();
}

// Count active TCBs
void tcb_c(void) {
  UART_OutString("\n\r Active TCBs: ");
  UART_OutUDec(activeThreads);
  IT_Kill();
}

// Print max jitter
void jitter() {
  UART_OutString("\r\n  Jitter1: ");
  UART_OutUDec(OS_GetJitter(OS_THREAD_1));
  UART_OutString("\r\n  Jitter2: ");
  UART_OutUDec(OS_GetJitter(OS_THREAD_2));
  IT_Kill();
}

// Print jitter histogram
void jitter_h() {
  unsigned long max1 = JitterHistogram1[0];
  unsigned long max2 = JitterHistogram2[0];
  for (int i = 1; i < JITTERSIZE; i++) {
    if (max1 < JitterHistogram1[i])
      max1 = JitterHistogram1[i];
    if (max2 < JitterHistogram2[i])
      max2 = JitterHistogram2[i];
  }
  UART_OutString("\n\r\n\r  Jitter1: ");
  UART_OutUDec(OS_GetJitter(OS_THREAD_1));
  UART_OutString("\r\n");
  for (int i = 10; i > 0; i--) {
    UART_OutString("\r\n ║ ");
    for (int j = 0; j < JITTERSIZE; j++) {
      if (JitterHistogram1[j]*100/max1/10 >= i) {
        UART_OutString("█ ");
      } else {
        if (JitterHistogram1[j] > 0 && i == 1) {
          UART_OutString("█ ");
        } else {
          UART_OutString("  ");
        }
      }
    }
  }
  UART_OutString("\r\n ╚═");
  for (int i = 0; i < JITTERSIZE; i++) {
    UART_OutString("══");
  }
  UART_OutString("\r\n\r\n");
  UART_OutString("\r\n  Jitter2: ");
  UART_OutUDec(OS_GetJitter(OS_THREAD_2));
  UART_OutString("\r\n");
  for (int i = 10; i > 0; i--) {
    UART_OutString("\r\n ║ ");
    for (int j = 0; j < JITTERSIZE; j++) {
      if (JitterHistogram2[j]*100/max2/10 >= i) {
        UART_OutString("█ ");
      } else {
        if (JitterHistogram2[j] > 0 && i == 1) {
          UART_OutString("█ ");
        } else {
          UART_OutString("  ");
        }
      }
    }
  }
  UART_OutString("\r\n ╚═");
  for (int i = 0; i < JITTERSIZE; i++) {
    UART_OutString("══");
  }
  UART_OutString("\r\n   ");
  IT_Kill();
}

// Kill a specific thread by name
void killall(void) {
  tcb_t *thread;
  int count = 0;
  IT_GetBuffer(paramBuffer);
  for (int i = 0; i < NUM_PRIORITIES; i++) {
    thread = tcbLists[i];
    if (thread) {
      do {
        if (strcmp(thread->name, paramBuffer[0]) == 0) {
          removeThread(thread);
          Heap_Free(thread->stack);
          Heap_Free(thread);
          UART_OutString("\n\r  Thread removed successfully");
          count++;
        }
        thread = thread->next;
      } while (thread != tcbLists[i]);
    }
  }
  thread = SleepPt;
  if (thread) {
    if (thread == SleepPt->prev) {
      if (strcmp(thread->name, paramBuffer[0]) == 0) {
        SleepPt = 0;
        Heap_Free(thread->stack);
        Heap_Free(thread);
        UART_OutString("\n\r  Thread removed successfully");
        count++;
      }
      if (count == 0)
        UART_OutError("\n\r  ERROR: Thread not found");
      IT_Kill();
    } else {
      while (thread != SleepPt->prev) {
        if (strcmp(thread->name, paramBuffer[0]) == 0) {
          thread->prev->next = thread->next;
          thread->next->prev = thread->prev;
          Heap_Free(thread->stack);
          Heap_Free(thread);
          UART_OutString("\n\r  Thread removed successfully");
          count++;
        }
        thread = thread->next;
      }
      if (strcmp(thread->name, paramBuffer[0]) == 0) {
        SleepPt->prev = thread->prev;
        thread->prev->next = thread->next;
        Heap_Free(thread->stack);
        Heap_Free(thread);
        UART_OutString("\n\r  Thread removed successfully");
        count++;
      }
      if (count == 0)
        UART_OutError("\n\r  ERROR: Thread not found");
      IT_Kill();
    }
  }
  if (count == 0)
    UART_OutError("\n\r  ERROR: Thread not found");
  IT_Kill();
}
