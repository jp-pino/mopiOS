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
#include "UART1.h"
#include "OS.h"

#define NVIC_EN0_INT6            0x00000040  // Interrupt 5 enable
#define UART1_FR_RXFF            0x00000040  // UART Receive FIFO Full
#define UART1_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART1_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART1_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART1_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART1_CTL_UARTEN         0x00000001  // UART Enable
#define UART1_IFLS_RX1_8         0x00000000  // RX FIFO >= 1/8 full
#define UART1_IFLS_TX1_8         0x00000000  // TX FIFO <= 1/8 full
#define UART1_IM_RTIM            0x00000040  // UART Receive Time-Out Interrupt
                                            // Mask
#define UART1_IM_TXIM            0x00000020  // UART Transmit Interrupt Mask
#define UART1_IM_RXIM            0x00000010  // UART Receive Interrupt Mask
#define UART1_RIS_RTRIS          0x00000040  // UART Receive Time-Out Raw
                                            // Interrupt Status
#define UART1_RIS_TXRIS          0x00000020  // UART Transmit Raw Interrupt
                                            // Status
#define UART1_RIS_RXRIS          0x00000010  // UART Receive Raw Interrupt
                                            // Status
#define UART1_ICR_RTIC           0x00000040  // Receive Time-Out Interrupt Clear
#define UART1_ICR_TXIC           0x00000020  // Transmit Interrupt Clear
#define UART1_ICR_RXIC           0x00000010  // Receive Interrupt Clear
#define SYSCTL_RCGC1_UART0      0x00000001  // UART0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control

sema_t UART1_FREE;

// Two-pointer implementation of the FIFO
// can hold 0 to FIFOSIZE-1 elements
#define TXFIFOSIZE 32     // can be any size
#define FIFOSUCCESS 1
#define FIFOFAIL    0

static int func = 0;

typedef char dataType;
static dataType volatile *Tx1PutPt;  // put next
static dataType volatile *Tx1GetPt;  // get next
static dataType TxFifo1[TXFIFOSIZE];
static sema_t TxRoomLeft;   // Semaphore counting empty spaces in TxFifo1

static void TxFifo1_Init(void){ // this is critical
  // should make atomic
  func = 1;
  Tx1PutPt = Tx1GetPt = &TxFifo1[0]; // Empty
  OS_InitSemaphore("tx1_room_l", &TxRoomLeft, TXFIFOSIZE-1);// Initially lots of spaces
  // end of critical section
}

static void TxFifo1_Put(dataType data){
  func = 2;
  OS_Wait(&TxRoomLeft);      // If the buffer is full, spin/block
  func = 201;
  *(Tx1PutPt) = data;   // Put
  Tx1PutPt = Tx1PutPt+1;
  if(Tx1PutPt ==&TxFifo1[TXFIFOSIZE]){
    Tx1PutPt = &TxFifo1[0];      // wrap
  }
}

static int TxFifo1_Get(dataType *datapt){
  func = 3;
  if(Tx1PutPt == Tx1GetPt ){
    return(FIFOFAIL);// Empty if PutPt=GetPt
  }
  else{
    *datapt = *(Tx1GetPt++);
    if(Tx1GetPt==&TxFifo1[TXFIFOSIZE]){
       Tx1GetPt = &TxFifo1[0]; // wrap
    }
    OS_Signal(&TxRoomLeft); // increment if data removed
    return(FIFOSUCCESS);
  }
}
static unsigned int TxFifo1_Size(void){
  func = 4;
  return(((unsigned int)Tx1PutPt - (unsigned int)Tx1GetPt)& TXFIFOSIZE-1) ;
}

#define RXFIFOSIZE 16     // can be any size

static dataType volatile *Rx1PutPt;  // put next
static dataType volatile *Rx1GetPt;  // get next
static dataType RxFifo1[RXFIFOSIZE];
static sema_t RxFifo1Available;   // Semaphore counting data in RxFifo1

