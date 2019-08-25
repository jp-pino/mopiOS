// UART2.c
// Runs on LM4F120/TM4C123
// Use UART0 to implement bidirectional data transfer to and from a
// computer running HyperTerminal.  This time, interrupts and FIFOs
// are used.
// Daniel Valvano
// September 11, 2013
// Modified by EE345L students Charlie Gough && Matt Hawk
// Modified by EE345M students Agustinus Darmawan && Mingjie Qiu

/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2017
   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2017

 Copyright 2017 by Jonathan W. Valvano, valvano@mail.utexas.edu
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


// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1
#include <stdint.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "UART.h"
#include "OS.h"

#define NVIC_EN0_INT5           0x00000020  // Interrupt 5 enable
#define UART_FR_RXFF            0x00000040  // UART Receive FIFO Full
#define UART_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART_CTL_UARTEN         0x00000001  // UART Enable
#define UART_IFLS_RX1_8         0x00000000  // RX FIFO >= 1/8 full
#define UART_IFLS_TX1_8         0x00000000  // TX FIFO <= 1/8 full
#define UART_IM_RTIM            0x00000040  // UART Receive Time-Out Interrupt
                                            // Mask
#define UART_IM_TXIM            0x00000020  // UART Transmit Interrupt Mask
#define UART_IM_RXIM            0x00000010  // UART Receive Interrupt Mask
#define UART_RIS_RTRIS          0x00000040  // UART Receive Time-Out Raw
                                            // Interrupt Status
#define UART_RIS_TXRIS          0x00000020  // UART Transmit Raw Interrupt
                                            // Status
#define UART_RIS_RXRIS          0x00000010  // UART Receive Raw Interrupt
                                            // Status
#define UART_ICR_RTIC           0x00000040  // Receive Time-Out Interrupt Clear
#define UART_ICR_TXIC           0x00000020  // Transmit Interrupt Clear
#define UART_ICR_RXIC           0x00000010  // Receive Interrupt Clear
#define SYSCTL_RCGC1_UART0      0x00000001  // UART0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control

char cmd_memory[MAX_CMD_MEM][MAX_CMD_LEN]; // memory to recall commands
sema_t UART_FREE;

// Two-pointer implementation of the FIFO
// can hold 0 to FIFOSIZE-1 elements
#define TXFIFOSIZE 32     // can be any size
#define FIFOSUCCESS 1
#define FIFOFAIL    0

int func = 0;

typedef char dataType;
dataType volatile *TxPutPt;  // put next
dataType volatile *TxGetPt;  // get next
dataType TxFifo[TXFIFOSIZE];
sema_t TxRoomLeft;   // Semaphore counting empty spaces in TxFifo
void TxFifo_Init(void){ // this is critical
  // should make atomic
  func = 1;
  TxPutPt = TxGetPt = &TxFifo[0]; // Empty
  OS_InitSemaphore(&TxRoomLeft, TXFIFOSIZE-1);// Initially lots of spaces
  // end of critical section
}


void TxFifo_Put(dataType data){
  func = 2;
  OS_Wait(&TxRoomLeft);      // If the buffer is full, spin/block
  func = 201;
  *(TxPutPt) = data;   // Put
  TxPutPt = TxPutPt+1;
  if(TxPutPt ==&TxFifo[TXFIFOSIZE]){
    TxPutPt = &TxFifo[0];      // wrap
  }
}
int TxFifo_Get(dataType *datapt){
  func = 3;
  if(TxPutPt == TxGetPt ){
    return(FIFOFAIL);// Empty if PutPt=GetPt
  }
  else{
    *datapt = *(TxGetPt++);
    if(TxGetPt==&TxFifo[TXFIFOSIZE]){
       TxGetPt = &TxFifo[0]; // wrap
    }
    OS_Signal(&TxRoomLeft); // increment if data removed
    return(FIFOSUCCESS);
  }
}
unsigned int TxFifo_Size(void){
  func = 4;
  return(((unsigned int)TxPutPt - (unsigned int)TxGetPt)& TXFIFOSIZE-1) ;
}

