// os.c
// Runs on LM4F120/TM4C123
// A very simple real time operating system with minimal features.
// Daniel Valvano
// January 29, 2015

/* This example accompanies the book
 "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
 ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

 Programs 4.4 through 4.12, section 4.2

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
 You may use, edit, run or distribute this file
 as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#include <string.h>
#include <stdint.h>
#include "OS.h"
#include "PLL.h"
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "UART.h"
#include "Interpreter.h"
#include "diskio.h"
#include "adc.h"
#include "heap.h"
#include "loader.h"
#include "ff.h"

/* File System Globals */
static FATFS OSfatFS;

#define NULL 0

/* Constants definitions */

#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
#define PF4                     (*((volatile uint32_t *)0x40025040))
#define PF0                     (*((volatile uint32_t *)0x40025004))
#define PF1                     (*((volatile uint32_t *)0x40025008))
#define PF2                     (*((volatile uint32_t *)0x40025010))
#define PF3                     (*((volatile uint32_t *)0x40025020))
#define SW1       0x10                      // on the left side of the Launchpad board
#define SW2       0x01                      // on the right side of the Launchpad board
#define NVIC_INT_CTRL_R         (*((volatile uint32_t *)0xE000ED04))
#define NVIC_INT_CTRL_SETPENDSV        0x10000000  // Set pending PendSV interrupt
#define NVIC_SYS_PRI3_R         (*((volatile uint32_t *)0xE000ED20))  // PendSV Handlers 23-21 Priority
#define MAX_UL_VAL            0xFFFFFFFF

/* Configurable definitions */

#define NUM_PRIORITIES    8
#define ROUND_ROBIN       0
#define FIFO_SIZE         4

/* Interpreter function definitions */
void log_dump(void);
void log_clear(void);
void jitter(void);

void insertThread(TCB *insert);
void removeThread(void);

/* Function prototypes in startup.c */
void DisableInterrupts(void);
void EnableInterrupts(void);
long StartCritical(void);
void EndCritical(long);
/* Function prototypes in osasm.s */
int GetHighestPriority(void);
void StartOS(void);

/* Global variables */

// Log Globals
log_entry LOG[LOG_MAX_ENTRY] = {0};

// Process globals
PCB *pcbList = 0;
int nextPId = 0;

// Thread Globals
TCB tcbs[NUM_THREADS];
TCB *RunPt = NULL;
// List of currently active threads, indexed by priority level (0-NUM_PRIORITIES-1)
// 0 is highest PL (priority level)
TCB *tcbLists[NUM_PRIORITIES];
// This variable will contain a 1 for each bit corresponding to a list in the above array that has at least one thread, 0 otherwise
#if NUM_PRIORITIES > 32
TODO: Fix the below variable
#endif
uint32_t threadActiveBitField = 0;
TCB *SleepPt = NULL;
int nextThreadId = 0;
int activeThreads = 0;
unsigned long time, mstime;
int timeslice;
int osStarted = 0;
int incTime = 0;

int interruptsEnabled = 0;

/* Round Robin Flag */
int roundRobin = 0;

/* Mailbox Globals */
sema_t mailBoxFree, mailDataValid;
int mailData;

/* Debugging Globals */
long waitStart, bWaitStart, signalStart, bSignalStart = 0;
long waitFinish, bWaitFinish, signalFinish, bSignalFinish = 0;

sema_t OPEN_FREE;
FIL file;


// Periodic Background Task Globals

#define JITTERSIZE 64
//Jitter
long MaxJitter1;             // largest time jitter between interrupts in usec
unsigned long JitterHistogram1[JITTERSIZE]={0,};
long MaxJitter2;             // largest time jitter between interrupts in usec
unsigned long JitterHistogram2[JITTERSIZE]={0,};

// Status of tasks
char t1_set = 0;
char t2_set = 0;

// Timing
volatile char t1_time_f = 0;
volatile char t2_time_f = 0;
volatile int t1_time = 0;
volatile int t2_time = 0;

// Periods
uint32_t period1 = 0;
uint32_t period2 = 0;

// User defined periodic tasks
void (*PeriodicTask1)(void);
void (*PeriodicTask2)(void);

void measureIntEn(void);
void measureIntDis(void);

char readBuffer[512];
char buffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];

char semafunc = 0;

// Symbol table
static const ELFSymbol_t symtab[] = {
  { "ST7735_Message", ST7735_Message },
  { "UART_OutString", UART_OutString },
  { "UART_InString", UART_InString },
  { "f_open", f_open },
  { "f_close", f_close },
  { "f_read", f_read },
  { "f_write", f_write },
  { "f_lseek", f_lseek },
};



// Aperiodic Background Task Globals

// SW1 Task Pt
void(*sw1Task)(void) = 0;

// SW2 Task Pt
void(*sw2Task)(void) = 0;

sema_t sw1ready, sw2ready;
int periodicTask1Pri, periodicTask2Pri;

// Port F initialization
void PortF_Init(void) {
  SYSCTL_RCGCGPIO_R |= 0x20;     // 1) activate Port F
  while((SYSCTL_PRGPIO_R&0x20) == 0){};// ready?
  // 2a) unlock GPIO Port F Commit Register
  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
  GPIO_PORTF_CR_R |= SW1 | SW2 | 0x04 | 0x08 | 0x2;  // 2b) enable commit for PF4, PF0, PF1(debugging)
  // 3) disable analog functionality on PF4, PF0, PF1
  GPIO_PORTF_AMSEL_R &= ~SW1 & ~SW2 & ~0x04 & ~0x08 & ~0x2;
  // 4) configure PF4, PF0, PF1 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFF0FF00)+0x00000000;
  GPIO_PORTF_DIR_R |= SW1 | SW2;// 5) make PF4, PF0, PF1 in (built-in buttons)
  GPIO_PORTF_DIR_R |=  0x04 | 0x08 | 0x02;
  // 6) disable alt funct on PF4, PF0, PF1
  GPIO_PORTF_AFSEL_R &= ~SW1 & ~SW2 & ~0x04 & ~0x08 & ~0x2;
   // delay = SYSCTL_RCGC2_R;        // put a delay here if you are seeing erroneous NMI
  // GPIO_PORTF_PUR_R |= SW1 | SW2; // enable weak pull-up on PF4, PF0
  GPIO_PORTF_DEN_R |= SW1 | SW2 | 0x04 | 0x08 | 0x2; // 7) enable digital I/O on PF4, PF0, PF1

  // GPIO_PORTF_IM_R |= SW1 | SW2 ;

  // Initially set priority to 3 (will be overwritten)
  NVIC_PRI3_R = (NVIC_PRI3_R & 0xFF00FFFF) | (3 << 21);
  NVIC_EN0_R |= 1 << 30;
}