static void RxFifo1_Init(void){ // this is critical
  func = 5;

  // should make atomic
  Rx1PutPt = Rx1GetPt = &RxFifo1[0]; // Empty
  OS_InitSemaphore("rx1_fifo_a", &RxFifo1Available, 0); // Initially empty
  // end of critical section
}

static int RxFifo1_Put(dataType data){
  dataType volatile *nextPutPt;
  func = 6;
  nextPutPt = Rx1PutPt+1;
  if(nextPutPt ==&RxFifo1[RXFIFOSIZE]){
    nextPutPt = &RxFifo1[0];      // wrap
  }
  if(nextPutPt == Rx1GetPt){
    return(FIFOFAIL);  // Failed, fifo full
  }
  else{
    *(Rx1PutPt) = data;   // Put
    Rx1PutPt = nextPutPt; // Success, update
    OS_Signal(&RxFifo1Available); // increment only if data actually stored
    return(FIFOSUCCESS);
  }
}

static dataType RxFifo1_Get(void){
	dataType data;
  func = 7;
  OS_Wait(&RxFifo1Available);
  data = *(Rx1GetPt++);
  if(Rx1GetPt==&RxFifo1[RXFIFOSIZE]){
     Rx1GetPt = &RxFifo1[0]; // wrap
  }
  return data;
}

static unsigned int RxFifo1_Size(void){
  func = 8;
  return(((unsigned int)Rx1PutPt - (unsigned int)Rx1GetPt)& RXFIFOSIZE-1) ;

//  return RxFifo1Available.Value;
}

// Initialize UART0
// Baud rate is 115200 bits/sec
void UART1_Init(void){
  func = 9;

  OS_InitSemaphore("uart1_free", &UART1_FREE, 1);

  SYSCTL_RCGCUART_R |= 0x02; // activate UART1
  SYSCTL_RCGCGPIO_R |= 0x04; // activate port C
  RxFifo1_Init();                        // initialize empty FIFOs
  TxFifo1_Init();
  UART1_CTL_R &= ~UART1_CTL_UARTEN;     // disable UART
  UART1_IBRD_R = 43;                    // IBRD = int(80,000,000 / (16 * 115200)) = int(43.402778)
  UART1_FBRD_R = 26;                    // FBRD = round(0.402778 * 64) = 26
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART1_LCRH_R = (UART1_LCRH_WLEN_8|UART1_LCRH_FEN);
  UART1_IFLS_R &= ~0x3F;                // clear TX and RX interrupt FIFO level fields
                                        // configure interrupt for TX FIFO <= 1/8 full
                                        // configure interrupt for RX FIFO >= 1/8 full
  UART1_IFLS_R += (UART1_IFLS_TX1_8|UART1_IFLS_RX1_8);
                                        // enable TX and RX FIFO interrupts and RX time-out interrupt
  UART1_IM_R |= (UART1_IM_RXIM|UART1_IM_TXIM|UART1_IM_RTIM);
  UART1_CTL_R |= UART1_CTL_UARTEN;       // enable UART
  // Unlock GPIO Port F Commit Register
  GPIO_PORTC_LOCK_R = GPIO_LOCK_KEY;
  GPIO_PORTC_CR_R |= ((1 << 4) | (1 << 5) & 0xFF); // Enable commit for PC4-5
  GPIO_PORTC_AFSEL_R |= ((1 << 4) | (1 << 5) & 0xFF);           // enable alt funct on PA1-0 -> PC4-5
  GPIO_PORTC_DEN_R |= ((1 << 4) | (1 << 5) & 0xFF);             // enable digital I/O on PA1-0 -> PC4-5
                                        // configure PA1-0 as UART -> PC4-5
  GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R&0xFF00FFFF)+0x00220000;
  GPIO_PORTF_AMSEL_R &= ~((1 << 4) | (1 << 5) & 0xFF);             // disable analog functionality on PA -> PC
                                        // UART0=priority 2
  NVIC_PRI1_R = (NVIC_PRI1_R & 0xFF00FFFF) | (3 << 21); // bits 13-15
  NVIC_EN0_R |= NVIC_EN0_INT6;           // enable interrupt 6 in NVIC

  OS_EnableInterrupts();
}
// copy from hardware RX FIFO to software RX FIFO
// stop when hardware RX FIFO is empty or software RX FIFO is full
void static copyHardwareToSoftware(void){
  char letter;
    func = 10;
  while(((UART1_FR_R&UART1_FR_RXFE) == 0) && (RxFifo1_Size() < (RXFIFOSIZE - 1))){
    letter = UART1_DR_R;
    RxFifo1_Put(letter);
  }
}
// copy from software TX FIFO to hardware TX FIFO
// stop when software TX FIFO is empty or hardware TX FIFO is full
void static copySoftwareToHardware(void){
  char letter;
    func = 11;
  while(((UART1_FR_R&UART1_FR_TXFF) == 0) && (TxFifo1_Size() > 0)){
    TxFifo1_Get(&letter);
    UART1_DR_R = letter;
  }
}
// input ASCII character from UART
// spin if RxFifo1 is empty
char UART1_InChar(void){
  char letter;
    func = 12;
  letter = RxFifo1_Get(); // block or spin if empty
  return(letter);
}
// output ASCII character to UART
// spin if TxFifo1 is full
void UART1_OutChar(char data){
  func = 13;
  TxFifo1_Put(data);
  UART1_IM_R &= ~UART1_IM_TXIM;          // disable TX FIFO interrupt
  copySoftwareToHardware();
  UART1_IM_R |= UART1_IM_TXIM;           // enable TX FIFO interrupt
}

