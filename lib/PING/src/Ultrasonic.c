#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "OS.h"
#include "Ultrasonic.h"
#include "UART.h"
#include "ST7735.h"



#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_TAMR_TACMR        0x00000004  // GPTM TimerA Capture Mode
#define TIMER_TAMR_TAMR_CAP     0x00000003  // Capture mode
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_CTL_TAEVENT_POS   0x00000000  // Positive edge
#define TIMER_IMR_CAEIM         0x00000004  // GPTM CaptureA Event Interrupt
                                            // Mask
#define TIMER_ICR_CAECINT       0x00000004  // GPTM CaptureA Event Interrupt
                                            // Clear
#define TIMER_TAILR_M           0xFFFFFFFF  // GPTM Timer A Interval Load
                                            // Register



#define ECHO1       (*((volatile uint32_t *)0x40005100))
#define ECHO1HIGH   0x40
#define ECHO2       (*((volatile uint32_t *)0x40005010))
#define ECHO2HIGH   0x04

#define PF1                     (*((volatile uint32_t *)0x40025008))

volatile unsigned long period[2];
volatile unsigned char ready[2];

#define PERIOD 100

volatile sema_t PING_FREE[2];

void Ultrasonic1(void) {
	unsigned long time, sr;
  unsigned char i;

	OS_InitSemaphore(&PING_FREE[0], 1);
  while (1) {
    time = OS_Time();
    i = 0;
    // Trigger the PING))) sensor
		// ST7735_Message(0,0,"Distance 1:", period[0]);
    ready[0] = 0;
		sr = OS_StartCritical();
    GPIO_PORTB_DATA_R |= (1 << 7);
    while (i++ < 50);
    GPIO_PORTB_DATA_R &= ~(1 << 7);
		OS_EndCritical(sr);
		// Wait for pulse measurement
    OS_bWait(&PING_FREE[0]);
    OS_Sleep(PERIOD);
  }
}

void Timer0A_Handler(void){
  static unsigned long start = 0;
  TIMER0_ICR_R = TIMER_ICR_CAECINT;// acknowledge timer0A capture match
  if (ECHO1 == ECHO1HIGH) {
    start = TIMER0_TAR_R;
  } else {
    TIMER0_TAILR_R = TIMER_TAILR_M;  // max start value
		period[0] = ((start - TIMER0_TAR_R)*3430/2*0.0000016+7.9817)/1.2985;
    ready[0] = 1;
    OS_bSignal(&PING_FREE[0]);
  }
}



void Ultrasonic2 (void) {
	unsigned long time, sr;
  unsigned char i;

	OS_InitSemaphore(&PING_FREE[1], 1);
  while (1) {
    time = OS_Time();
    i = 0;
    // Trigger the PING))) sensor
		// ST7735_Message(0,1,"Distance 2:", period[1]);
    ready[1] = 0;
		sr = OS_StartCritical();
    GPIO_PORTB_DATA_R |= (1 << 3);
    while (i++ < 50);
    GPIO_PORTB_DATA_R &= ~(1 << 3);
		OS_EndCritical(sr);
		// Wait for pulse measurement
    OS_bWait(&PING_FREE[1]);
    OS_Sleep(PERIOD);
  }
}

void Timer3A_Handler(void){
  static unsigned long start = 0;
  TIMER3_ICR_R = TIMER_ICR_CAECINT;// acknowledge timer0A capture match
  if (ECHO2 == ECHO2HIGH) {
    start = TIMER3_TAR_R;
  } else {
    TIMER3_TAILR_R = TIMER_TAILR_M;  // max start value
		period[1] = ((start - TIMER3_TAR_R)*3430/2*0.0000016+9.7317)/1.2805;
    ready[1] = 1;
    OS_bSignal(&PING_FREE[1]);
  }
}


void PING_Init(void) {
	unsigned char i = 0;
	unsigned long sr;


  sr = OS_StartCritical();
	// Input capture init 1
  SYSCTL_RCGCTIMER_R |= 0x01;// activate timer0
	SYSCTL_RCGCTIMER_R |= 0x08;// activate timer3
  SYSCTL_RCGCGPIO_R |= 0x02; // activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};// ready?

  GPIO_PORTB_DIR_R &= ~0x44;       // make PB6 in
  GPIO_PORTB_DIR_R |= 0x88;
  GPIO_PORTB_AFSEL_R |= 0x44;      // enable alt funct on PB6
  GPIO_PORTB_AFSEL_R &= ~0x88;
  GPIO_PORTB_PCTL_R |= (GPIO_PORTB_PCTL_R&0x00FF00FF) + 0x07000700; // configure PB6 as T0CCP0
  GPIO_PORTB_AMSEL_R &= ~0xCC;
  GPIO_PORTB_DEN_R |= 0xCC;        // enable digital I/O on PB6 & PB7


  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
                                   // configure for capture mode, default down-count settings
  TIMER0_TAMR_R = (TIMER_TAMR_TACMR|TIMER_TAMR_TAMR_CAP);
  TIMER0_TAPR_R |= 0xFF;
                                   // configure for rising edge event
  TIMER0_CTL_R |= TIMER_CTL_TAEVENT_BOTH;
  TIMER0_TAILR_R = TIMER_TAILR_M;  // max start value
  TIMER0_IMR_R |= TIMER_IMR_CAEIM; // enable capture match interrupt
  TIMER0_ICR_R = TIMER_ICR_CAECINT;// clear timer0A capture match flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, +edge timing, interrupts
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R = 1<<19;     // enable interrupt 19 in NVIC


	TIMER3_CTL_R &= ~TIMER_CTL_TAEN; // disable timer3A during setup
  TIMER3_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
  TIMER3_TAMR_R = (TIMER_TAMR_TACMR|TIMER_TAMR_TAMR_CAP);   // 24-bit capture
  TIMER3_CTL_R |= TIMER_CTL_TAEVENT_BOTH;// configure for both edges
  TIMER3_TAILR_R = TIMER_TAILR_M;  // max start value
  TIMER3_TAPR_R = 0xFF;            // activate prescale, creating 24-bit
  TIMER3_IMR_R |= TIMER_IMR_CAEIM; // enable capture match interrupt
  TIMER3_ICR_R = TIMER_ICR_CAECINT;// clear timer3A capture match flag
  TIMER3_CTL_R |= TIMER_CTL_TAEN;  // enable timer3A 16-b, +edge timing, interrupts
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 51, interrupt number 35
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC

  OS_EndCritical(sr);

	for (i = 0; i < 2; i++) {
		period[i] = 0;
	}

	OS_AddThread("ultra1", &Ultrasonic1, 128, 3);
	OS_AddThread("ultra2", &Ultrasonic2, 128, 3);

	ST7735_Message(1,5,"Init:", 1);
}

unsigned long PING_GetDistance(unsigned char sensor) {
  if (sensor > 1)
    return 0;
	if (ready[sensor] == 1)
    return period[sensor];
  return 0;
}