// Trigger Context Switch
void triggerPendSV(void) {
  if(osStarted)
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_SETPENDSV;
}

void OS_DisableInterrupts(void) {
  DisableInterrupts();
  measureIntDis();
}

void OS_EnableInterrupts(void) {
  measureIntEn();
  EnableInterrupts();
}

long OS_StartCritical(void) {
  long sr = StartCritical();
  measureIntDis();
  return sr;
}

void OS_EndCritical(long sr) {
  measureIntEn();
  EndCritical(sr);
}

/* OS Functions */

void OS_Idle() {
  while (1) {
    // PF1 ^= 0x2;
  }
}

// Interpreter Functions for FATFS

// Launch an ELF process
void open(void) {
  ELFEnv_t env = { symtab, 8 };
  IT_GetBuffer(buffer);
  OS_bWait(&OPEN_FREE);
  UART_OutString("\n\r");
  // Loader funtions
  UART_SetColor(CYAN);
  if (exec_elf(buffer[0], &env) == -1) {
    UART_OutError("\n\rERROR: file not found");
  }
  UART_SetColor(RESET);
  OS_bSignal(&OPEN_FREE);
}

// Create new file
void touch(void) {
  int code;
  IT_GetBuffer(buffer);
  code = f_open(&file, buffer[0], FA_CREATE_ALWAYS);
  if (code) {
    UART_OutError("\n\rERROR ");
    UART_SetColor(RED);
    UART_OutUDec(code);
    UART_SetColor(RESET);
    UART_OutError(": File not created");
  }
  f_close(&file);
}

// Remove file
void rm(void) {
  int code;
  IT_GetBuffer(buffer);
  code = f_unlink(buffer[0]);
	if (code) {
    UART_OutError("\n\rERROR ");
    UART_SetColor(RED);
    UART_OutUDec(code);
    UART_SetColor(RESET);
    UART_OutError(": File not found");
    return;
  }
}

// Print file
void cat(void) {
  int code, i;
  char out;
  unsigned int bytesRead = 0;
  IT_GetBuffer(buffer);
  for (i = 0; i < 512; i++) {
    readBuffer[i] = 0;
  }

  code = f_open(&file, buffer[0], FA_READ);
  if(code) {
    UART_OutError("\n\rERROR ");
    UART_SetColor(RED);
    UART_OutUDec(code);
    UART_SetColor(RESET);
    UART_OutError(": File not found");
    return;
  }
  UART_OutError("\n\r");
  while (f_read(&file, &out, 1, &bytesRead) == FR_OK && bytesRead == 1) {
    if (bytesRead != 1)
      break;
    UART_OutChar2(out);
  }
  f_close(&file);
}

// Format disk
void df(void) {
  int code;
  // NO CLUE HOW TO DO THIS
  code = f_mkfs("", 1, 512);
  if(code) {
    UART_OutError("\n\rERROR ");
    UART_SetColor(RED);
    UART_OutUDec(code);
    UART_SetColor(RESET);
    UART_OutError(": SD not formatted");
    return;
  }
}

void ls(void) {
  DIR cwd;
  //char cwdName[13];
  FILINFO f;
  int i;
  //f_getcwd(cwdName, 12);
  f_opendir(&cwd, "/");
  UART_OutString("\n\rName");
  for(i =0; i < 15-4; i++) {
    UART_OutString(" ");
  }
  UART_OutString("Size(bytes)\r\n\r\n");
  f_readdir(&cwd, &f);
  while(f.fname[0] != 0) {
    UART_OutString(f.fname);
    for(i = 0; i < 15-strlen(f.fname); i++) {
      UART_OutString(" ");
    }
    UART_OutUDec(f.fsize);
    UART_OutString("\r\n");
    f_readdir(&cwd, &f);
  }
  f_closedir(&cwd);
}

void systime(void) {
  UART_OutString("\n\r  Time: ");
  UART_OutUDec(OS_Time()/timeslice);
  UART_OutString(" ms ");
}

void OS_FsInit(void) {
	if(f_mount(&OSfatFS, "", 0)) {
		UART_OutString("Error mounting file system.");
    while(1);
  }
  // Add interpreter commands
  IT_AddCommand("touch", 1, "[name]", &touch, "create new file");
  IT_AddCommand("rm", 1, "[name]", &rm, "delete file");
  IT_AddCommand("cat", 1, "[name]", &cat, "print file");
  IT_AddCommand("df", 0, "", &df, "format disk");
  IT_AddCommand("ls", 0, "", &ls, "print disk directory");
  IT_AddCommand("open", 1, "[name]", &open, "launch ELF file");

	OS_Kill();
}

/** Initialize operating system, disable interrupts until OS_Launch
 * Initialize OS controlled I/O: pendsv, 80 MHz PLL
 */
