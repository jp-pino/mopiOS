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
#include "UART5.h"
#include "OS.h"

#if UART5_Enabled == 1

#define NVIC_EN1_INT61           0x20000000  // Interrupt 5 enable
#define UART5_FR_RXFF            0x00000040  // UART Receive FIFO Full
#define UART5_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART5_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART5_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART5_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART5_CTL_UARTEN         0x00000001  // UART Enable
#define UART5_IFLS_RX1_8         0x00000000  // RX FIFO >= 1/8 full
#define UART5_IFLS_TX1_8         0x00000000  // TX FIFO <= 1/8 full
#define UART5_IM_RTIM            0x00000040  // UART Receive Time-Out Interrupt
                                            // Mask
#define UART5_IM_TXIM            0x00000020  // UART Transmit Interrupt Mask
#define UART5_IM_RXIM            0x00000010  // UART Receive Interrupt Mask
#define UART5_RIS_RTRIS          0x00000040  // UART Receive Time-Out Raw
                                            // Interrupt Status
#define UART5_RIS_TXRIS          0x00000020  // UART Transmit Raw Interrupt
                                            // Status
#define UART5_RIS_RXRIS          0x00000010  // UART Receive Raw Interrupt
                                            // Status
#define UART5_ICR_RTIC           0x00000040  // Receive Time-Out Interrupt Clear
#define UART5_ICR_TXIC           0x00000020  // Transmit Interrupt Clear
#define UART5_ICR_RXIC           0x00000010  // Receive Interrupt Clear
#define SYSCTL_RCGC1_UART0      0x00000001  // UART0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control

sema_t UART5_FREE;

// Two-pointer implementation of the FIFO
// can hold 0 to FIFOSIZE-1 elements
#define TXFIFOSIZE 32     // can be any size
#define FIFOSUCCESS 1
#define FIFOFAIL    0

static int func = 0;

typedef char dataType;
static dataType volatile *Tx5PutPt;  // put next
static dataType volatile *Tx5GetPt;  // get next
static dataType TxFifo5[TXFIFOSIZE];
sema_t TxFifo5RoomLeft;   // Semaphore counting empty spaces in TxFifo5

void TxFifo5_Init(void){ // this is critical
  // should make atomic
  func = 1;
  Tx5PutPt = Tx5GetPt = &TxFifo5[0]; // Empty
  OS_InitSemaphore("tx5_room_l", &TxFifo5RoomLeft, TXFIFOSIZE-1);// Initially lots of spaces
  // end of critical section
}

void TxFifo5_Put(dataType data){
  func = 2;
  OS_Wait(&TxFifo5RoomLeft);      // If the buffer is full, spin/block
  func = 201;
  *(Tx5PutPt) = data;   // Put
  Tx5PutPt = Tx5PutPt+1;
  if(Tx5PutPt ==&TxFifo5[TXFIFOSIZE]){
    Tx5PutPt = &TxFifo5[0];      // wrap
  }
}

int TxFifo5_Get(dataType *datapt){
  func = 3;
  if(Tx5PutPt == Tx5GetPt ){
    return(FIFOFAIL);// Empty if PutPt=GetPt
  }
  else{
    *datapt = *(Tx5GetPt++);
    if(Tx5GetPt==&TxFifo5[TXFIFOSIZE]){
       Tx5GetPt = &TxFifo5[0]; // wrap
    }
    OS_Signal(&TxFifo5RoomLeft); // increment if data removed
    return(FIFOSUCCESS);
  }
}
unsigned int TxFifo5_Size(void){
  func = 4;
  return(((unsigned int)Tx5PutPt - (unsigned int)Tx5GetPt)& TXFIFOSIZE-1) ;
}

#define RXFIFOSIZE 32     // can be any size

dataType volatile *Rx5PutPt;  // put next
dataType volatile *Rx5GetPt;  // get next
dataType RxFifo5[RXFIFOSIZE];
sema_t RxFifo5Available;   // Semaphore counting data in RxFifo5

void RxFifo5_Init(void){ // this is critical
  func = 5;

  // should make atomic
  Rx5PutPt = Rx5GetPt = &RxFifo5[0]; // Empty
  OS_InitSemaphore("rx5_fifo_a", &RxFifo5Available, 0); // Initially empty
  // end of critical section
}