#define RXFIFOSIZE 16     // can be any size

dataType volatile *RxPutPt;  // put next
dataType volatile *RxGetPt;  // get next
dataType static RxFifo[RXFIFOSIZE];
sema_t RxFifoAvailable;   // Semaphore counting data in RxFifo

void RxFifo_Init(void){ // this is critical
  func = 5;

  // should make atomic
  RxPutPt = RxGetPt = &RxFifo[0]; // Empty
  OS_InitSemaphore(&RxFifoAvailable, 0); // Initially empty
  // end of critical section
}
int RxFifo_Put(dataType data){
  dataType volatile *nextPutPt;
  func = 6;
  nextPutPt = RxPutPt+1;
  if(nextPutPt ==&RxFifo[RXFIFOSIZE]){
    nextPutPt = &RxFifo[0];      // wrap
  }
  if(nextPutPt == RxGetPt){
    return(FIFOFAIL);  // Failed, fifo full
  }
  else{
    *(RxPutPt) = data;   // Put
    RxPutPt = nextPutPt; // Success, update
    OS_Signal(&RxFifoAvailable); // increment only if data actually stored
    return(FIFOSUCCESS);
  }
}
dataType RxFifo_Get(void){ dataType data;
  func = 7;
  OS_Wait(&RxFifoAvailable);
  data = *(RxGetPt++);
  if(RxGetPt==&RxFifo[RXFIFOSIZE]){
     RxGetPt = &RxFifo[0]; // wrap
  }
  return data;
}
unsigned int RxFifo_Size(void){
  func = 8;
  return(((unsigned int)RxPutPt - (unsigned int)RxGetPt)& RXFIFOSIZE-1) ;

//  return RxFifoAvailable.Value;
}

// Initialize UART0
// Baud rate is 115200 bits/sec
void UART_Init(void){
  func = 9;

  OS_InitSemaphore(&UART_FREE, 1);

  SYSCTL_RCGCUART_R |= 0x01; // activate UART0
  SYSCTL_RCGCGPIO_R |= 0x01; // activate port A
  RxFifo_Init();                        // initialize empty FIFOs
  TxFifo_Init();
  UART0_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  UART0_IBRD_R = 43;                    // IBRD = int(80,000,000 / (16 * 115200)) = int(43.402778)
  UART0_FBRD_R = 26;                    // FBRD = round(0.402778 * 64) = 26
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
  UART0_IFLS_R &= ~0x3F;                // clear TX and RX interrupt FIFO level fields
                                        // configure interrupt for TX FIFO <= 1/8 full
                                        // configure interrupt for RX FIFO >= 1/8 full
  UART0_IFLS_R += (UART_IFLS_TX1_8|UART_IFLS_RX1_8);
                                        // enable TX and RX FIFO interrupts and RX time-out interrupt
  UART0_IM_R |= (UART_IM_RXIM|UART_IM_TXIM|UART_IM_RTIM);
  UART0_CTL_R |= UART_CTL_UARTEN;       // enable UART
  GPIO_PORTA_AFSEL_R |= 0x03;           // enable alt funct on PA1-0
  GPIO_PORTA_DEN_R |= 0x03;             // enable digital I/O on PA1-0
                                        // configure PA1-0 as UART
  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x00000011;
  GPIO_PORTA_AMSEL_R = 0;               // disable analog functionality on PA
                                        // UART0=priority 2
  NVIC_PRI1_R = (NVIC_PRI1_R&0xFFFF00FF)|0x00004000; // bits 13-15
  NVIC_EN0_R = NVIC_EN0_INT5;           // enable interrupt 5 in NVIC

  OS_EnableInterrupts();
}
// copy from hardware RX FIFO to software RX FIFO
// stop when hardware RX FIFO is empty or software RX FIFO is full
void static copyHardwareToSoftware(void){
  char letter;
    func = 10;
  while(((UART0_FR_R&UART_FR_RXFE) == 0) && (RxFifo_Size() < (RXFIFOSIZE - 1))){
    letter = UART0_DR_R;
    RxFifo_Put(letter);
  }
}
// copy from software TX FIFO to hardware TX FIFO
// stop when software TX FIFO is empty or hardware TX FIFO is full
void static copySoftwareToHardware(void){
  char letter;
    func = 11;
  while(((UART0_FR_R&UART_FR_TXFF) == 0) && (TxFifo_Size() > 0)){
    TxFifo_Get(&letter);
    UART0_DR_R = letter;
  }
}
// input ASCII character from UART
// spin if RxFifo is empty
char UART_InChar(void){
  char letter;
    func = 12;
  letter = RxFifo_Get(); // block or spin if empty
  return(letter);
}
// output ASCII character to UART
// spin if TxFifo is full
void UART_OutChar(char data){
  func = 13;
  TxFifo_Put(data);
  UART0_IM_R &= ~UART_IM_TXIM;          // disable TX FIFO interrupt
  copySoftwareToHardware();
  UART0_IM_R |= UART_IM_TXIM;           // enable TX FIFO interrupt
}