void OS_Init(void){
  cmd_t *cmd;
  DisableInterrupts();
  PLL_Init(Bus80MHz);         // set processor clock to 80 MHz
  PortF_Init();
	ST7735_InitR(INITR_REDTAB);
	UART_Init();
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0xFF00FFFF)|(7 << 21); // PendSV = priority 7
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|(0 << 28); // SysTick = priority 0
  NVIC_SYS_PRI2_R =(NVIC_SYS_PRI2_R&0x00FFFFFF)|(3 << 28);

  //OS_AddPeriodicThread(&disk_timerproc, 10*TIME_1MS, 1);

	Heap_Init();

  cmd = IT_AddCommand("log", 0, "", &log_dump, "dump log entries");
  IT_AddFlag(cmd, 'c', 0, "" , &log_clear, "clear log");
  log_clear();

  cmd = IT_AddCommand("jitter", 0, "", &jitter, "show jitter");
  cmd = IT_AddCommand("time", 0, "", &systime, "show time");

  OS_InitSemaphore(&OPEN_FREE, 1);

  ADC_InitIT();
  OS_AddThread(&OS_Idle,128,NUM_PRIORITIES-1);
  OS_AddThread(&Interpreter,128, NUM_PRIORITIES-2);
	//OS_AddThread(&OS_FsInit, 256, 0);
  //OS_Fifo_Init(256);
}

/** Start the scheduler, enable interrupts. Does not return.
 * @param timeSlice number of 20ns clock cycles for each time slice
 */
void OS_Launch(uint32_t timeSlice){
  if (RunPt != 0) {
    timeslice = timeSlice;
		mstime = 0;
    time = 0;
    NVIC_ST_RELOAD_R = timeSlice - 1; // reload value
    NVIC_ST_CTRL_R = 0x00000007; // enable, core clock and interrupt arm

    osStarted = 1;

    StartOS();
  } else {
    while(1);
  }
}

/* Foreground Thread Functions */

void SetInitialStack(TCB *tcb, void (*task)(void)){
    // Allocate memory for stack
    tcb->sp = &tcb->stack[0];
    tcb->sp[tcb->stackSize-1] = 0x01000000;   // thumb bit
    tcb->sp[tcb->stackSize-2] = (int)task;         // Set PC to task pointer
    tcb->sp[tcb->stackSize-3] = 0x14141414;   // R14 (LR) - Maybe set this to &OS_Kill
    tcb->sp[tcb->stackSize-4] = 0x12121212;   // R12
    tcb->sp[tcb->stackSize-5] = 0x03030303;   // R3
    tcb->sp[tcb->stackSize-6] = 0x02020202;   // R2
    tcb->sp[tcb->stackSize-7] = 0x01010101;   // R1
    tcb->sp[tcb->stackSize-8] = 0x00000000;   // R0
    tcb->sp[tcb->stackSize-9] = 0x11111111;   // R11
    tcb->sp[tcb->stackSize-10] = 0x10101010;  // R10
    tcb->sp[tcb->stackSize-11] = 0x09090909;  // R9
    tcb->sp[tcb->stackSize-12] = 0x08080808;  // R8
    tcb->sp[tcb->stackSize-13] = 0x07070707;  // R7
    tcb->sp[tcb->stackSize-14] = 0x06060606;  // R6
    tcb->sp[tcb->stackSize-15] = 0x05050505;  // R5
    tcb->sp[tcb->stackSize-16] = 0x04040404;  // R4
    tcb->sp += tcb->stackSize - 16;
}

/** Add one foreground thread to the scheduler
* @param task pointer to task
* @param stackSize memory to be allocated in the stack
* @param priority priority of the thread to be added
* @return 1 if the thread was successfully added, 0 if not
*/
int OS_AddThread(void(*task0)(void), int stackSize, int priority) {
  TCB *newTCB;
  int tcbI, i;
  long sr = OS_StartCritical();
  if(activeThreads >= NUM_THREADS) {
 		OS_EndCritical(sr);
    return 0;
  }
  // Find index of open TCB
  tcbI = -1;
  for(i = 0; i < NUM_THREADS; i++) {
    if(tcbs[i].inUse == 0) {
      tcbI = i;
      break;
    }
  }
  if(tcbI == -1) {
 		OS_EndCritical(sr);
    return 0;
  }
  newTCB = &tcbs[tcbI];
  // Set TCB stack size
  newTCB->stackSize = STACK_SIZE;
  // Assign new thread ID
  newTCB->id = nextThreadId++;
  // Set priority
  newTCB->priority = priority;
  // Set parent
  if (RunPt == 0) {
    newTCB->parent = 0;
  } else {
    newTCB->parent = RunPt->parent;
  }
  if (newTCB->parent != 0) {
    newTCB->parent->numThreads++;
  }
  if(newTCB->priority < 0 || newTCB->priority >= NUM_PRIORITIES) {
   OS_EndCritical(sr);
   return 0;
  }

  insertThread(newTCB);



  // Initialize remaining variables
  newTCB->inUse = 1;
  newTCB->isBlocked = 0;
  newTCB->sleepTime = 0;
  newTCB->deleteThis = 0;

  // Allocate space for and initialize the stack
  SetInitialStack(newTCB, task0);
  if (newTCB->parent != 0) {
    newTCB->stack[newTCB->stackSize-11] = newTCB->parent->data;
  }

  activeThreads++;

  OS_EndCritical(sr);
  return 1;
}

int OS_AddProcess(void (*entry)(void), void *code, void *data, int stackSize, int priority) {
  PCB *newPCB;
  long sr = OS_StartCritical();
  PF0 ^= 0x01;
  newPCB = Heap_Malloc(sizeof(PCB));
  if (newPCB == 0) {
    Heap_Free(code);
    Heap_Free(data);
    OS_EndCritical(sr);
    return 0;
  }

  if (!OS_AddThread(entry, stackSize, priority)) {
    Heap_Free(newPCB);
    Heap_Free(code);
    Heap_Free(data);
    OS_EndCritical(sr);
    return 0;
  }

  // Add content
  newPCB->pid = nextPId++;
  newPCB->code = code;
  newPCB->data = data;
  newPCB->numThreads = 1
  ;

  // Add to list
  if (pcbList == 0) {
    pcbList = newPCB;
    newPCB->next = newPCB;
    newPCB->prev = newPCB;
  } else {
    newPCB->next = pcbList->next;
    newPCB->prev = pcbList;
    pcbList->next->prev = newPCB;
    pcbList->next = newPCB;
  }

  // Set parent
  if (RunPt->priority == priority) {
    RunPt->next->parent = newPCB;
    RunPt->next->stack[RunPt->next->stackSize-11] = (int)data;
  } else {
    tcbLists[priority]->prev->parent = newPCB;
    tcbLists[priority]->prev->stack[tcbLists[priority]->prev->stackSize-11] = (int)data;
  }
  OS_EndCritical(sr);

  UART_OutStringColor("Process added", YELLOW);
  return 1;
}

