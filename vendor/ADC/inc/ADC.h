/**
 * @file ADC.h
 * @author Cole Morgan, Juan Pablo Pino
 * @date February 4, 2019
 *
 * Provide functions that open one ADC channel to be sampled
 * by calls to ADC_In or ADC_Collect.
 *
 */

#ifndef __ADC_H__
#define __ADC_H__

#define ADC_COLLECTOR_READY 1
#define ADC_OS_ERROR 2
#define ADC_NO_THREADS_AVAILABLE 3
#define ADC_NO_COLLECTOR_READY 4

#define MAX_ADC_COLLECT 1000

void ADC_InitIT(void);

/** Opens specified ADC channel
 * @param channelNum Number of ADC channel to be opened
 * @return 0 for success or 1 for error
 */
int ADC_Init(uint32_t channelNum);

/** Samples the ADC on the open channel
 * @return 12 bit unsigned ADC result
 */
uint16_t ADC_In(void);

/** Sets up periodic ADC sampling on specified channel
 * @param channelNum Number of ADC channel to collect data on
 * @param fs Desired sample rate of ADC in Hz
 * @param buffer Pointer to buffer where ADC data will be stored
 * @param numberOfSamples Number of ADC samples to be collected into buffer
 * @return 0 for success or 1 for error
 */

int ADC_Collect(uint32_t channelNum, uint32_t fs, void(*)(unsigned long), int32_t);


uint32_t ADC2millimeter(uint32_t adcSample);

#endif //__ADC_H__
