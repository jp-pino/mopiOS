#include "OS_Functions.h"
#include "OS.h"
#include "UART.h"
#include "diskio.h"
#include "loader.h"
#include "ff.h"
#include "Interpreter.h"
#include "ST7735.h"
#include "Heap.h"
#include <stdlib.h>

extern char paramBuffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];
extern sema_t OPEN_FREE;
extern int activeThreads;
FIL file;
extern int disk_set;
char readBuffer[512];
extern tcb_t *tcbLists[NUM_PRIORITIES];
extern tcb_t *SleepPt;
extern sema_t *SemaPt;
extern unsigned long JitterHistogram1[JITTERSIZE];
extern unsigned long JitterHistogram2[JITTERSIZE];
extern log_entry LOG[LOG_MAX_ENTRY];
extern DIR cwd;
extern char path[50];


// Symbol table
static const ELFSymbol_t symtab[] = {
    {"ST7735_Message", ST7735_Message},
    {"OS_Id", OS_Id},
    {"OS_AddThread", OS_AddThread},
    {"OS_Time", OS_Time},
    {"OS_Sleep", OS_Sleep},
    {"OS_Kill", OS_Kill},
    {"OS_TimeDifference", OS_TimeDifference},
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

  IT_Init();

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
  FILINFO f;
  int i;

  IT_Init();
  UART_OutString("\n\r");
  f_readdir(&cwd, &f);
  while (f.fname[0] != 0) {
    UART_OutString(tolowercase(f.fname));
    UART_OutString("\t");
    f_readdir(&cwd, &f);
  }
  IT_Kill();
}

