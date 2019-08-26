// filename *************************heap.c ************************
// Implements memory heap for dynamic memory allocation.
// Follows standard malloc/calloc/realloc/free interface
// for allocating/unallocating memory.

// Jacob Egner 2008-07-31
// modified 8/31/08 Jonathan Valvano for style
// modified 12/16/11 Jonathan Valvano for 32-bit machine
// modified August 10, 2014 for C99 syntax

/* This example accompanies the book
   "Embedded Systems: Real Time Operating Systems for ARM Cortex M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2014

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// Implementation Notes:
// This is a Knuth Heap. Each block has a header and a trailer, which I shall
// call the meta-sections.  The meta-sections are each a single int32_t that tells
// how many int32_ts/words can be stored between the header and trailer.
// If the block is used, the meta-sections record the room as a positive
// number.  If the block is unused, the meta-sections record the room as a
// negative number.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "UART.h"
#include "Interpreter.h"
#include "heap.h"
#include "OS.h"

#define HEAP_START (Heap)
#define HEAP_END (HEAP_START + HEAP_SIZE_WORDS)

sema_t HEAP_FREE;

typedef struct MEMORY {
  struct MEMORY *next;
  char name[10];
  uint32_t max;
  int *memory;
} memory_t;

memory_t *list;

// Interpreter commands
extern char paramBuffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];
void mem(void);
void mem_o(void);
void mem_a(void);
void mem_f(void);
void mem_w(void);
void mem_s(void);



//The actual heap is just a big array.
static int32_t Heap[HEAP_SIZE_WORDS];

static int32_t inHeapRange(int32_t* address);
static int32_t blockUsed(int32_t* block);
static int32_t blockUnused(int32_t* block);
static int32_t blockRoom(int32_t* block);
//static int32_t blockSize(int32_t* block);
static int32_t* blockHeader(int32_t* blockEnd);
static int32_t* blockTrailer(int32_t* blockStart);
static int32_t* nextBlockHeader(int32_t* blockStart);
static int32_t* previousBlockHeader(int32_t* blockStart);
static int32_t markBlockUsed(int32_t* blockStart);
static int32_t markBlockUnused(int32_t* blockStart);
static int32_t splitAndMarkBlockUsed(int32_t* upperBlockStart, int32_t desiredRoom);
static void mergeBlockWithBelow(int32_t* upperBlockStart);
//static int32_t byteIndex(int32_t* ptr);

//******** Heap_Init ***************
// Initialize the Heap
// input: none
// output: always HEAP_OK
// notes: Initializes/resets the heap to a clean state where no memory
//  is allocated.
int32_t Heap_Init(void){
  cmd_t *cmd;
  int32_t* blockStart = HEAP_START;
  int32_t* blockEnd = (HEAP_START + HEAP_SIZE_WORDS - 1);
  *blockStart = -(int32_t)(HEAP_SIZE_WORDS - 2);
  *blockEnd = -(int32_t)(HEAP_SIZE_WORDS - 2);

  OS_InitSemaphore("heap_free", &HEAP_FREE, 1);

  list = 0;

  // Add interpreter commands
  cmd = IT_AddCommand("mem", 0, "", &mem, "print variables in heap", 128, 3);
  IT_AddFlag(cmd, 'a', 2, "[name] [size]" , &mem_a, "allocate memory", 128, 3);
  IT_AddFlag(cmd, 'f', 1, "[name]" , &mem_f, "free memory", 128, 3);
  IT_AddFlag(cmd, 'w', 3, "[name] [offset] [value]" , &mem_w, "write value to memory", 128, 3);
  IT_AddFlag(cmd, 'o', 1, "[name]" , &mem_o, "print contents of variable", 128, 3);
  IT_AddFlag(cmd, 's', 0, "" , &mem_s, "print heap stats", 128, 3);



  return HEAP_OK;
}


//******** Heap_Malloc ***************
// Allocate memory, data not initialized
// input:
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory or will return NULL
//   if there isn't sufficient space to satisfy allocation request
void* Heap_Malloc(int32_t desiredBytes){
  int32_t desiredWords = (desiredBytes + sizeof(int32_t) - 1) / sizeof(int32_t);
  int32_t* blockStart = HEAP_START;  // implements first fit
  OS_bWait(&HEAP_FREE);
  if(desiredWords <= 0){
    OS_bSignal(&HEAP_FREE);
    return 0; //NULL
  }
  while(inHeapRange(blockStart)){
  // one pass through the heap
  // choose first block that is big enough
    if(blockUnused(blockStart) && desiredWords <= blockRoom(blockStart)){
      if(splitAndMarkBlockUsed(blockStart, desiredWords)){
        OS_bSignal(&HEAP_FREE);
        return 0; //NULL
      }
      OS_bSignal(&HEAP_FREE);
      return blockStart + 1;
    }
    blockStart = nextBlockHeader(blockStart);
  }
  OS_bSignal(&HEAP_FREE);
  return 0; //NULL
}


//******** Heap_Calloc ***************
// Allocate memory, data are initialized to 0
// input:
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory block or will return NULL
//   if there isn't sufficient space to satisfy allocation request
//notes: the allocated memory block will be zeroed out
void* Heap_Calloc(int32_t desiredBytes){
  int32_t* blockPtr;
  int32_t wordsToClear;
  int32_t i;

  //malloc a block
  blockPtr = Heap_Malloc(desiredBytes);
  //did malloc fail?
  if(blockPtr == 0){
    return 0; //NULL
  }
  wordsToClear = *(blockPtr - 1); //get room from header
  //clear out block
  for(i = 0; i < wordsToClear; i++){
    blockPtr[i] = 0;
  }
  return blockPtr;
}


//******** Heap_Realloc ***************
// Reallocate buffer to a new size
//input:
//  oldBlock: pointer to a block
//  desiredBytes: a desired number of bytes for a new block
//    where the contents of the old block will be copied to
// output: void* pointing to the new block or will return NULL
//   if there is any reason the reallocation can't be completed
// notes: the given block will be unallocated after its contents
//   are copied to the new block
void* Heap_Realloc(void* oldBlock, int32_t desiredBytes){
  int32_t* oldBlockPtr;
  int32_t* oldBlockStart;
  int32_t* newBlockPtr;
  int32_t oldBlockRoom;
  int32_t newBlockRoom;
  int32_t wordsToCopy;
  int32_t i;

  oldBlockPtr = (int32_t*) oldBlock;
  // error if...
  // 1) oldBlockPtr doesn't point in the heap
  // 2) oldBlockPtr points to an unused block
  oldBlockStart = oldBlockPtr - 1;
  if(!inHeapRange(oldBlockStart) || blockUnused(oldBlockStart)){
    return 0; // NULL
  }

  newBlockPtr = Heap_Malloc(desiredBytes);
  // did Malloc fail?
  if(newBlockPtr == 0){
    return 0; // NULL
  }

  oldBlockRoom = blockRoom(oldBlockStart);
  newBlockRoom = blockRoom(newBlockPtr - 1);
  if(oldBlockRoom < newBlockRoom){
    wordsToCopy = oldBlockRoom;
  }
  else{
    wordsToCopy = newBlockRoom;
  }
  for(i = 0; i < wordsToCopy; i++){
    newBlockPtr[i] = oldBlockPtr[i];
  }
  if(Heap_Free(oldBlockPtr)){
    return 0; // NULL Free failed
  }
  return newBlockPtr;
}


//******** Heap_Free ***************
// return a block to the heap
// input: pointer to memory to unallocate
// output: HEAP_OK if everything is ok;
//  HEAP_ERROR_POINTER_OUT_OF_RANGE if pointer points outside the heap;
//  HEAP_ERROR_CORRUPTED_HEAP if heap has been corrupted or trying to
//  unallocate memory that has already been unallocated;
int32_t Heap_Free(void* pointer){
  int32_t* blockStart;
  int32_t* blockEnd;
  int32_t* nextBlockStart;

  OS_bWait(&HEAP_FREE);

  blockStart = ((int32_t*)pointer) - 1;

  //-----Begin error checking-------
  if(!inHeapRange(blockStart)){
    OS_bSignal(&HEAP_FREE);
    return HEAP_ERROR_POINTER_OUT_OF_RANGE;
  }
  if(blockUnused(blockStart)){
    OS_bSignal(&HEAP_FREE);
    return HEAP_ERROR_CORRUPTED_HEAP;
  }
  blockEnd = blockTrailer(blockStart);
  if(!inHeapRange(blockEnd) || blockUnused(blockEnd)){
    OS_bSignal(&HEAP_FREE);
    return HEAP_ERROR_CORRUPTED_HEAP;
  }
  //-----End error checking-------

  if(markBlockUnused(blockStart)){
    OS_bSignal(&HEAP_FREE);
    return HEAP_ERROR_CORRUPTED_HEAP;
  }

  // time to possibly merge with block above
  // first, make sure there IS a block above us
  if(blockStart > HEAP_START){
    int32_t* previousBlockStart = previousBlockHeader(blockStart);
    // second, make sure we only merge with an unused block
    if(blockUnused(previousBlockStart)){
      mergeBlockWithBelow(previousBlockStart);
      blockStart = previousBlockStart; // start of block has moved
    }
  }

  // possibly merge with block below
  nextBlockStart = nextBlockHeader(blockStart);
  if(inHeapRange(nextBlockStart) && blockUnused(nextBlockStart)){
    mergeBlockWithBelow(blockStart);
  }
  OS_bSignal(&HEAP_FREE);
  return HEAP_OK;
}


//******** Heap_Test ***************
// Test the heap
// input: none
// output: validity of the heap - either HEAP_OK or HEAP_ERROR_HEAP_CORRUPTED
int32_t Heap_Test(void){
  int32_t lastBlockWasUnused = 0;
  int32_t* blockStart = HEAP_START;
  while(inHeapRange(blockStart)){
    int32_t* blockEnd;

    //shouldn't have any blocks holding zero words
    if(*blockStart == 0){
      return HEAP_ERROR_CORRUPTED_HEAP;
    }
    blockEnd = blockTrailer(blockStart);
    //error if blockEnd is not in the heap or blockend disagrees with blockStart
    if(!inHeapRange(blockEnd) || *blockStart != *blockEnd){
      return HEAP_ERROR_CORRUPTED_HEAP;
    }
    //error if we have two adjacent unused blocks
    if(lastBlockWasUnused && blockUnused(blockStart)){
      return HEAP_ERROR_CORRUPTED_HEAP;
    }
    lastBlockWasUnused = blockUnused(blockStart);
    blockStart = blockEnd + 1;
  }
  //traversing the heap should end exactly where the heap ends
  if(blockStart != HEAP_END){
    return HEAP_ERROR_CORRUPTED_HEAP;
  }
  return HEAP_OK;
}


//******** Heap_Stats ***************
// return the current status of the heap
// input: none
// output: a heap_stats_t that describes the current usage of the heap
heap_stats_t Heap_Stats(void){
  int32_t* blockStart;
  heap_stats_t stats;

  stats.wordsAllocated = 0;
  stats.wordsAvailable = 0;
  stats.blocksUsed = 0;
  stats.blocksUnused = 0;

  //just go through each block to get stats on heap usage
  blockStart = HEAP_START;
  while(inHeapRange(blockStart)){
    if(blockUsed(blockStart)){
      stats.wordsAllocated += blockRoom(blockStart);
      stats.blocksUsed++;
    }
    else{
      stats.wordsAvailable += blockRoom(blockStart);
      stats.blocksUnused++;
    }
    blockStart = nextBlockHeader(blockStart);
  }
  stats.wordsOverhead = HEAP_SIZE_WORDS - stats.wordsAllocated - stats.wordsAvailable;
  return stats;
}


// inHeapRange
// input: a pointer
// output: whether or not the pointer points inside the heap
static int32_t inHeapRange(int32_t* address){
  return address >= HEAP_START && address < HEAP_END;
}


// blockUsed
// input: pointer to the header or trailer of a block
// output: whether or not the block is marked as used/allocated
static int32_t blockUsed(int32_t* block){
  return *block > 0;
}


// blockUnused
// input: pointer to the header or trailer of a block
// output: whether or not the block is marked as unused/unallocated
static int32_t blockUnused(int32_t* block){
  return *block < 0;
}


// blockRoom
// input: pointer to the header or trailer of a block
// output: how many words of data the block can hold
static int32_t blockRoom(int32_t* block){
  if(*block > 0){
    return *block;
  }
  return -*block;
}


// // blockSize
// // input: pointer to the header or trailer of a block
// // output: the size of a block in words, including header and trailer
// static int32_t blockSize(int32_t* block){
//   if(*block > 0){
//     return *block + 2;
//   }
//   return -*block + 2;
// }


// blockHeader
// input: pointer to the trailer of a block
// output: pointer to the header of the same block
static int32_t* blockHeader(int32_t* blockEnd){
  return blockEnd - blockRoom(blockEnd) - 1;
}


// blockTrailer
// input: pointer to the header of a block
// output: pointer to the trailer of the same block
static int32_t* blockTrailer(int32_t* blockStart){
  return blockStart + blockRoom(blockStart) + 1;
}


// nextBlockHeader
// input: pointer to the header of a block
// output: pointer the the header of the next block in the heap
// notes: given the header of the last block in the heap, will point to HEAP_END,
//   which is not a valid block; be careful
static int32_t* nextBlockHeader(int32_t* blockStart){
  return blockTrailer(blockStart) + 1;
}


// previousBlockHeader
// input: pointer to the header of a block
// output: pointer the the header of the previous block in the heap
// notes: given the header of the first block in the heap, this function
//   will go crazy and return a proportionally crazy address!
static int32_t* previousBlockHeader(int32_t* blockStart){
  return blockHeader(blockStart - 1);
}


// markBlockUsed
// input: pointer to the header of a block
// output: a heap flag - HEAP_OK if everything is ok or HEAP_ERROR_CORRUPTEDHEAP
//   if there is something obviously wrong with the block
//notes: marks the block as used/allocated
static int32_t markBlockUsed(int32_t* blockStart){
  int32_t* blockEnd = blockTrailer(blockStart);
  if(blockUsed(blockStart) || *blockStart != *blockEnd){
    return HEAP_ERROR_CORRUPTED_HEAP;
  }
  *blockStart = -*blockStart;
  *blockEnd = -*blockEnd;
  return HEAP_OK;
}


// markBlockUnused
// input: pointer to the header of a block
// output: a heap flag - HEAP_OK if everything is ok or HEAP_ERROR_CORRUPTEDHEAP
//  if there is something obviously wrong with the block
// notes: marks the block as unused/unallocated
static int32_t markBlockUnused(int32_t* blockStart){
  int32_t* blockEnd = blockTrailer(blockStart);
  if(blockUnused(blockStart) || *blockStart != *blockEnd){
    return HEAP_ERROR_CORRUPTED_HEAP;
  }
  *blockStart = -*blockStart;
  *blockEnd = -*blockEnd;
  return HEAP_OK;
}


// splitAndMarkBlockUsed
// input:
//  uppterBlockStart: header of a block
//  desiredRoom: desired amount of words to be in the new upper block
// output: none
// notes: splits the block given so that the new upper block holds desiredRoom
//  words (or more).  Marks the upper block as used, lower block as unused.
//  Will not split a block if the leftover room is insufficient to make another
//  useful block.
static int32_t splitAndMarkBlockUsed(int32_t* upperBlockStart, int32_t desiredRoom){
  int32_t leftoverRoom = blockRoom(upperBlockStart) - desiredRoom - 2;
  // only split block if leftovers could actually make another useful block
  if(leftoverRoom > 0){
    int32_t* upperBlockEnd = upperBlockStart + desiredRoom + 1;
    int32_t* lowerBlockStart = upperBlockEnd + 1;
    int32_t* lowerBlockEnd = blockTrailer(upperBlockStart);
    *upperBlockStart = desiredRoom; // marked used
    *upperBlockEnd = desiredRoom;
    *lowerBlockStart = -leftoverRoom; // marked unused
    *lowerBlockEnd = -leftoverRoom;
  }
  // can't split block - just mark it at used
  else{
    if(markBlockUsed(upperBlockStart)){
      return 0; // NULL Free failed
    }
  }
  return HEAP_OK;
}


// mergeBlockWithBelow
// input: pointer to the header of a block
// output: none
// notes: will merge the given block with the block below it.
//  WARNING: Does not check that the block below actually exists.
static void mergeBlockWithBelow(int32_t* upperBlockStart){
  int32_t* upperBlockEnd = blockTrailer(upperBlockStart);
  int32_t* lowerBlockStart = upperBlockEnd + 1;
  int32_t* lowerBlockEnd = blockTrailer(lowerBlockStart);

  int32_t room = lowerBlockEnd - upperBlockStart - 1;
  *upperBlockStart = -room;
  *lowerBlockEnd = -room;
  return;
}




// Interpreter

// List all variables stored in memory
void mem(void) {
  memory_t *el = list;

  IT_Init();
  
  while (el != 0) {
    UART_OutString("\r\n  ");
    UART_OutString(el->name);
    el = el->next;
  }
  IT_Kill();
}

// Print variables stored in memory
void mem_o(void) {
  int i;
  memory_t *el = list;

  IT_Init();

  IT_GetBuffer(paramBuffer);

  while (el != 0) {
    if (strcmp(el->name, paramBuffer[0]) == 0) {
      UART_OutString("\r\n  ");
      UART_OutString(el->name);
      for (i = 0; i < el->max; i++) {
        UART_OutString("\r\n    ");
        UART_OutUDec(el->memory[i]);
      }
      IT_Kill();
    }
    el = el->next;
  }
  UART_OutError("\r\n  ERROR: couldn't find the memory location\r\n");
  IT_Kill();
}

// Allocate memory
void mem_a(void) {
  memory_t *new;
  uint32_t i;

  IT_Init();

  IT_GetBuffer(paramBuffer);

  // Allocate memory for the struct
  new = Heap_Malloc(sizeof(memory_t));
  // Save the name of the new memory location
  strcpy(new->name, paramBuffer[0]);
  // Save the max value in the allocated memory
  if (digits_only(paramBuffer[1]) == 0) {
    UART_OutError("\r\n  ERROR: the number of memory addresses to allocate can only contain digits 0 - 9\r\n");
    IT_Kill();
  }
  new->max = atoi(paramBuffer[1]);

  // Allocate memory
  new->memory = Heap_Malloc(sizeof(int) * new->max);
  if (new->memory == 0) {
    UART_OutError("\r\n  ERROR: couldn't allocate memory\r\n");
    Heap_Free(new);
    IT_Kill();
  }

  // Initialize memory
  for (i = 0; i < new->max; i++) {
    new->memory[i] = 0;
  }

  // Add to the memory list
  new->next = list;
  list = new;
  IT_Kill();
}

// Free memory
void mem_f(void) {
  memory_t *el, *prev;

  IT_Init();

  IT_GetBuffer(paramBuffer);

  prev = 0;
  el = list;
  while (el != 0) {
    if (strcmp(el->name, paramBuffer[0]) == 0) {
      if (el == list) { // The first
        list = list->next;
      } else if (el->next == 0) { // The last
        prev->next = 0;
      } else { // Any
        prev->next = el->next;
      }

      // Free memory
      if (Heap_Free(el->memory) != HEAP_OK) {
        UART_OutError("\r\n  ERROR: couldn't free memory\r\n");
      }

      // Free memory struct
      if (Heap_Free(el) != HEAP_OK) {
        UART_OutError("\r\n  ERROR: couldn't free memory\r\n");
      }
      IT_Kill();
    }
    prev = el;
    el = el->next;
  }
  UART_OutError("\r\n  ERROR: couldn't find the memory location\r\n");
  IT_Kill();
}

// Write to memory
void mem_w(void) {
  memory_t *el;

  IT_Init();

  IT_GetBuffer(paramBuffer);

  if (digits_only(paramBuffer[1]) == 0) {
    UART_OutError("\r\n  ERROR: the address offset can only contain digits 0 - 9\r\n");
    IT_Kill();
  }

  if (digits_only(paramBuffer[2]) == 0) {
    UART_OutError("\r\n  ERROR: the value to write can only contain digits 0 - 9\r\n");
    IT_Kill();
  }

  el = list;
  while (el != 0) {
    if (strcmp(el->name, paramBuffer[0]) == 0) {
      if (atoi(paramBuffer[1]) > el->max) {
        UART_OutError("\r\n  ERROR: the address offset is out of range\r\n");
        IT_Kill();
      }
      el->memory[atoi(paramBuffer[1])] = atoi(paramBuffer[2]);
      IT_Kill();
    }
    el = el->next;
  }
  UART_OutError("\r\n  ERROR: couldn't find the memory location\r\n");
  IT_Kill();
}

// Print stats
void mem_s(void) {
  heap_stats_t stats;

  IT_Init();

  stats = Heap_Stats();
  UART_OutString("\n\r  Words Allocated: ");
  UART_OutUDec(stats.wordsAllocated);
  UART_OutString("\n\r  Words Available: ");
  UART_OutUDec(stats.wordsAvailable);
  UART_OutString("\n\r  Words Overhead: ");
  UART_OutUDec(stats.wordsOverhead);
  UART_OutString("\n\r  Blocks Used: ");
  UART_OutUDec(stats.blocksUsed);
  UART_OutString("\n\r  Blocks Unused: ");
  UART_OutUDec(stats.blocksUnused);
  IT_Kill();
}