void removeThread(void) {
  long sr = OS_StartCritical();
  // If it's not the only thread in its list
  if(RunPt->next != RunPt) {
    // Update priority list pointer if removing first element
    if(tcbLists[RunPt->priority] == RunPt)
      tcbLists[RunPt->priority] = RunPt->next;
    RunPt->prev->next = RunPt->next;
    RunPt->next->prev = RunPt->prev;
    if(RunPt->next->next == RunPt->next) {
      roundRobin = 0; // Disable round robin if only one thread left in list
    } else {
      roundRobin = 1;
    }
  } else {
    tcbLists[RunPt->priority] = 0;
    threadActiveBitField &= ~(1 << RunPt->priority);
    if(tcbLists[GetHighestPriority()]->next != tcbLists[GetHighestPriority()]) {
      // Enable round robin
      roundRobin = 1;
    }
    else {
      // Disable round robin
      roundRobin = 0;
    }
    RunPt->next = tcbLists[GetHighestPriority()];
  }
  OS_EndCritical(sr);
}

void insertThread(TCB *insert) {
  long sr = OS_StartCritical();

  if(RunPt == 0)
    RunPt = insert;

  // Insert into priority level linked list
  if(tcbLists[insert->priority] != 0) {
    // If new thread is same priority as running thread

    if(insert->priority == RunPt->priority) {
      // Activate round robin
      roundRobin = 1;

      insert->next = RunPt->next;
      insert->prev = RunPt;
      RunPt->next->prev = insert;
      RunPt->next = insert;
    } else {
      insert->prev = tcbLists[insert->priority]->prev;
      insert->next = tcbLists[insert->priority];
      tcbLists[insert->priority]->prev->next = insert;
      tcbLists[insert->priority]->prev = insert;
    }

  } else {

    // Deactivate round robin
    roundRobin = 0;

    // Indicate at least one thread is active for that PL in bit field
    threadActiveBitField |= 1 << insert->priority;

    tcbLists[insert->priority] = insert;
    insert->prev = insert;
    insert->next = insert;

    if(insert->priority < RunPt->priority) {
      triggerPendSV();
      if(osStarted == 0)
        RunPt = insert;
    }

  }

  OS_EndCritical(sr);
}

void OS_Kill(void) {
  OS_DisableInterrupts();
  OS_LogEntry(LOG_THREAD_KILL);
  removeThread();

  if (RunPt->parent != 0){
    RunPt->parent->numThreads--;
    if (RunPt->parent->numThreads == 0) {
      if (RunPt->parent->prev == RunPt->parent) {
        pcbList = 0;
      } else {
        RunPt->parent->prev->next = RunPt->parent->next;
        RunPt->parent->next->prev = RunPt->parent->prev;
      }
      Heap_Free(RunPt->parent->code);
      Heap_Free(RunPt->parent->data);
      Heap_Free(RunPt->parent);
      PF0 ^= 0x01;
    }
  }

  RunPt->deleteThis = 1;
  activeThreads--;

  OS_Suspend();
}

void sleep_decrement() {
  TCB *current, *awakeTCB, *prev, *last;
  long sr = OS_StartCritical();
  current = SleepPt;
  if (current != NULL) {
    last = SleepPt->prev;
    do {
      prev = current;
      current->sleepTime -= timeslice/TIME_1MS;
			// Take it out of the list
      if (current->sleepTime <= 0) {
				current->sleepTime = 0;
				if (current == SleepPt) {
					if (current != last) {
						SleepPt = SleepPt->next;
						SleepPt->prev = current->prev;
					} else {
						SleepPt = NULL;
					}
				} else {
					current->prev->next = current->next;
					if (current != SleepPt->prev) {
						current->next->prev = current->prev;
					} else {
						SleepPt->prev = SleepPt->prev->prev;
            last = SleepPt->prev;
            prev = last;
					}
				}

        awakeTCB = current;
        current = current->next;
        insertThread(awakeTCB);
        continue;
      }
      current = current->next;
    } while (prev != last);
  }
  OS_EndCritical(sr);
}

void OS_Sleep(int duration) {
  TCB *last;
  OS_DisableInterrupts();
  removeThread();

  // Add to sleep list
  if(SleepPt != NULL) {
    last = SleepPt->prev;
    last->next = RunPt;
    RunPt->prev = last;
  } else {
    SleepPt = RunPt;
  }
	SleepPt->prev = RunPt;

  // Set sleep counter
  RunPt->sleepTime = duration;

  OS_Suspend();
}

void OS_Suspend(void) {
    if(activeThreads <= 1) {
      OS_EnableInterrupts();
      return;
    }
    triggerPendSV();
    OS_EnableInterrupts();
}

int OS_Id(void) {
    return RunPt->id;
}

/* Aperiodic Background Thread Functions */

// Add task to be executed when SW1 is pressed
void OS_AddSW1Task(void(*task)(void), int pri) {
  OS_InitSemaphore(&sw1ready, 1);
  sw1Task = task;
	NVIC_PRI3_R = (NVIC_PRI3_R & 0xFF00FFFF) | (pri << 21);
}

// Add task to be executed when SW2 is pressed
void OS_AddSW2Task(void(*task)(void), int pri) {
  OS_InitSemaphore(&sw2ready, 1);
  sw2Task = task;
  NVIC_PRI3_R = (NVIC_PRI3_R & 0xFF00FFFF) | (pri << 21);
}