void UART_OutChar2(char data){
  func = 14;
  TxFifo_Put(data);
  UART0_IM_R &= ~UART_IM_TXIM;          // disable TX FIFO interrupt
  copySoftwareToHardware();
  UART0_IM_R |= UART_IM_TXIM;           // enable TX FIFO interrupt
}
// at least one of three things has happened:
// hardware TX FIFO goes from 3 to 2 or less items
// hardware RX FIFO goes from 1 to 2 or more items
// UART receiver has timed out
void UART0_Handler(void){
  func = 15;
  if(UART0_RIS_R&UART_RIS_TXRIS){       // hardware TX FIFO <= 2 items
    UART0_ICR_R = UART_ICR_TXIC;        // acknowledge TX FIFO
    // copy from software TX FIFO to hardware TX FIFO
    copySoftwareToHardware();
    if(TxFifo_Size() == 0){             // software TX FIFO is empty
      UART0_IM_R &= ~UART_IM_TXIM;      // disable TX FIFO interrupt
    }
  }
  if(UART0_RIS_R&UART_RIS_RXRIS){       // hardware RX FIFO >= 2 items
    UART0_ICR_R = UART_ICR_RXIC;        // acknowledge RX FIFO
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
  if(UART0_RIS_R&UART_RIS_RTRIS){       // receiver timed out
    UART0_ICR_R = UART_ICR_RTIC;        // acknowledge receiver time out
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
}

//------------UART_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART_OutString(char *pt){
  OS_bWait(&UART_FREE);
  func = 16;
  while(*pt){
    UART_OutChar(*pt);
    pt++;
  }
  OS_bSignal(&UART_FREE);
}

//------------UART_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART_InUDec(void){
  unsigned long number=0, length=0;
  char character;
  func = 17;
  character = UART_InChar();
  while(character != CR){
  // accepts until <enter> is typed
  // The next line checks that the input is a digit, 0-9.
  // If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART_OutChar(character);
    }
  // If the input is a backspace, then the return number is
  // changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  return number;
}

//-----------------------UART_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART_OutUDecRec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  func = 18;
  if(n >= 10){
    UART_OutUDecRec(n/10);
    n = n%10;
  }
  UART_OutChar(n+'0'); /* n is between 0 and 9 */
}

void UART_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  func = 19;
  if(n >= 10){
    UART_OutUDecRec(n/10);
    n = n%10;
  }
  UART_OutChar(n+'0'); /* n is between 0 and 9 */
}