int RxFifo5_Put(dataType data){
  dataType volatile *nextPutPt;
  func = 6;
  nextPutPt = Rx5PutPt+1;
  if(nextPutPt ==&RxFifo5[RXFIFOSIZE]){
    nextPutPt = &RxFifo5[0];      // wrap
  }
  if(nextPutPt == Rx5GetPt){
    return(FIFOFAIL);  // Failed, fifo full
  }
  else{
    *(Rx5PutPt) = data;   // Put
    Rx5PutPt = nextPutPt; // Success, update
    OS_Signal(&RxFifo5Available); // increment only if data actually stored
    return(FIFOSUCCESS);
  }
}

dataType RxFifo5_Get(void){
	dataType data;
  func = 7;
  OS_Wait(&RxFifo5Available);
  data = *(Rx5GetPt++);
  if(Rx5GetPt==&RxFifo5[RXFIFOSIZE]){
     Rx5GetPt = &RxFifo5[0]; // wrap
  }
  return data;
}

unsigned int RxFifo5_Size(void){
  func = 8;
  return(((unsigned int)Rx5PutPt - (unsigned int)Rx5GetPt)& RXFIFOSIZE-1) ;

//  return RxFifo5Available.Value;
}

// Initialize UART0
// Baud rate is 115200 bits/sec
void UART5_Init(void){
  func = 9;

  OS_InitSemaphore("uart5_free", &UART5_FREE, 1);


  SYSCTL_RCGCUART_R |= 0x20; // Enable UART5
  while((SYSCTL_PRUART_R&0x20)==0){};
  SYSCTL_RCGCGPIO_R |= 0x10; // Enable PORT E clock gating
  while((SYSCTL_PRGPIO_R&0x10)==0){};

  RxFifo5_Init();                        // initialize empty FIFOs
  TxFifo5_Init();

	// Unlock GPIO Port F Commit Register
  // GPIO_PORTE_LOCK_R = GPIO_LOCK_KEY;
  // GPIO_PORTE_CR_R |= ((1 << 4) | (1 << 5) & 0xFF); // Enable commit for PC4-5
  GPIO_PORTE_AFSEL_R |= ((1 << 4) | (1 << 5) & 0xFF);           // enable alt funct on PA1-0 -> PC4-5
  GPIO_PORTE_DIR_R |= ((1 << 4) | (1 << 5) & 0xFF);   // PB5 output to reset
  GPIO_PORTE_DEN_R |= ((1 << 4) | (1 << 5) & 0xFF);   // enable digital I/O on PA1-0 -> PC4-5
                                        // configure PA1-0 as UART -> PC4-5
  GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R&0xFF00FFFF)+0x00110000;
  GPIO_PORTE_AMSEL_R &= ~((1 << 4) | (1 << 5) & 0xFF);             // disable analog functionality on PA -> PC

	GPIO_PORTE_DATA_R |= ((1 << 5) & 0xFF);
	GPIO_PORTE_DATA_R &= ~((1 << 4) & 0xFF);


  // GPIO_PORTE_LOCK_R = GPIO_LOCK_M;


  UART5_CTL_R &= ~UART5_CTL_UARTEN;     // disable UART
  UART5_IBRD_R = 43;                    // IBRD = int(80,000,000 / (16 * 115200)) = int(43.402778)
  UART5_FBRD_R = 26;                    // FBRD = round(0.402778 * 64) = 26
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART5_LCRH_R = (UART5_LCRH_WLEN_8|UART5_LCRH_FEN);
  UART5_IFLS_R &= ~0x3F;                // clear TX and RX interrupt FIFO level fields
                                        // configure interrupt for TX FIFO <= 1/8 full
                                        // configure interrupt for RX FIFO >= 1/8 full
  UART5_IFLS_R += (UART5_IFLS_TX1_8|UART5_IFLS_RX1_8);
                                        // enable TX and RX FIFO interrupts and RX time-out interrupt
  UART5_IM_R |= (UART5_IM_RXIM|UART5_IM_TXIM|UART5_IM_RTIM);
  UART5_CTL_R |= UART5_CTL_UARTEN;       // enable UART



                                        // UART0=priority 2
  NVIC_PRI15_R = (NVIC_PRI15_R & 0xFFFF00FF) | (3 << 13); // bits 13-15
  NVIC_EN1_R |= NVIC_EN1_INT61;           // enable interrupt 6 in NVIC

  OS_EnableInterrupts();
}
// copy from hardware RX FIFO to software RX FIFO
// stop when hardware RX FIFO is empty or software RX FIFO is full
void static copyHardwareToSoftware(void){
  char letter;
    func = 10;
  while(((UART5_FR_R&UART5_FR_RXFE) == 0) && (RxFifo5_Size() < (RXFIFOSIZE - 1))){
    letter = UART5_DR_R;
    RxFifo5_Put(letter);
  }
}
// copy from software TX FIFO to hardware TX FIFO
// stop when software TX FIFO is empty or hardware TX FIFO is full
void static copySoftwareToHardware(void){
  char letter;
    func = 11;
  while(((UART5_FR_R&UART5_FR_TXFF) == 0) && (TxFifo5_Size() > 0)){
    TxFifo5_Get(&letter);
    UART5_DR_R = letter;
  }
}
// input ASCII character from UART
// spin if RxFifo5 is empty
char UART5_InChar(void){
  char letter;
    func = 12;
  letter = RxFifo5_Get(); // block or spin if empty
  return(letter);
}
// output ASCII character to UART
// spin if TxFifo5 is full
void UART5_OutChar(char data){
  func = 13;
  TxFifo5_Put(data);
  UART5_IM_R &= ~UART5_IM_TXIM;          // disable TX FIFO interrupt
  copySoftwareToHardware();
  UART5_IM_R |= UART5_IM_TXIM;           // enable TX FIFO interrupt
}