void triggerSW1(void) {
  if((sw1Task != 0)) {
    OS_Sleep(30); // sleep 30ms
    if(PF4 != 0) {
      GPIO_PORTF_IM_R |= SW1;
      OS_Kill();
    }
    OS_bWait(&sw1ready);
    (*sw1Task)();
    OS_Sleep(100); // sleep 100ms
    OS_bSignal(&sw1ready);
  }
	GPIO_PORTF_IM_R |= SW1;
	OS_Kill();
}

void triggerSW2(void) {
  if((sw2Task != 0)) {
    OS_Sleep(30); // sleep 30ms
    if((PF0) != 0) {
      GPIO_PORTF_IM_R |= SW2;
      OS_Kill();
    }
    OS_bWait(&sw2ready);
    (*sw2Task)();
    OS_Sleep(100); // sleep 100ms
    OS_bSignal(&sw2ready);
  }
	GPIO_PORTF_IM_R |= SW2;
	OS_Kill();
}

// Interrupt handler for GPIO Port F (PF4 = SW1, PF0 = SW2)
void GPIOPortF_Handler(void) {
  // Check for SW1
  if((GPIO_PORTF_RIS_R & SW1) == SW1) {
    GPIO_PORTF_ICR_R |= SW1;
		GPIO_PORTF_IM_R &= ~SW1;
    if(OS_AddThread(&triggerSW1, 128, 0) == 0)
			GPIO_PORTF_IM_R |= SW1;
  }
  // Check for SW2
  else if((GPIO_PORTF_RIS_R & SW2) == SW2) {
    GPIO_PORTF_ICR_R |= SW2;
		GPIO_PORTF_IM_R &= ~SW2;
    if(OS_AddThread(&triggerSW2, 128, 0) == 0)
			GPIO_PORTF_IM_R |= SW2;
  }
}

/* Time Functions */

long OS_Time(void) {
  return (timeslice - 1 - NVIC_ST_CURRENT_R) + time*timeslice;
}

unsigned long OS_MsTime(void) {
    return mstime*timeslice/TIME_1MS;
}

void OS_ClearMsTime(void) {
    mstime = 0;
}

unsigned long OS_TimeDifference(unsigned long lastTime, unsigned long thisTime) {
	if (lastTime > thisTime) {
    unsigned long res = (((unsigned long long)thisTime + MAX_UL_VAL) - (lastTime));
		return res;
  }
	return thisTime - lastTime;
}

long maxTimeIntDis = 0; // Maximum time with interrupts disabled in units of clock period (12.5ns)
long timeIntDis, firstTimeID = 0; // Total time with interrupts disabled
long timeIntEn, firstTimeIE = 0;  // Total time with interrupts enabled

/* Interrupts Disabled Time Measurements */

void OS_ClearIntDisTimes(void) {
  maxTimeIntDis = 0;
  timeIntDis = timeIntEn = 0;
}

void measureIntDis(void) {
  long timeEn = 0;
  interruptsEnabled = 0;
  firstTimeID = OS_Time(); // Units of 12.5ns
  if(firstTimeIE < firstTimeID)
    timeEn = firstTimeID - firstTimeIE;
  else
    return;
  timeIntEn += timeEn;
}

void measureIntEn(void) {
  long timeDis = 0;
  interruptsEnabled = 1;
  firstTimeIE = OS_Time(); // Units of 12.5ns
  if(firstTimeID < firstTimeIE)
    timeDis = firstTimeIE - firstTimeID;
  else
    return;
  timeIntDis += timeDis;
  if(timeDis > maxTimeIntDis)
    maxTimeIntDis = timeDis;
}

/**
 * Calculates interrupt disabled percentage
 * @returns percentage of time interrupts are disabled (in fixed point format with resolution of .1%)
 */
long OS_GetPercIntDis(void) {
  return ((timeIntDis) * 1000) / (timeIntDis + timeIntEn);
}

/**
 * Calculates max time with interrupts disabled
 * @returns maximum time interrupts are disabled (in units of clock cycles)
 */
long OS_GetMaxTimeIntDis(void) {
  return maxTimeIntDis;
}


//MAILBOX FUNCTIONS

void OS_MailBox_Send(int data) {
  OS_bWait(&mailBoxFree);
  mailData = data;
  OS_bSignal(&mailDataValid);
}

int OS_MailBox_Recv(void) {
  int data;
  OS_bWait(&mailDataValid);
  data = mailData;
  OS_bSignal(&mailBoxFree);
  return data;
}

void OS_MailBox_Init(void) {
  OS_InitSemaphore(&mailBoxFree, 1);
  OS_InitSemaphore(&mailDataValid, 0);
}



/* FIFO Globals */
sema_t fifoFree, fifoSpaceAvail, fifoSpaceTaken;
int fifo[FIFO_SIZE];
int fifoSize;
int fifoPutI;
int fifoGetI;

/* FIFO Functions */
// Size in words
int OS_Fifo_Init(int size) {
  fifoSize = FIFO_SIZE;
  fifoPutI = fifoGetI = 0;
  OS_InitSemaphore(&fifoFree, 1);
  OS_InitSemaphore(&fifoSpaceAvail, fifoSize);
  OS_InitSemaphore(&fifoSpaceTaken, 0);
  return fifo == 0;
}

// 1 for error, 0 for success
int OS_Fifo_Put(int val) {
	OS_Wait(&fifoSpaceAvail);
  OS_bWait(&fifoFree);

  fifo[fifoPutI] = val;
  fifoPutI = (fifoPutI + 1) % fifoSize;

  OS_bSignal(&fifoFree);
	OS_Signal(&fifoSpaceTaken);
  return 1;
}

// -1 for error, value for success
int OS_Fifo_Get(void) {
  int res;
	OS_Wait(&fifoSpaceTaken);
  OS_bWait(&fifoFree);

  res = fifo[fifoGetI];
  fifoGetI = (fifoGetI + 1) % fifoSize;

  OS_bSignal(&fifoFree);
	OS_Signal(&fifoSpaceAvail);
  return res;
}