void UART1_OutChar2(char data){
  func = 14;
  TxFifo1_Put(data);
  UART1_IM_R &= ~UART1_IM_TXIM;          // disable TX FIFO interrupt
  copySoftwareToHardware();
  UART1_IM_R |= UART1_IM_TXIM;           // enable TX FIFO interrupt
}
// at least one of three things has happened:
// hardware TX FIFO goes from 3 to 2 or less items
// hardware RX FIFO goes from 1 to 2 or more items
// UART receiver has timed out
void UART1_Handler(void){
  func = 15;
  if(UART1_RIS_R&UART1_RIS_TXRIS){       // hardware TX FIFO <= 2 items
    UART1_ICR_R = UART1_ICR_TXIC;        // acknowledge TX FIFO
    // copy from software TX FIFO to hardware TX FIFO
    copySoftwareToHardware();
    if(TxFifo1_Size() == 0){             // software TX FIFO is empty
      UART1_IM_R &= ~UART1_IM_TXIM;      // disable TX FIFO interrupt
    }
  }
  if(UART1_RIS_R&UART1_RIS_RXRIS){       // hardware RX FIFO >= 2 items
    UART1_ICR_R = UART1_ICR_RXIC;        // acknowledge RX FIFO
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
  if(UART1_RIS_R&UART1_RIS_RTRIS){       // receiver timed out
    UART1_ICR_R = UART1_ICR_RTIC;        // acknowledge receiver time out
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
}

//------------UART1_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART1_OutString(char *pt){
  OS_bWait(&UART1_FREE);
  func = 16;
  while(*pt){
    UART1_OutChar(*pt);
    pt++;
  }
  OS_bSignal(&UART1_FREE);
}

//------------UART1_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART1_InUDec(void){
  unsigned long number=0, length=0;
  char character;
  func = 17;
  character = UART1_InChar();
  while(character != CR){
  // accepts until <enter> is typed
  // The next line checks that the input is a digit, 0-9.
  // If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART1_OutChar(character);
    }
  // If the input is a backspace, then the return number is
  // changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  return number;
}

//-----------------------UART1_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART1_OutUDecRec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  func = 18;
  if(n >= 10){
    UART1_OutUDecRec(n/10);
    n = n%10;
  }
  UART1_OutChar(n+'0'); /* n is between 0 and 9 */
}

