// ----------------------------------- OS.h -----------------------------------
// Compatibility: Runs on TM4C123.
// Description: Provide functions to execute tasks periodically
//              using timer interrupts.
// Authors: Juan Pablo Pino, Cole Morgan.
// February 4, 2019
// ----------------------------------------------------------------------------

#ifndef __OS_H__
#define __OS_H__


#include <stdint.h>

#define OS_THREAD_ADD_ERROR 0
#define OS_THREAD_1 1
#define OS_THREAD_2 2

#define JITTERSIZE 32
#define NUM_PRIORITIES 8

#define TIME_1MS  80000
#define TIME_2MS  2*TIME_1MS

#define PE0  (*((volatile unsigned long *)0x40024004))
#define PE1  (*((volatile unsigned long *)0x40024008))
#define PE2  (*((volatile unsigned long *)0x40024010))
#define PE3  (*((volatile unsigned long *)0x40024020))

#define PF0 (*((volatile uint32_t *)0x40025004))
#define PF1 (*((volatile uint32_t *)0x40025008))
#define PF2 (*((volatile uint32_t *)0x40025010))
#define PF3 (*((volatile uint32_t *)0x40025020))
#define PF4 (*((volatile uint32_t *)0x40025040))

#define TPE0() PE0 ^= 0x01
#define TPE1() PE1 ^= 0x02
#define TPE2() PE2 ^= 0x04
#define TPE3() PE3 ^= 0x08

#define TCB_NAME_LEN    25
#define SEMA_T_NAME_LEN 15



/* Type definitions */

/* Process Control Block */
typedef struct pcb_t {
  struct pcb_t *next;
  struct pcb_t *prev;
  int pid;
  uint32_t *code;
  uint32_t *data;
  int priority;
  int numThreads;
} PCB;

/* Thread Control Block */
typedef struct tcb_t {
  int *sp;
  struct tcb_t *next;
  struct tcb_t *prev;
  int id;
  int stackSize;
  int inUse;
  int sleepTime;
  int priority;
  int deleteThis;
  int isBlocked;
  int *stack;
  PCB *parent;
	int fpu;
  char name[TCB_NAME_LEN + 1];
	int output;
} tcb_t;

/* Semaphore */
typedef struct sema_t {
  char name[SEMA_T_NAME_LEN + 1];
  struct sema_t *next;
  int value;
  int max;
  tcb_t *blocked;
	tcb_t *who;
} sema_t;

/* Log Entry */
typedef struct {
 unsigned char event;
 long time;
 int thread;
} log_entry;



/* OS Functions */

/** Initialize operating system, disable interrupts until OS_Launch
 * Initialize OS controlled I/O: pendsv, 80 MHz PLL
 */
void OS_Init(void);

/** Start the scheduler, enable interrupts. Does not return.
 * @param theTimeSlice number of 20ns clock cycles for each time slice
 */
void OS_Launch(uint32_t theTimeSlice);

long OS_StartCritical(void);
void OS_EndCritical(long);
void OS_EnableInterrupts(void);
void OS_DisableInterrupts(void);



/* Foreground Thread Functions */

/** Add one foreground thread to the scheduler
 * @param task pointer to task
 * @param stackSize memory to be allocated in the stack
 * @param priority priority of the thread to be added
 * @return 1 if the thread was successfully added, 0 if not
 */
tcb_t* OS_AddThread(char *name, void(*task)(void), int stackSize, int priority);
tcb_t* OS_CurrentThread(void);
void OS_Kill(void);
void OS_Sleep(int);
void OS_Suspend(void);
int OS_Id(void);
void OS_FsInit(void);
unsigned long OS_ThreadCount(void);

int OS_AddProcess(void(*entry)(void), void *text, void *data, int stackSize, int priority);



/* Periodic Background Thread Functions */

int OS_AddPeriodicThread(void(*task)(void), uint32_t period, uint32_t priority);
void OS_RemovePeriodicThread(unsigned char thread);
int OS_RunningThreads(void);



/* Aperiodic Background Thread Functions */

void OS_AddSW1Task(void(*task)(void), int pri);
void OS_AddSW2Task(void(*task)(void), int pri);



/* Time Functions */

unsigned long OS_Time(void);
unsigned long OS_TimeDifference(unsigned long, unsigned long);
unsigned long OS_MsTime(void);
void OS_ClearMsTime(void);
uint32_t OS_ReadPeriodicTime(unsigned char thread);
void OS_ClearPeriodicTime(void);

/** Time how long the periodic thread is active
 * @param thread Number of thread to be accessed: OS_THREAD_1 or OS_THREAD_2
 */
void OS_EnableTiming(unsigned char thread);

/** Disable timing options
 * @param thread Number of thread to be accessed: OS_THREAD_1 or OS_THREAD_2
 */
void OS_DisableTiming(unsigned char thread);

/** Get the time it takes to execute the function
 * @param thread Number of thread to be accessed: OS_THREAD_1 or OS_THREAD_2
 * @return nanoseconds for execution time.
 */
uint32_t OS_GetTime(unsigned char thread);
long OS_GetJitter(unsigned char);

/* Interrupt Disabled Time Measurement Functions */
long OS_GetPercIntDis(void);
long OS_GetMaxTimeIntDis(void);



/* Mailbox Functions */

void OS_MailBox_Init(void);
void OS_MailBox_Send(int);
int OS_MailBox_Recv(void);



/* Semaphore Functions */

void OS_InitSemaphore(char *name, sema_t *s, int val);
void OS_bWait(sema_t *s);
void OS_Wait(sema_t *s);
void OS_bSignal(sema_t *s);
void OS_Signal(sema_t *s);



/* FIFO Functions */

int OS_Fifo_Init(int size);
int OS_Fifo_Put(int val);
int OS_Fifo_Get(void);



/* Log Functions */

#define LOG_MAX_ENTRY 100

#define LOG_THREAD_START    1
#define LOG_THREAD_SWITCH   2
#define LOG_THREAD_KILL     3
#define LOG_PERIODIC_START  4
#define LOG_PERIODIC_FINISH 5

void OS_LogEntry(int);



#endif //__OS_H__