void UART5_OutChar2(char data){
  func = 14;
  TxFifo5_Put(data);
  UART5_IM_R &= ~UART5_IM_TXIM;          // disable TX FIFO interrupt
  copySoftwareToHardware();
  UART5_IM_R |= UART5_IM_TXIM;           // enable TX FIFO interrupt
}
// at least one of three things has happened:
// hardware TX FIFO goes from 3 to 2 or less items
// hardware RX FIFO goes from 1 to 2 or more items
// UART receiver has timed out
void UART5_Handler(void){
  func = 15;
  if(UART5_RIS_R&UART5_RIS_TXRIS){       // hardware TX FIFO <= 2 items
    UART5_ICR_R = UART5_ICR_TXIC;        // acknowledge TX FIFO
    // copy from software TX FIFO to hardware TX FIFO
    copySoftwareToHardware();
    if(TxFifo5_Size() == 0){             // software TX FIFO is empty
      UART5_IM_R &= ~UART5_IM_TXIM;      // disable TX FIFO interrupt
    }
  }
  if(UART5_RIS_R&UART5_RIS_RXRIS){       // hardware RX FIFO >= 2 items
    UART5_ICR_R = UART5_ICR_RXIC;        // acknowledge RX FIFO
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
  if(UART5_RIS_R&UART5_RIS_RTRIS){       // receiver timed out
    UART5_ICR_R = UART5_ICR_RTIC;        // acknowledge receiver time out
    // copy from hardware RX FIFO to software RX FIFO
    copyHardwareToSoftware();
  }
}

//------------UART5_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART5_OutString(char *pt){
  OS_bWait(&UART5_FREE);
  func = 16;
  while(*pt){
    UART5_OutChar(*pt);
    pt++;
  }
  OS_bSignal(&UART5_FREE);
}

//------------UART5_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART5_InUDec(void){
  unsigned long number=0, length=0;
  char character;
  func = 17;
  character = UART5_InChar();
  while(character != CR){
  // accepts until <enter> is typed
  // The next line checks that the input is a digit, 0-9.
  // If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART5_OutChar(character);
    }
  // If the input is a backspace, then the return number is
  // changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART5_OutChar(character);
    }
    character = UART5_InChar();
  }
  return number;
}

//-----------------------UART5_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART5_OutUDecRec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  func = 18;
  if(n >= 10){
    UART5_OutUDecRec(n/10);
    n = n%10;
  }
  UART5_OutChar(n+'0'); /* n is between 0 and 9 */
}

void UART5_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  OS_bWait(&UART5_FREE);
  func = 19;
  if(n >= 10){
    UART5_OutUDecRec(n/10);
    n = n%10;
  }
  UART5_OutChar(n+'0'); /* n is between 0 and 9 */
  OS_bSignal(&UART5_FREE);
}