void SysTick_Handler(void) {
  long sr = OS_StartCritical();
  triggerPendSV();
  // Count bit is set, should always be set
  time++;
  mstime++;
  sleep_decrement();
  OS_EndCritical(sr);
}

void * doServiceCall(int svcNum, void * arg0, void * arg1, void * arg2) {
  void *res = 0;
  PF4 ^= 0x10;
  switch(svcNum) {
    case 0:
      res = (void *) OS_AddThread((void(*)(void))(arg0), (int)arg1, (int)arg2); // SVC Call 0
      break;
    case 1:

      OS_Suspend(); // SVC Call 1
      break;

    case 2:

      OS_Kill();  // SVC Call 2
      break;

    case 3:
      res = (void *) OS_Time(); // SVC Call 3
      break;
    case 4:
      res = (void *) OS_TimeDifference((unsigned long)arg0, (unsigned long)arg1); // SVC Call 4
      break;
    case 5:
      res = (void *) OS_MsTime(); // SVC Call 5
      break;
    case 6:

      OS_ClearMsTime(); // SVC Call 6
      break;

    case 7:

      OS_Sleep((int) arg0); // SVC Call 7
      break;

    case 8:

      OS_Wait((sema_t *) arg0); // SVC Call 8
      break;

    case 9:

      OS_bWait((sema_t *) arg0); // SVC Call 9
      break;

    case 10:

      OS_Signal((sema_t *) arg0); // SVC Call 10
      break;

    case 11:

      OS_bSignal((sema_t *) arg0); // SVC Call 11
      break;

    case 12:
      res = (void *) OS_Id(); // SVC Call 12
      break;
  }

  PF4 ^= 0x10;
  return res;
}

/* Semaphore Functions */

void OS_InitSemaphore(sema_t *s, int val) {
    s->value = val;
    s->max = val;
    s->blocked = NULL;
		s->who = NULL;
}

void OS_bWait(sema_t *s) {
  TCB *aux;
	long sr = OS_StartCritical();
  semafunc = 1;
  bWaitStart = OS_Time();
	aux = s->blocked;
	s->value--;

	if (s->value == 0)
		s->who = RunPt;

	if (s->value < 0) {
    RunPt->isBlocked = 1;
	  // Take it out of the round robin
    removeThread();
    semafunc = 11;

		// Add it to the blocked list.
    // Keep the next pointer.
    // Make the first prev pointer point to the end of the list.

		if (aux == NULL) {
			s->blocked = RunPt;
			s->blocked->prev = RunPt;
		} else {
      s->blocked->prev->next = RunPt;
      RunPt->prev = s->blocked->prev;
      s->blocked->prev = RunPt;
		}
		triggerPendSV();
	}
  bWaitFinish = OS_Time();
	OS_EndCritical(sr);
}

void OS_bSignal(sema_t *s) {
  TCB *aux;
	long sr;
  TCB *highest;
  bSignalStart = OS_Time();
	sr = OS_StartCritical();
  semafunc = 2;
  if (s->value < 1) {
    s->value++;
  }

	if (s->value == 1)
		s->who = NULL;

	if (s->value <= 0 && s->blocked != NULL) {
		highest = s->blocked;
		aux = s->blocked;

		// Run through the list using aux. Only update highest if aux is of higher priority
		while (aux != s->blocked->prev) {
			aux = aux->next;
			if (aux->priority < highest->priority) {
				highest = aux;
			}
		}

		if (s->blocked->prev == s->blocked) { //only
			s->blocked = NULL;
		} else if (highest == s->blocked->prev) { //last
			s->blocked->prev = s->blocked->prev->prev;
		} else if (s->blocked == highest) { //first
			s->blocked = s->blocked->next;
			highest->next->prev = highest->prev;
		} else {
			highest->next->prev = highest->prev;
			highest->prev->next = highest->next;
		}

    highest->isBlocked = 0;
    insertThread(highest);
    semafunc = 21;
		s->who = highest;
	}
  bSignalFinish = OS_Time();
	OS_EndCritical(sr);
}

void OS_Wait(sema_t *s) {
  TCB *aux;
	long sr;
  semafunc = 3;
  waitStart = OS_Time();
	sr = OS_StartCritical();
	aux = s->blocked;
	s->value--;

	if (s->value == 0)
		s->who = RunPt;

	if (s->value < 0) {
    RunPt->isBlocked = 1;
		// Take it out of the round robin
    removeThread();
    semafunc = 31;

		// Add it to the blocked list.
    // Keep the next pointer.
    // Make the first prev pointer point to the end of the list.

		if (aux == NULL) {
			s->blocked = RunPt;
			s->blocked->prev = RunPt;
		} else {
      s->blocked->prev->next = RunPt;
      RunPt->prev = s->blocked->prev;
      s->blocked->prev = RunPt;
		}
		triggerPendSV();
	}

  waitFinish = OS_Time();
	OS_EndCritical(sr);
}

void OS_Signal(sema_t *s) {
  TCB *aux, *highest;
	long sr;
  semafunc = 4;
  signalStart = OS_Time();
	sr = OS_StartCritical();
  s->value++;

	if (s->value > 0)
		s->who = NULL;

	if (s->value <= 0 && s->blocked != NULL) {
		highest = s->blocked;
		aux = s->blocked;

		// Run through the list using aux. Only update highest if aux is of higher priority
		while (aux != s->blocked->prev) {
			aux = aux->next;
			if (aux->priority < highest->priority) {
				highest = aux;
			}
		}

		if (s->blocked->prev == s->blocked) { //only
			s->blocked = NULL;
		} else if (highest == s->blocked->prev) { //last
			s->blocked->prev = s->blocked->prev->prev;
		} else if (s->blocked == highest) { //first
			s->blocked = s->blocked->next;
			highest->next->prev = highest->prev;
		} else {
			highest->next->prev = highest->prev;
			highest->prev->next = highest->next;
		}

    highest->isBlocked = 0;
		insertThread(highest);
    semafunc = 41;
		s->who = highest;
	}
  signalFinish = OS_Time();
	OS_EndCritical(sr);
}