//-----------------------UART_OutUDec3-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Fixed format 3 digits with space after
void UART_OutUDec3(unsigned long n){
  func = 20;
  if(n>999){
    UART_OutString("***");
  }else if(n >= 100){
    UART_OutChar(n/100+'0');
    n = n%100;
    UART_OutChar(n/10+'0');
    n = n%10;
    UART_OutChar(n+'0');
  }else if(n >= 10){
    UART_OutChar(' ');
    UART_OutChar(n/10+'0');
    n = n%10;
    UART_OutChar(n+'0');
  }else{
    UART_OutChar(' ');
    UART_OutChar(' ');
    UART_OutChar(n+'0');
  }
  UART_OutChar(' ');
}
//-----------------------UART_OutUDec5-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Fixed format 5 digits with space after
void UART_OutUDec5(unsigned long n){
  func = 21;
  if(n>99999){
    UART_OutString("*****");
  }else if(n >= 10000){
    UART_OutChar(n/10000+'0');
    n = n%10000;
    UART_OutChar(n/1000+'0');
    n = n%1000;
    UART_OutChar(n/100+'0');
    n = n%100;
    UART_OutChar(n/10+'0');
    n = n%10;
    UART_OutChar(n+'0');
  }else if(n >= 1000){
    UART_OutChar(' ');
    UART_OutChar(n/1000+'0');
    n = n%1000;
    UART_OutChar(n/100+'0');
    n = n%100;
    UART_OutChar(n/10+'0');
    n = n%10;
    UART_OutChar(n+'0');
  }else if(n >= 100){
    UART_OutChar(' ');
    UART_OutChar(' ');
    UART_OutChar(n/100+'0');
    n = n%100;
    UART_OutChar(n/10+'0');
    n = n%10;
    UART_OutChar(n+'0');
  }else if(n >= 10){
    UART_OutChar(' ');
    UART_OutChar(' ');
    UART_OutChar(' ');
    UART_OutChar(n/10+'0');
    n = n%10;
    UART_OutChar(n+'0');
  }else{
    UART_OutChar(' ');
    UART_OutChar(' ');
    UART_OutChar(' ');
    UART_OutChar(' ');
    UART_OutChar(n+'0');
  }
  UART_OutChar(' ');
}
//-----------------------UART_OutSDec-----------------------
// Output a 32-bit number in signed decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART_OutSDec(long n){
  func = 22;
  if(n<0){
    UART_OutChar('-');
    n = -n;
  }
  UART_OutUDec((unsigned long)n);
}
//---------------------UART_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
// No '$' or '0x' need be entered, just the 1 to 8 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     value range is 0 to FFFFFFFF
// If you enter a number above FFFFFFFF, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART_InUHex(void){
unsigned long number=0, digit, length=0;
char character;
  func = 23;
  character = UART_InChar();
  while(character != CR){
    digit = 0x10; // assume bad
    if((character>='0') && (character<='9')){
      digit = character-'0';
    }
    else if((character>='A') && (character<='F')){
      digit = (character-'A')+0xA;
    }
    else if((character>='a') && (character<='f')){
      digit = (character-'a')+0xA;
    }
// If the character is not 0-9 or A-F, it is ignored and not echoed
    if(digit <= 0xF){
      number = number*0x10+digit;
      length++;
      UART_OutChar(character);
    }
// Backspace outputted and return value changed if a backspace is inputted
    else if((character==BS) && length){
      number /= 0x10;
      length--;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  return number;
}

//--------------------------UART_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void UART_OutUHex(unsigned long number){
  func = 24;
// This function uses recursion to convert the number of
//   unspecified length as an ASCII string
  if(number >= 0x10){
    UART_OutUHex(number/0x10);
    UART_OutUHex(number%0x10);
  }
  else{
    if(number < 0xA){
      UART_OutChar(number+'0');
     }
    else{
      UART_OutChar((number-0x0A)+'A');
    }
  }
}