void UART1_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  OS_bWait(&UART1_FREE);
  func = 19;
  if(n >= 10){
    UART1_OutUDecRec(n/10);
    n = n%10;
  }
  UART1_OutChar(n+'0'); /* n is between 0 and 9 */
  OS_bSignal(&UART1_FREE);
}

//-----------------------UART1_OutUDec3-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Fixed format 3 digits with space after
void UART1_OutUDec3(unsigned long n){
  func = 20;
  if(n>999){
    UART1_OutString("***");
  }else if(n >= 100){
    UART1_OutChar(n/100+'0');
    n = n%100;
    UART1_OutChar(n/10+'0');
    n = n%10;
    UART1_OutChar(n+'0');
  }else if(n >= 10){
    UART1_OutChar(' ');
    UART1_OutChar(n/10+'0');
    n = n%10;
    UART1_OutChar(n+'0');
  }else{
    UART1_OutChar(' ');
    UART1_OutChar(' ');
    UART1_OutChar(n+'0');
  }
  UART1_OutChar(' ');
}
//-----------------------UART1_OutUDec5-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Fixed format 5 digits with space after
void UART1_OutUDec5(unsigned long n){
  func = 21;
  if(n>99999){
    UART1_OutString("*****");
  }else if(n >= 10000){
    UART1_OutChar(n/10000+'0');
    n = n%10000;
    UART1_OutChar(n/1000+'0');
    n = n%1000;
    UART1_OutChar(n/100+'0');
    n = n%100;
    UART1_OutChar(n/10+'0');
    n = n%10;
    UART1_OutChar(n+'0');
  }else if(n >= 1000){
    UART1_OutChar(' ');
    UART1_OutChar(n/1000+'0');
    n = n%1000;
    UART1_OutChar(n/100+'0');
    n = n%100;
    UART1_OutChar(n/10+'0');
    n = n%10;
    UART1_OutChar(n+'0');
  }else if(n >= 100){
    UART1_OutChar(' ');
    UART1_OutChar(' ');
    UART1_OutChar(n/100+'0');
    n = n%100;
    UART1_OutChar(n/10+'0');
    n = n%10;
    UART1_OutChar(n+'0');
  }else if(n >= 10){
    UART1_OutChar(' ');
    UART1_OutChar(' ');
    UART1_OutChar(' ');
    UART1_OutChar(n/10+'0');
    n = n%10;
    UART1_OutChar(n+'0');
  }else{
    UART1_OutChar(' ');
    UART1_OutChar(' ');
    UART1_OutChar(' ');
    UART1_OutChar(' ');
    UART1_OutChar(n+'0');
  }
  UART1_OutChar(' ');
}
//-----------------------UART1_OutSDec-----------------------
// Output a 32-bit number in signed decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART1_OutSDec(long n){
  func = 22;
  if(n<0){
    UART1_OutChar('-');
    n = -n;
  }
  UART1_OutUDec((unsigned long)n);
}
//---------------------UART1_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
// No '$' or '0x' need be entered, just the 1 to 8 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     value range is 0 to FFFFFFFF
// If you enter a number above FFFFFFFF, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART1_InUHex(void){
unsigned long number=0, digit, length=0;
char character;
  func = 23;
  character = UART1_InChar();
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
      UART1_OutChar(character);
    }
// Backspace outputted and return value changed if a backspace is inputted
    else if((character==BS) && length){
      number /= 0x10;
      length--;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  return number;
}

//--------------------------UART1_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void UART1_OutUHex(unsigned long number){
  func = 24;
// This function uses recursion to convert the number of
//   unspecified length as an ASCII string
  if(number >= 0x10){
    UART1_OutUHex(number/0x10);
    UART1_OutUHex(number%0x10);
  }
  else{
    if(number < 0xA){
      UART1_OutChar(number+'0');
     }
    else{
      UART1_OutChar((number-0x0A)+'A');
    }
  }
}