//-----------------------UART5_OutUDec3-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Fixed format 3 digits with space after
void UART5_OutUDec3(unsigned long n){
  func = 20;
  if(n>999){
    UART5_OutString("***");
  }else if(n >= 100){
    UART5_OutChar(n/100+'0');
    n = n%100;
    UART5_OutChar(n/10+'0');
    n = n%10;
    UART5_OutChar(n+'0');
  }else if(n >= 10){
    UART5_OutChar(' ');
    UART5_OutChar(n/10+'0');
    n = n%10;
    UART5_OutChar(n+'0');
  }else{
    UART5_OutChar(' ');
    UART5_OutChar(' ');
    UART5_OutChar(n+'0');
  }
  UART5_OutChar(' ');
}
//-----------------------UART5_OutUDec5-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Fixed format 5 digits with space after
void UART5_OutUDec5(unsigned long n){
  func = 21;
  if(n>99999){
    UART5_OutString("*****");
  }else if(n >= 10000){
    UART5_OutChar(n/10000+'0');
    n = n%10000;
    UART5_OutChar(n/1000+'0');
    n = n%1000;
    UART5_OutChar(n/100+'0');
    n = n%100;
    UART5_OutChar(n/10+'0');
    n = n%10;
    UART5_OutChar(n+'0');
  }else if(n >= 1000){
    UART5_OutChar(' ');
    UART5_OutChar(n/1000+'0');
    n = n%1000;
    UART5_OutChar(n/100+'0');
    n = n%100;
    UART5_OutChar(n/10+'0');
    n = n%10;
    UART5_OutChar(n+'0');
  }else if(n >= 100){
    UART5_OutChar(' ');
    UART5_OutChar(' ');
    UART5_OutChar(n/100+'0');
    n = n%100;
    UART5_OutChar(n/10+'0');
    n = n%10;
    UART5_OutChar(n+'0');
  }else if(n >= 10){
    UART5_OutChar(' ');
    UART5_OutChar(' ');
    UART5_OutChar(' ');
    UART5_OutChar(n/10+'0');
    n = n%10;
    UART5_OutChar(n+'0');
  }else{
    UART5_OutChar(' ');
    UART5_OutChar(' ');
    UART5_OutChar(' ');
    UART5_OutChar(' ');
    UART5_OutChar(n+'0');
  }
  UART5_OutChar(' ');
}
//-----------------------UART5_OutSDec-----------------------
// Output a 32-bit number in signed decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART5_OutSDec(long n){
  func = 22;
  if(n<0){
    UART5_OutChar('-');
    n = -n;
  }
  UART5_OutUDec((unsigned long)n);
}
//---------------------UART5_InUHex----------------------------------------
// Accepts ASCII input in unsigned hexadecimal (base 16) format
// Input: none
// Output: 32-bit unsigned number
// No '$' or '0x' need be entered, just the 1 to 8 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     value range is 0 to FFFFFFFF
// If you enter a number above FFFFFFFF, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART5_InUHex(void){
unsigned long number=0, digit, length=0;
char character;
  func = 23;
  character = UART5_InChar();
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
      UART5_OutChar(character);
    }
// Backspace outputted and return value changed if a backspace is inputted
    else if((character==BS) && length){
      number /= 0x10;
      length--;
      UART5_OutChar(character);
    }
    character = UART5_InChar();
  }
  return number;
}

//--------------------------UART5_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void UART5_OutUHex(unsigned long number){
  func = 24;
// This function uses recursion to convert the number of
//   unspecified length as an ASCII string
  if(number >= 0x10){
    UART5_OutUHex(number/0x10);
    UART5_OutUHex(number%0x10);
  }
  else{
    if(number < 0xA){
      UART5_OutChar(number+'0');
     }
    else{
      UART5_OutChar((number-0x0A)+'A');
    }
  }
}

