#include "tm4c123gh6pm.h"
#include "watchdog.h"
#include "OS.h"

void reset_thread(void) {
	while(1) {
		Watchdog_Reset();
		OS_Sleep(300);
	}
}

void Watchdog_Init(void) {
	SYSCTL_RCGCWD_R |= 0x01; // Enable UART5
  while((SYSCTL_PRWD_R&0x01)==0);
	WATCHDOG0_LOAD_R = WD_RELOAD;
	WATCHDOG0_CTL_R |= WDT_CTL_RESEN;
	WATCHDOG0_CTL_R |=WDT_CTL_INTEN;

	OS_AddThread("watchdog", &reset_thread, 128, 7);
}

void Watchdog_Reset(void) {
	WATCHDOG0_LOAD_R = WD_RELOAD;
}
