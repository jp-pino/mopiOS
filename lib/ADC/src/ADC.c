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


extern char paramBuffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];

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
  cmd = IT_AddCommand("adc", 0, "", &adc, "", 128, 3);
  IT_AddFlag(cmd, 'o', 1, "[chan]" , &adc_o, "open a channel", 128, 3);
  IT_AddFlag(cmd, 'i', 0, "" , &adc_i, "read value from open channel", 128, 3);
  IT_AddFlag(cmd, 'c', 3, "[chan] [freq] [samps]" , &adc_c, "read value periodically", 128, 3);
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


// Interpreter
void adc(void) {
  IT_Init();

  UART_OutError("\r\nERROR: COMMAND \"adc\" EXPECTS FLAGS");
  IT_Kill();
}

void adc_o(void) {
  IT_Init();

  IT_GetBuffer(paramBuffer);

  if (digits_only(paramBuffer[0]) == 0) {
    UART_OutError("\r\nERROR: the channel number can only contain digits 0 - 9\r\n");
  } else {
    ADC_Init(atoi(paramBuffer[0]));
  }
  IT_Kill();
}

void adc_i(void) {
  int16_t adc;

  IT_Init();

  adc = ADC_In() - 2048;

  UART_OutString("\r\n    ");
	if (adc < 0) {
		UART_OutString("-");
	  UART_OutUDec(-adc);
	} else {
	  UART_OutUDec(adc);
	}
  IT_Kill();
}

void adc_c_task(void) {
  unsigned long start;
  int sleep, samples, i = 1;

  IT_Init();

  sleep = OS_MailBox_Recv();
  samples = OS_MailBox_Recv();
  while (i < samples) {
    start = OS_Time();
    ST7735_Message(ST7735_DISPLAY_TOP, 1, "Run:", i++);
    ST7735_Message(ST7735_DISPLAY_TOP, 2, "Val:", ADC_In());
    OS_Sleep(sleep - (OS_Time() - start));
  }
  OS_Kill();
}

void adc_c(void) {
  IT_Init();

  IT_GetBuffer(paramBuffer);

  if (digits_only(paramBuffer[0]) == 0) {
    UART_OutError("\r\nERROR: the channel number can only contain digits 0 - 9\r\n");
  } else {
    if (digits_only(paramBuffer[1]) == 0) {
      UART_OutError("\r\nERROR: the frequency can only contain digits 0 - 9\r\n");
    } else {
      if (digits_only(paramBuffer[2]) == 0) {
        UART_OutError("\r\nERROR: the number of samples can only contain digits 0 - 9\r\n");
      } else {
        ADC_Init(atoi(paramBuffer[0]));
        OS_MailBox_Init();
        OS_AddThread("adc_c", &adc_c_task, 128, 2);
        OS_MailBox_Send(1000 / atoi(paramBuffer[1]));
        OS_MailBox_Send(atoi(paramBuffer[2]));
      }
    }
  }
  IT_Kill();
}
