/**
 * @file ADC.c
 * @author Cole Morgan, Juan Pablo Pino
 * @date February 4, 2019
 *
 * Provide functions that open one ADC channel to be sampled
 * by calls to ADC_In or ADC_Collect.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "ADCSWTrigger.h"
#include "ADC.h"
#include "OS.h"
#include "Interpreter.h"
#include "ST7735.h"
#include "UART.h"

// Interpreter function definitions
void adc(void);
void adc_o(void);
void adc_i(void);
void adc_c(void);

void (*ADCTask)(unsigned long) = 0;
int32_t numberOfSamples;
int currentSamples;

void ADC_InitIT(void) {
  cmd_t *cmd;
  cmd = IT_AddCommand("adc", 0, "", &adc, "");
  IT_AddFlag(cmd, 'o', 1, "[chan]" , &adc_o, "open a channel");
  IT_AddFlag(cmd, 'i', 0, "" , &adc_i, "read value from open channel");
  IT_AddFlag(cmd, 'c', 3, "[chan] [freq] [samps]" , &adc_c, "read value periodically");
}

/** Opens specified ADC channel
 * @param channelNum Number of ADC channel to be opened
 * @return 0 for success or 1 for error
 */
int ADC_Init(uint32_t channelNum) {
    if(channelNum > 11)
        return 1;
    ADC0_InitSWTriggerSeq3(channelNum);
    return 0;
}

/** Samples the ADC on the open channel
 * @return 12 bit unsigned ADC result
 */
uint16_t ADC_In(void) {
    return (uint16_t) ADC0_InSeq3();
}

/** Sets up periodic ADC sampling on specified channel
 * @param channelNum Number of ADC channel to collect data on
 * @param fs Desired sample rate of ADC in Hz
 * @param task Pointer to task into which the ADC data will be passed
 * @param numberOfSamples Number of ADC samples to be collected into buffer
 * @return 0 for success or 1 for error
 */
int ADC_Collect(uint32_t channelNum, uint32_t fs, void(*t)(unsigned long), int32_t n) {
	if (ADCTask != 0)
		return 1;
  // Store periodic task
	ADCTask = t;
  numberOfSamples = n;
	currentSamples = 0;

	// Activate TIMER2: SYSCTL_RCGCTIMER_R bit 5
	SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R2;

	// 1) Ensure the timer is disabled (the TnEN bit in the GPTMCTL register
	// is cleared) before making any changes
	TIMER2_CTL_R &= ~TIMER_CTL_TAEN;

	// 2) Write the GPTM Configuration Register (GPTMCFG) with a value of
	// 0x0000.0000.
	TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;

	// 3) Configure the TnMR field in the GPTM Timer n Mode Register (GPTMTnMR):
	//   a. Write a value of 0x1 for One-Shot mode.
	//   b. Write a value of 0x2 for Periodic mode.
	TIMER2_TAMR_R = TIMER_TAMR_TAMR_PERIOD;

	// 4) Optional
	// Bus clock resolution (prescaler)
	TIMER2_TAPR_R = 0x00;
	// Clear TIMER2A timeout flag
	TIMER2_ICR_R |= TIMER_ICR_TATOCINT;

	// 5) Load the start value into the GPTM Timer n Interval Load Register
	// (GPTMTnILR).
	TIMER2_TAILR_R = 80000000/fs - 1;

	// 6) If interrupts are required, set the appropriate bits in the GPTM
	// Interrupt Mask Register (GPTMIMR)
	TIMER2_IMR_R |= TIMER_IMR_TATOIM;

	// Priority
	NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|(3<<29);
	// interrupts enabled in the main program after all devices initialized
	// Enable IRQ 23 in NVIC
	NVIC_EN0_R = 1<<(23);


	// 7) Set the TnEN bit in the GPTMCTL register to enable the timer and
	// start counting.
  TIMER2_CTL_R |= TIMER_CTL_TAEN;

	ADC_Init(channelNum);
	return 0;
}

// void Timer2A_Handler(void){
// 	// 8) Poll the GPTMRIS register or wait for the interrupt to be generated
// 	// (if enabled). In both cases, the status flags are cleared by writing a 1
// 	// to the appropriate bit of the GPTM Interrupt Clear Register (GPTMICR)
// 	TIMER2_ICR_R |= TIMER_ICR_TATOCINT;// acknowledge TIMER2 timeout
//   if (ADCTask != 0) {
// 		(*ADCTask)(ADC_In());
// 		currentSamples++;
// 		if (numberOfSamples >= 0 && currentSamples >= numberOfSamples) {
// 			TIMER2_CTL_R &= ~TIMER_CTL_TAEN;
// 			ADCTask = 0;
// 			currentSamples = 0;
// 		}
// 	}
// }



// Interpreter
void adc(void) {
  UART_OutError("\r\nERROR: COMMAND \"adc\" EXPECTS FLAGS");
}

void adc_o(void) {
  char buffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];
  IT_GetBuffer(buffer);

  if (digits_only(buffer[0]) == 0) {
    UART_OutError("\r\nERROR: the channel number can only contain digits 0 - 9\r\n");
  } else {
    ADC_Init(atoi(buffer[0]));
  }
}

void adc_i(void) {
  uint16_t adc = ADC_In();
  UART_OutString("\r\n    ");
  UART_OutUDec3(adc);
}

void adc_c_task(unsigned long value) {
  static int i = 0;
	ST7735_Message(ST7735_DISPLAY_TOP, 1, "Run:", i++);
	ST7735_Message(ST7735_DISPLAY_TOP, 2, "Val:", value);
}

void adc_c(void) {
  char buffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];
  IT_GetBuffer(buffer);

  if (digits_only(buffer[0]) == 0) {
    UART_OutError("\r\nERROR: the channel number can only contain digits 0 - 9\r\n");
  } else {
    if (digits_only(buffer[1]) == 0) {
      UART_OutError("\r\nERROR: the frequency can only contain digits 0 - 9\r\n");
    } else {
      if (digits_only(buffer[2]) == 0) {
        UART_OutError("\r\nERROR: the number of samples can only contain digits 0 - 9\r\n");
      } else {
        ADC_Collect(atoi(buffer[0]), atoi(buffer[1]), &adc_c_task, atoi(buffer[2]));
      }
    }
  }
}