//------------UART1_InString------------
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
void UART1_InString(char *bufPt, uint16_t max) {
  int length = 0;
	char character;
  func = 25;
  character = UART1_InChar();
  while (character != CR) {
    if (character == BS && length) {
      bufPt--;
      length--;
      UART1_OutChar(BS);
    } else if (length < max) {
      *bufPt = character;
      bufPt++;
      length++;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  *bufPt = 0;
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
static void Fixed_Fix2Str(long const num,char *string){
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
//--------------------------UART1_Fix2----------------------------
// Output a 32-bit number in 0.01 fixed-point format
// Input: 32-bit number to be transferred -99999 to +99999
// Output: none
// Fixed format
//  12345 to " 123.45"
// -22100 to "-221.00"
//   -102 to "  -1.02"
//     31 to "   0.31"
// error     " ***.**"
void UART1_Fix2(long number){
  char message[10];
    func = 28;
  Fixed_Fix2Str(number,message);
  UART1_OutString(message);
}

void UART1_SetColor(char color) {
  func = 29;
  UART1_OutChar(0x1B);
  UART1_OutChar('[');
  switch (color) {
    case BLACK:
      UART1_OutChar('3');
      UART1_OutChar('0');
      break;
    case RED:
      UART1_OutChar('3');
      UART1_OutChar('1');
      break;
    case GREEN:
      UART1_OutChar('3');
      UART1_OutChar('2');
      break;
    case YELLOW:
      UART1_OutChar('3');
      UART1_OutChar('3');
      break;
    case BLUE:
      UART1_OutChar('3');
      UART1_OutChar('4');
      break;
    case MAGENTA:
      UART1_OutChar('3');
      UART1_OutChar('5');
      break;
    case CYAN:
      UART1_OutChar('3');
      UART1_OutChar('6');
      break;
    case WHITE:
      UART1_OutChar('3');
      UART1_OutChar('7');
      break;
    case RESET:
      UART1_OutChar('0');
      break;
    default:
      UART1_OutChar('0');
  }
  UART1_OutChar('m');
}


void UART1_OutError(char *string) {
  OS_bWait(&UART1_FREE);
  func = 30;
  UART1_OutChar(0x1B);
  UART1_OutChar('[');
  UART1_OutChar('3');
  UART1_OutChar('1');
  UART1_OutChar('m');
  while (*string != '\0') {
    UART1_OutChar(*string);
    string++;
  }
  UART1_OutChar(0x1B);
  UART1_OutChar('[');
  UART1_OutChar('0');
  UART1_OutChar('m');
  OS_bSignal(&UART1_FREE);
}

void UART1_OutStringColor(char *string, char color) {
  OS_bWait(&UART1_FREE);
  func = 31;
  UART1_OutChar(0x1B);
  UART1_OutChar('[');
  switch (color) {
    case BLACK:
      UART1_OutChar('3');
      UART1_OutChar('0');
      break;
    case RED:
      UART1_OutChar('3');
      UART1_OutChar('1');
      break;
    case GREEN:
      UART1_OutChar('3');
      UART1_OutChar('2');
      break;
    case YELLOW:
      UART1_OutChar('3');
      UART1_OutChar('3');
      break;
    case BLUE:
      UART1_OutChar('3');
      UART1_OutChar('4');
      break;
    case MAGENTA:
      UART1_OutChar('3');
      UART1_OutChar('5');
      break;
    case CYAN:
      UART1_OutChar('3');
      UART1_OutChar('6');
      break;
    case WHITE:
      UART1_OutChar('3');
      UART1_OutChar('7');
      break;
    case RESET:
      UART1_OutChar('0');
      break;
    default:
      UART1_OutChar('0');
  }
  UART1_OutChar('m');
  while (*string != '\0') {
    UART1_OutChar(*string);
    string++;
  }
  UART1_OutChar(0x1B);
  UART1_OutChar('[');
  UART1_OutChar('0');
  UART1_OutChar('m');
  OS_bSignal(&UART1_FREE);
}
