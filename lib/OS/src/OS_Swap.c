#include "OS_Swap.h"

#include "OS.h"
#include "ff.h"
#include "printf.h"
#include "heap.h"


void OS_SwapInit() {
	if (f_chdir("/") != 0) return;
  int code = f_mkdir(".swap");
  
}

void OS_SwapOut(tcb_t *tcb) {
  FIL file;
  char buffer[25];
  int size;

  sprintf(buffer, "%x.swap", tcb);
  if (f_open(&file, buffer, FA_CREATE_ALWAYS) == 0) {
    f_write(&file, tcb->stack, tcb->stackSize, &size);
    if (tcb->stackSize == size) {
      Heap_Free(tcb->stack);
    }
    tcb->stack = 0;
  }
  f_close(&file);
}

void OS_SwapIn(tcb_t *tcb) {
  FIL file;
  char buffer[25];
  char out;
  unsigned int bytesRead = 0;

  sprintf(buffer, "%x.swap", tcb);
  if (f_open(&file, buffer, FA_READ) == 0) {
    // Reallocate stack
    tcb->stack = Heap_Malloc(sizeof(int) * tcb->stackSize);
    
    // Read stack
    if (f_read(&file, tcb->stack, tcb->stackSize, &bytesRead) != FR_OK || bytesRead != tcb->stackSize) {
      // Error
    }
  }
  f_close(&file);
  f_unlink(buffer);
}