//------------UART_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART_InString(char *bufPt, uint16_t max) {
  int length = 0;
	char character;
  func = 25;
  character = UART_InChar();
  while (character != CR) {
    if (character == BS && length) {
      bufPt--;
      length--;
      UART_OutChar(BS);
    } else if (length < max) {
      *bufPt = character;
      bufPt++;
      length++;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  *bufPt = 0;
}

//------------UART_InCMD------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART_InCMD(char *bufPt) {
	char *pt;
  static int mem_n = 0;
  int i, memory = -1, length = 0;
	char character;
  func = 26;
  character = UART_InChar();
  while (character != CR) {
    if (character == DEL) {
      if (length > 0) {
        bufPt--;
        length--;
        UART_OutChar(BS);
        UART_OutChar(SP);
        UART_OutChar(BS);
      }
    } else if (character == '\033') {
      UART_InChar();
      character = UART_InChar();
      if (character == UP) { // Up arrow
        if (memory < mem_n) {
          memory++;
          for (i = 0; i < length; i++) {
            bufPt--;
            UART_OutChar(BS);
            UART_OutChar(SP);
            UART_OutChar(BS);
          }
          strcpy(bufPt, cmd_memory[memory]);
          i = 0;
          while(cmd_memory[memory][i] != '\0') {
            bufPt++;
            i++;
          }
  				pt = cmd_memory[memory];
  				while(*pt){
  					UART_OutChar(*pt);
  					pt++;
  				}
          length = strlen(cmd_memory[memory]);
        }
      } else if (character == DOWN) { // Down arrow
        if (memory > 0) {
          memory--;
          for (i = 0; i < length; i++) {
            bufPt--;
            UART_OutChar(BS);
            UART_OutChar(SP);
            UART_OutChar(BS);
          }
          strcpy(bufPt, cmd_memory[memory]);
          i = 0;
          while(cmd_memory[memory][i] != '\0') {
            bufPt++;
            i++;
          }
  				pt = cmd_memory[memory];
  				while(*pt){
  					UART_OutChar(*pt);
  					pt++;
  				}
          length = strlen(cmd_memory[memory]);
        } else if (memory == 0) {
          memory--;
          for (i = 0; i < length; i++) {
            bufPt--;
            UART_OutChar(BS);
            UART_OutChar(SP);
            UART_OutChar(BS);
          }
          length = 0;
        }
      }
    } else if (length < MAX_CMD_LEN) {
      *bufPt = character;
      bufPt++;
      length++;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  *bufPt = 0;
  for (i = 0; i < length; i++) {
    bufPt--;
  }
  if (*bufPt != 0) {
    for (i = MAX_CMD_MEM - 1; i > 0; i--) {
      strcpy(cmd_memory[i],cmd_memory[i - 1]);
    }
    strcpy(cmd_memory[0], bufPt);
    if (mem_n < MAX_CMD_MEM - 1)
      mem_n++;
  }
}



/****************Fixed_Fix2Str***************
 converts fixed point number to ASCII string
 format signed 16-bit with resolution 0.01
 range -327.67 to +327.67
 Input: signed 16-bit integer part of fixed point number
         -32768 means invalid fixed-point number
 Output: null-terminated string exactly 8 characters plus null
 Examples
  12345 to " 123.45"
 -22100 to "-221.00"
   -102 to "  -1.02"
     31 to "   0.31"
 -32768 to " ***.**"
 */
void Fixed_Fix2Str(long const num,char *string){
  short n;
    func = 27;
  if((num>99999)||(num<-99990)){
    strcpy((char *)string," ***.**");
    return;
  }
  if(num<0){
    n = -num;
    string[0] = '-';
  } else{
    n = num;
    string[0] = ' ';
  }
  if(n>9999){
    string[1] = '0'+n/10000;
    n = n%10000;
    string[2] = '0'+n/1000;
  } else{
    if(n>999){
      if(num<0){
        string[0] = ' ';
        string[1] = '-';
      } else {
        string[1] = ' ';
      }
      string[2] = '0'+n/1000;
    } else{
      if(num<0){
        string[0] = ' ';
        string[1] = ' ';
        string[2] = '-';
      } else {
        string[1] = ' ';
        string[2] = ' ';
      }
    }
  }
  n = n%1000;
  string[3] = '0'+n/100;
  n = n%100;
  string[4] = '.';
  string[5] = '0'+n/10;
  n = n%10;
  string[6] = '0'+n;
  string[7] = 0;
}
//--------------------------UART_Fix2----------------------------
// Output a 32-bit number in 0.01 fixed-point format
// Input: 32-bit number to be transferred -99999 to +99999
// Output: none
// Fixed format
//  12345 to " 123.45"
// -22100 to "-221.00"
//   -102 to "  -1.02"
//     31 to "   0.31"
// error     " ***.**"
void UART_Fix2(long number){
  char message[10];
    func = 28;
  Fixed_Fix2Str(number,message);
  UART_OutString(message);
}

void UART_SetColor(char color) {
  func = 29;
  UART_OutChar(0x1B);
  UART_OutChar('[');
  switch (color) {
    case BLACK:
      UART_OutChar('3');
      UART_OutChar('0');
      break;
    case RED:
      UART_OutChar('3');
      UART_OutChar('1');
      break;
    case GREEN:
      UART_OutChar('3');
      UART_OutChar('2');
      break;
    case YELLOW:
      UART_OutChar('3');
      UART_OutChar('3');
      break;
    case BLUE:
      UART_OutChar('3');
      UART_OutChar('4');
      break;
    case MAGENTA:
      UART_OutChar('3');
      UART_OutChar('5');
      break;
    case CYAN:
      UART_OutChar('3');
      UART_OutChar('6');
      break;
    case WHITE:
      UART_OutChar('3');
      UART_OutChar('7');
      break;
    case RESET:
      UART_OutChar('0');
      break;
    default:
      UART_OutChar('0');
  }
  UART_OutChar('m');
}


void UART_OutError(char *string) {
  OS_bWait(&UART_FREE);
  func = 30;
  UART_OutChar(0x1B);
  UART_OutChar('[');
  UART_OutChar('3');
  UART_OutChar('1');
  UART_OutChar('m');
  while (*string != '\0') {
    UART_OutChar(*string);
    string++;
  }
  UART_OutChar(0x1B);
  UART_OutChar('[');
  UART_OutChar('0');
  UART_OutChar('m');
  OS_bSignal(&UART_FREE);
}

void UART_OutStringColor(char *string, char color) {
  OS_bWait(&UART_FREE);
  func = 31;
  UART_OutChar(0x1B);
  UART_OutChar('[');
  switch (color) {
    case BLACK:
      UART_OutChar('3');
      UART_OutChar('0');
      break;
    case RED:
      UART_OutChar('3');
      UART_OutChar('1');
      break;
    case GREEN:
      UART_OutChar('3');
      UART_OutChar('2');
      break;
    case YELLOW:
      UART_OutChar('3');
      UART_OutChar('3');
      break;
    case BLUE:
      UART_OutChar('3');
      UART_OutChar('4');
      break;
    case MAGENTA:
      UART_OutChar('3');
      UART_OutChar('5');
      break;
    case CYAN:
      UART_OutChar('3');
      UART_OutChar('6');
      break;
    case WHITE:
      UART_OutChar('3');
      UART_OutChar('7');
      break;
    case RESET:
      UART_OutChar('0');
      break;
    default:
      UART_OutChar('0');
  }
  UART_OutChar('m');
  while (*string != '\0') {
    UART_OutChar(*string);
    string++;
  }
  UART_OutChar(0x1B);
  UART_OutChar('[');
  UART_OutChar('0');
  UART_OutChar('m');
  OS_bSignal(&UART_FREE);
}