// List direcotires in long format
void ls_l(void) {
  FILINFO f;
  int i = 0;

  IT_Init();
  f_readdir(&cwd, &f);
  while (f.fname[0] != 0) {
    i++;
    f_readdir(&cwd, &f);
  }
  UART_OutString("\n\rtotal ");
  UART_OutUDec(i);

  f_readdir(&cwd, 0);
  f_readdir(&cwd, &f);
  while (f.fname[0] != 0) {
    // Attributes
    if ((((char)(f.fattrib) >> 4) & 0x01) == (char)(0x01)) {
      UART_OutString("\n\rd");
    } else {
      UART_OutString("\n\r-");
    }
    if ((((char)(f.fattrib) >> 0) & 0x01) == (char)(0x01)) {
      UART_OutString("r-");
    } else {
      UART_OutString("rw");
    }
    if (strstr(f.fname, ".AXF") != NULL || strstr(f.fname, ".ELF") != NULL) {
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

		if ((((char)(f.fattrib) >> 1) & 0x01) == (char)(0x01))
			UART_OutString(".");
    UART_OutString(tolowercase(f.fname));
    f_readdir(&cwd, &f);
  }
  IT_Kill();
}

void cd(void) {
	IT_Init();

  IT_GetBuffer(paramBuffer);
	if(f_chdir(paramBuffer[0]) != 0) {
    UART_OutError("\n\r  ERROR: directory could not be found");
	  IT_Kill();
		return;
	}
  IT_Kill();
}

// Create new file
void touch(void) {
  int code;
	char aux[60];
  IT_Init();

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
  IT_Init();

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

  IT_Init();

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
  IT_Init();

  IT_GetBuffer(paramBuffer);
  if (paramBuffer[0] == 0) {
    UART_OutError("\n\r  ERROR: must specify name");
  }
  if (f_mkdir(paramBuffer[0]) != 0) {
    UART_OutError("\n\r  ERROR: directory could not be created");
  }
  IT_Kill();
}

// Format disk
void df(void) {
  int code;

  IT_Init();

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
  IT_Init();

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
  IT_Init();

  OS_AddThread("fsinit", &OS_FsInit, 128, 0);
  IT_Kill();
}

// Force mount filesystem
void mount_f(void) {
  IT_Init();

  if (disk_set) {
    OS_RemovePeriodicThread(disk_set);
    disk_set = 0;
  }
  OS_AddThread("fsinit", &OS_FsInit, 128, 0);
  IT_Kill();
}

void ts(void) {
  tcb_t *thread;

  IT_Init();

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

void ts_l(void) {
  tcb_t *thread;
  sema_t *semaphore;
	long sr;

  IT_Init();
	UART_OutString("\n\rtotal ");
	UART_OutUDec(OS_ThreadCount());
  UART_OutString("\n\rid\tname\t\tstack\tpri\tsleep\tblock");

	// Scheduled tasks
  for (int i = 0; i < NUM_PRIORITIES; i++) {
    thread = tcbLists[i];
    if (thread) {
      do {
        UART_OutString("\n\r");
        UART_OutUDec(thread->id);
        UART_OutString("\t");
        UART_OutString(thread->name);
				if (strlen(thread->name) < 8)
					UART_OutString("\t");
        UART_OutString("\t");
        UART_OutUDec(thread->stackSize);
        UART_OutString("\t");
        UART_OutUDec(thread->priority);
        UART_OutString("\t");
        UART_OutUDec(thread->sleepTime);
        thread = thread->next;
      } while (thread != tcbLists[i]);
    }
  }

	// Sleeping tasks
  thread = SleepPt;
  if (thread) {
    if (thread == SleepPt->prev) {
      UART_OutString("\n\r");
      UART_OutUDec(thread->id);
      UART_OutString("\t");
      UART_OutString(thread->name);
			if (strlen(thread->name) < 8)
				UART_OutString("\t");
			UART_OutString("\t");
      UART_OutUDec(thread->stackSize);
      UART_OutString("\t");
      UART_OutUDec(thread->priority);
      UART_OutString("\t");
      UART_OutUDec(thread->sleepTime);
    } else {
      while (thread != SleepPt->prev) {
        UART_OutString("\n\r");
        UART_OutUDec(thread->id);
        UART_OutString("\t");
        UART_OutString(thread->name);
				if (strlen(thread->name) < 8)
					UART_OutString("\t");
        UART_OutString("\t");
        UART_OutUDec(thread->stackSize);
        UART_OutString("\t");
        UART_OutUDec(thread->priority);
        UART_OutString("\t");
        UART_OutUDec(thread->sleepTime);
        thread = thread->next;
      }
    }
  }

  semaphore = SemaPt;
  while(semaphore != 0) {
    thread = semaphore->blocked;
    if (thread) {
      if (thread == semaphore->blocked->prev) {
        UART_OutString("\n\r");
        UART_OutUDec(thread->id);
        UART_OutString("\t");
        UART_OutString(thread->name);
				if (strlen(thread->name) < 8)
					UART_OutString("\t");
        UART_OutString("\t");
        UART_OutUDec(thread->stackSize);
        UART_OutString("\t");
        UART_OutUDec(thread->priority);
        UART_OutString("\t");
        UART_OutUDec(thread->sleepTime);
        UART_OutString("\t");
        UART_OutString(semaphore->name);
      } else {
        while(thread != semaphore->blocked->prev) {
          UART_OutString("\n\r");
          UART_OutUDec(thread->id);
          UART_OutString("\t");
          UART_OutString(thread->name);
					if (strlen(thread->name) < 8)
						UART_OutString("\t");
	        UART_OutString("\t");
          UART_OutUDec(thread->stackSize);
          UART_OutString("\t");
          UART_OutUDec(thread->priority);
          UART_OutString("\t");
          UART_OutUDec(thread->sleepTime);
          UART_OutString("\t");
          UART_OutString(semaphore->name);
          thread = thread->next;
        }
      }
    }
    semaphore = semaphore->next;
  }
  IT_Kill();
}

// Count active TCBs
void ts_c(void) {
  IT_Init();

  UART_OutString("\n\r Active TCBs: ");
  UART_OutUDec(activeThreads);
  IT_Kill();
}

// Print max jitter
void jitter() {
  IT_Init();

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

  IT_Init();

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

  IT_Init();

  IT_GetBuffer(paramBuffer);
  for (int i = 0; i < NUM_PRIORITIES; i++) {
    thread = tcbLists[i];
    if (thread) {
      do {
        if (strcmp(thread->name, paramBuffer[0]) == 0) {
          thread->prev->next = thread->next;
          thread->next->prev = thread->prev;
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

void log(void) {
  int i = 0;
  IT_Init();
  while (i < LOG_MAX_ENTRY && LOG[i].thread != -1) {
    UART_OutString("\r\n");
    switch (LOG[i].event) {
    case LOG_THREAD_START:
      UART_OutString(" FOREGROUND THREAD STARTED");
      break;
    case LOG_THREAD_SWITCH:
      UART_OutString(" FOREGROUND THREAD SWITCH ");
      break;
    case LOG_THREAD_KILL:
      UART_OutString(" FOREGROUND THREAD KILLED ");
      break;
    case LOG_PERIODIC_START:
      UART_OutString(" PERIODIC THREAD STARTED  ");
      break;
    case LOG_PERIODIC_FINISH:
      UART_OutString(" PERIODIC THREAD FINISHED ");
      break;
    }
    UART_OutString(" ID: ");
    UART_OutUDec(LOG[i].thread);
    UART_OutString("  Time: ");
    UART_OutUDec(LOG[i].time);

    i++;
  }
  IT_Kill();
}

void log_c(void) {
  int i;
  long sr;
  IT_Init();
  sr = OS_StartCritical();
  for (i = 0; i < LOG_MAX_ENTRY; i++) {
    LOG[i].thread = -1;
  }
  OS_EndCritical(sr);
  IT_Kill();
}