//------------UART5_InString------------
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
void UART5_InString(char *bufPt, uint16_t max) {
  int length = 0;
	char character;
  func = 25;
  character = UART5_InChar();
  while (character != CR) {
    if (character == BS && length) {
      bufPt--;
      length--;
      UART5_OutChar(BS);
    } else if (length < max) {
      *bufPt = character;
      bufPt++;
      length++;
      UART5_OutChar(character);
    }
    character = UART5_InChar();
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
//--------------------------UART5_Fix2----------------------------
// Output a 32-bit number in 0.01 fixed-point format
// Input: 32-bit number to be transferred -99999 to +99999
// Output: none
// Fixed format
//  12345 to " 123.45"
// -22100 to "-221.00"
//   -102 to "  -1.02"
//     31 to "   0.31"
// error     " ***.**"
void UART5_Fix2(long number){
  char message[10];
    func = 28;
  Fixed_Fix2Str(number,message);
  UART5_OutString(message);
}

void UART5_SetColor(char color) {
  func = 29;
  UART5_OutChar(0x1B);
  UART5_OutChar('[');
  switch (color) {
    case BLACK:
      UART5_OutChar('3');
      UART5_OutChar('0');
      break;
    case RED:
      UART5_OutChar('3');
      UART5_OutChar('1');
      break;
    case GREEN:
      UART5_OutChar('3');
      UART5_OutChar('2');
      break;
    case YELLOW:
      UART5_OutChar('3');
      UART5_OutChar('3');
      break;
    case BLUE:
      UART5_OutChar('3');
      UART5_OutChar('4');
      break;
    case MAGENTA:
      UART5_OutChar('3');
      UART5_OutChar('5');
      break;
    case CYAN:
      UART5_OutChar('3');
      UART5_OutChar('6');
      break;
    case WHITE:
      UART5_OutChar('3');
      UART5_OutChar('7');
      break;
    case RESET:
      UART5_OutChar('0');
      break;
    default:
      UART5_OutChar('0');
  }
  UART5_OutChar('m');
}


void UART5_OutError(char *string) {
  OS_bWait(&UART5_FREE);
  func = 30;
  UART5_OutChar(0x1B);
  UART5_OutChar('[');
  UART5_OutChar('3');
  UART5_OutChar('1');
  UART5_OutChar('m');
  while (*string != '\0') {
    UART5_OutChar(*string);
    string++;
  }
  UART5_OutChar(0x1B);
  UART5_OutChar('[');
  UART5_OutChar('0');
  UART5_OutChar('m');
  OS_bSignal(&UART5_FREE);
}

void UART5_OutStringColor(char *string, char color) {
  OS_bWait(&UART5_FREE);
  func = 31;
  UART5_OutChar(0x1B);
  UART5_OutChar('[');
  switch (color) {
    case BLACK:
      UART5_OutChar('3');
      UART5_OutChar('0');
      break;
    case RED:
      UART5_OutChar('3');
      UART5_OutChar('1');
      break;
    case GREEN:
      UART5_OutChar('3');
      UART5_OutChar('2');
      break;
    case YELLOW:
      UART5_OutChar('3');
      UART5_OutChar('3');
      break;
    case BLUE:
      UART5_OutChar('3');
      UART5_OutChar('4');
      break;
    case MAGENTA:
      UART5_OutChar('3');
      UART5_OutChar('5');
      break;
    case CYAN:
      UART5_OutChar('3');
      UART5_OutChar('6');
      break;
    case WHITE:
      UART5_OutChar('3');
      UART5_OutChar('7');
      break;
    case RESET:
      UART5_OutChar('0');
      break;
    default:
      UART5_OutChar('0');
  }
  UART5_OutChar('m');
  while (*string != '\0') {
    UART5_OutChar(*string);
    string++;
  }
  UART5_OutChar(0x1B);
  UART5_OutChar('[');
  UART5_OutChar('0');
  UART5_OutChar('m');
  OS_bSignal(&UART5_FREE);
}


void UART5_InResponse(char *bufPt, uint16_t max) {
  int length = 0;
	char character[6];
	character[0] = 0;
	character[1] = 0;
	character[2] = 0;
	character[3] = 0;
	character[4] = 0;
  func = 25;

  character[5] = UART5_InChar();
  while (character[0] != '\r' || character[1] != '\n' || character[2] != 'O' || character[3] != 'K' || character[4] != '\r' || character[5] != '\n' ) {
    if (length < max) {
      *bufPt = character[5];
      bufPt++;
      length++;
    }
		for (int i = 0; i < 5; i++) {
			character[i] = character[i+1];
		}
    character[5] = UART5_InChar();
  }


  *bufPt = 0;
}

#endif