/** Execute a software task at a periodic rate.
 * @param task Pointer to a function to be executed periodically
 * @param period Interval in ms for periodic execution
 * @param priority Priority for thread execution
 * @return thread number for success or OS_THREAD_ADD_ERROR for error
 */
int OS_AddPeriodicThread(void(*task)(void), uint32_t period, uint32_t priority) {
    // Check if first thread is already configured
    if (t1_set == 0) {
        // Store periodic task
        PeriodicTask1 = task;

        // Store period
        period1 = period;

        // Activate TIMER4: SYSCTL_RCGCTIMER_R bit 5
        SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R4;

        // 1) Ensure the timer is disabled (the TnEN bit in the GPTMCTL register
        // is cleared) before making any changes
        TIMER4_CTL_R &= ~TIMER_CTL_TAEN;

        // 2) Write the GPTM Configuration Register (GPTMCFG) with a value of
        // 0x0000.0000.
        TIMER4_CFG_R = TIMER_CFG_32_BIT_TIMER;

        // 3) Configure the TnMR field in the GPTM Timer n Mode Register (GPTMTnMR):
        //   a. Write a value of 0x1 for One-Shot mode.
        //   b. Write a value of 0x2 for Periodic mode.
        TIMER4_TAMR_R = TIMER_TAMR_TAMR_PERIOD;

        // 4) Optional
        // Bus clock resolution (prescaler)
        TIMER4_TAPR_R = 0x00;
        // Clear TIMER4A timeout flag
        TIMER4_ICR_R |= TIMER_ICR_TATOCINT;

        // 5) Load the start value into the GPTM Timer n Interval Load Register
        // (GPTMTnILR).
        TIMER4_TAILR_R = period - 1;

        // 6) If interrupts are required, set the appropriate bits in the GPTM
        // Interrupt Mask Register (GPTMIMR)
        TIMER4_IMR_R |= TIMER_IMR_TATOIM;

        // Priority
        NVIC_PRI17_R = (NVIC_PRI17_R&0xFF00FFFF)|(priority<<21);
        // interrupts enabled in the main program after all devices initialized
        // vector number 86, interrupt number 70
        // Enable IRQ 70 in NVIC
        NVIC_EN2_R = 1<<(70-2*32);


        // 7) Set the TnEN bit in the GPTMCTL register to enable the timer and
        // start counting.
        TIMER4_CTL_R |= TIMER_CTL_TAEN;

        t1_set = 1;
        return OS_THREAD_1;
    } else if (t2_set == 0) { // Check if second thread is already configured
        // Store periodic task
        PeriodicTask2 = task;

        // Store period
        period2 = period;

        // Activate TIMER4: SYSCTL_RCGCTIMER_R bit 6
        SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R5;

        // 1) Ensure the timer is disabled (the TnEN bit in the GPTMCTL register
        // is cleared) before making any changes
        TIMER5_CTL_R &= ~TIMER_CTL_TAEN;

        // 2) Write the GPTM Configuration Register (GPTMCFG) with a value of
        // 0x0000.0000.
        TIMER5_CFG_R = TIMER_CFG_32_BIT_TIMER;

        // 3) Configure the TnMR field in the GPTM Timer n Mode Register (GPTMTnMR):
        //   a. Write a value of 0x1 for One-Shot mode.
        //   b. Write a value of 0x2 for Periodic mode.
        TIMER5_TAMR_R = TIMER_TAMR_TAMR_PERIOD;

        // 4) Optional
        // Bus clock resolution (prescaler)
        TIMER5_TAPR_R = 0x00;
        // Clear TIMER4A timeout flag
        TIMER5_ICR_R |= TIMER_ICR_TATOCINT;

        // 5) Load the start value into the GPTM Timer n Interval Load Register
        // (GPTMTnILR).
        TIMER5_TAILR_R = period - 1;

        // 6) If interrupts are required, set the appropriate bits in the GPTM
        // Interrupt Mask Register (GPTMIMR)
        TIMER5_IMR_R |= TIMER_IMR_TATOIM;

        // Priority
        NVIC_PRI23_R = (NVIC_PRI23_R&0xFFFFFF00)|(priority<<5);
        // interrupts enabled in the main program after all devices initialized
        // vector number 108, interrupt number 92
        // Enable IRQ 92 in NVIC
        NVIC_EN2_R = 1<<(92-2*32);

        // 7) Set the TnEN bit in the GPTMCTL register to enable the timer and
        // start counting.
        TIMER5_CTL_R |= TIMER_CTL_TAEN;

        t2_set = 1;
        return OS_THREAD_2;
    }

    return OS_THREAD_ADD_ERROR;
}


void Timer4A_Handler(void){
    // 8) Poll the GPTMRIS register or wait for the interrupt to be generated
    // (if enabled). In both cases, the status flags are cleared by writing a 1
    // to the appropriate bit of the GPTM Interrupt Clear Register (GPTMICR)
    unsigned static long LastTime = 0;  // time at previous ADC sample
  	unsigned long thisTime, diff;         // time at current ADC sample
  	long jitter;
    TIMER4_ICR_R |= TIMER_ICR_TATOCINT;// acknowledge TIMER4 timeout
    if(osStarted == 0)
      return;
    if (PeriodicTask1 != 0) {
      OS_LogEntry(LOG_PERIODIC_START);
      LOG[0].thread = OS_THREAD_1;
      (*PeriodicTask1)();
      OS_LogEntry(LOG_PERIODIC_FINISH);
      LOG[0].thread = OS_THREAD_1;
      thisTime = OS_Time();
      if (LastTime > 0) {    // ignore timing of first interrupt
        diff = OS_TimeDifference(LastTime,thisTime);
        if (diff > period1){
          jitter = (diff-period1+4)/8;  // in 0.1 usec
        } else {
          jitter = (period1-diff+4)/8;  // in 0.1 usec
        }
        if (jitter > MaxJitter1){
          MaxJitter1 = jitter; // in usec

        }       // jitter should be 0
        if (jitter >= (unsigned long)(JITTERSIZE)){
          jitter = JITTERSIZE-1;
        }
        JitterHistogram1[jitter]++;
      }
      LastTime = thisTime;
    }
}


