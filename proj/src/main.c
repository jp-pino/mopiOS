//*****************************************************************************
//
// Lab5.c - Barebones OS initialization
//*****************************************************************************

#include "tm4c123gh6pm.h"
#include "OS.h"
#include "joystick.h"
#include "teleop.h"

#define TIMESLICE TIME_2MS  //thread switch time in system time units


void task1(void) {
  OS_StartCritical();
  PF1 ^= 0x02;

  SYSCTL_RCGCUART_R |= 0x01; // activate UART0
  SYSCTL_RCGCGPIO_R |= 0x01; // activate port A
  RxFifo_Init();                        // initialize empty FIFOs
  TxFifo_Init();
  UART0_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  UART0_IBRD_R = 43;                    // IBRD = int(80,000,000 / (16 * 115200)) = int(43.402778)
  UART0_FBRD_R = 26;                    // FBRD = round(0.402778 * 64) = 26
                                        // 8 bit word length (no parity bits, one stop bit)
  UART0_LCRH_R = (UART_LCRH_WLEN_8);
  UART0_CTL_R |= UART_CTL_UARTEN;       // enable UART

  PF2 ^= 0x04;
  GPIO_PORTA_AFSEL_R |= 0x03;           // enable alt funct on PA1-0
  GPIO_PORTA_DEN_R |= 0x03;             // enable digital I/O on PA1-0
                                        // configure PA1-0 as UART
  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x00000011;
  GPIO_PORTA_AMSEL_R = 0;               // disable analog functionality on PA

  PF1 ^= 0x02;


  NVIC_ST_CTRL_R = 0;    // disable SysTick during setup
  PF2 ^= 0x04;
  ROM_UpdateUART();
}

void task2(void) {
  PF1 ^= 0x02;
}

// OS and modules initialization
int main(void) {        // lab 4 real main

  // Initialize the OS
  OS_Init();           // initialize, disable interrupts

	// OS_AddThread("test", &test, 512, 5);
	// Joystick_Init();
	// Teleop_Init();
	// Speed_Init();

  OS_AddSW1Task(&task1, 0);
  OS_AddSW2Task(&task2, 1);

  // Launch the OS
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}

// Kill thread by name