void Timer5A_Handler(void){
  // 8) Poll the GPTMRIS register or wait for the interrupt to be generated
  // (if enabled). In both cases, the status flags are cleared by writing a 1
  // to the appropriate bit of the GPTM Interrupt Clear Register (GPTMICR)
  unsigned static long LastTime = 0;  // time at previous ADC sample
	unsigned long thisTime, diff;         // time at current ADC sample
	long jitter;
  TIMER5_ICR_R |= TIMER_ICR_TATOCINT;// acknowledge TIMER5 timeout
  if(osStarted == 0)
    return;
  if (PeriodicTask2 != 0) {
    thisTime = OS_Time();
    OS_LogEntry(LOG_PERIODIC_START);
    LOG[0].thread = OS_THREAD_2;
    (*PeriodicTask2)();
    OS_LogEntry(LOG_PERIODIC_FINISH);
    LOG[0].thread = OS_THREAD_2;
    if (LastTime > 0) {    // ignore timing of first interrupt
      diff = OS_TimeDifference(LastTime,thisTime);
      if (diff > period2){
        jitter = (diff-period2+4)/8;  // in 0.1 usec
      } else {
        jitter = (period2-diff+4)/8;  // in 0.1 usec
      }
      if (jitter > MaxJitter2){
        MaxJitter2 = jitter; // in usec
      }       // jitter should be 0
      if (jitter >= (unsigned long)(JITTERSIZE)){
        jitter = JITTERSIZE-1;
      }
      JitterHistogram2[jitter]++;
    }
    LastTime = thisTime;
  }
}


long OS_GetJitter(unsigned char bthread) {
  if (bthread == OS_THREAD_1)
    return MaxJitter1;
  if (bthread == OS_THREAD_2)
    return MaxJitter2;
  return 0;
}

void jitter() {
	UART_OutString("\r\n  Jitter1: ");
	UART_OutUDec(OS_GetJitter(OS_THREAD_1));
	UART_OutString("\r\n  Jitter2: ");
	UART_OutUDec(OS_GetJitter(OS_THREAD_2));
}

/** Read current value of timer counter
 * @param thread Number of thread to be accessed: OS_THREAD_1 or OS_THREAD_2
 * @return current time (ns) since last interrupt
 */
uint32_t OS_ReadPeriodicTime(unsigned char thread) {
    if (thread == OS_THREAD_1) {
        return (TIMER4_TAV_R + 1) * 12.5;
    } if (thread == OS_THREAD_2) {
        return (TIMER5_TAV_R + 1) * 12.5;
    }
    return 0;
}


/** Reset the 32-bit counter to 0 (of the timers executing the tasks).
 */
void OS_ClearPeriodicTime(void) {
    TIMER4_TAV_R = period1;
    TIMER5_TAV_R = period2;
}

/** Remove a configured software task
 * @param thread Number of thread to be removed: OS_THREAD_1 or OS_THREAD_2
 */
void OS_RemovePeriodicThread(unsigned char thread){
  if (thread == OS_THREAD_1) {
    PeriodicTask1 = 0;
    TIMER4_CTL_R &= ~TIMER_CTL_TAEN;
    t1_set = 0;
  } else if (thread == OS_THREAD_2) {
    PeriodicTask2 = 0;
    TIMER5_CTL_R &= ~TIMER_CTL_TAEN;
    t2_set = 0;
  }
}

int OS_RunningThreads(void) {
    return t1_set + t2_set;
}


/** Time how long the thread is active
 * @param thread Number of thread to be accessed: OS_THREAD_1 or OS_THREAD_2
 */
void OS_EnableTiming(unsigned char thread) {
  if (thread == OS_THREAD_1) {
    t1_time_f = 1;
    t1_time = 0;
  } if (thread == OS_THREAD_2) {
    t2_time_f = 1;
    t2_time = 0;
  }
}

/** Disable timing options
 * @param thread Number of thread to be accessed: OS_THREAD_1 or OS_THREAD_2
 */
void OS_DisableTiming(unsigned char thread) {
  if (thread == OS_THREAD_1) {
    t1_time_f = 0;
  } if (thread == OS_THREAD_2) {
    t2_time_f = 0;
  }
}

/** Get the time it takes to execute the function
 * @param thread Number of thread to be accessed: OS_THREAD_1 or OS_THREAD_2
 * @return nanoseconds for execution time.
 */
uint32_t OS_GetTime(unsigned char thread) {
  if (t1_time_f == 1 && thread == OS_THREAD_1) {
    ST7735_Message(ST7735_DISPLAY_TOP, 0, "Time #1: ", t1_time);
    return t1_time;
    // return (int)(t1_time * 12.5);
  } if (t2_time_f == 1 && thread == OS_THREAD_2) {
    ST7735_Message(ST7735_DISPLAY_TOP, 0, "Time #2: ", t2_time);
    return t2_time;
    // return (int)(t2_time * 12.5);
  }
  return 0;
}


void log_dump(void) {
  int i = 0;
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
    UART_OutUDec(LOG[i].time/timeslice);

    i++;
  }
}

void OS_LogEntry(int event) {
  log_entry new;
  int i;
  long sr = OS_StartCritical();
  for (i = LOG_MAX_ENTRY - 1; i > 0; i--) {
    LOG[i] = LOG[i-1];
  }
  new.thread = OS_Id();
  new.time = OS_Time();
  new.event = event;
  LOG[0] = new;
  OS_EndCritical(sr);
}

void log_clear(void) {
  int i;
  long sr = OS_StartCritical();
  for (i = 0; i < LOG_MAX_ENTRY; i++) {
    LOG[i].thread = -1;
  }
  OS_EndCritical(sr);
}
