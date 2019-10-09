#include "tm4c123gh6pm.h"
#include "watchdog.h"
#include "OS.h"
#include <stdlib.h>


sema_t watchdog_s[8];

void reset_0(void) {
	while(1) {
		OS_bSignal(&watchdog_s[0]);
		OS_Sleep(300);
	}
}

void reset_1(void) {
	while(1) {
		OS_bSignal(&watchdog_s[1]);
		OS_Sleep(300);
	}
}

void reset_2(void) {
	while(1) {
		OS_bSignal(&watchdog_s[2]);
		OS_Sleep(300);
	}
}

void reset_3(void) {
	while(1) {
		OS_bSignal(&watchdog_s[3]);
		OS_Sleep(300);
	}
}
void reset_4(void) {
	while(1) {
		OS_bSignal(&watchdog_s[4]);
		OS_Sleep(300);
	}
}

void reset_5(void) {
	while(1) {
		OS_bSignal(&watchdog_s[5]);
		OS_Sleep(300);
	}
}
void reset_6(void) {
	while(1) {
		OS_bSignal(&watchdog_s[6]);
		OS_Sleep(300);
	}
}

void reset_7(void) {
	while(1) {
		OS_bSignal(&watchdog_s[7]);
		OS_Sleep(300);
	}
}

void reset_thread(void) {
	while(1) {
		OS_Sleep(300);
		Watchdog_Reset();
	}
}

char name[100];
void Watchdog_Init(void) {
	SYSCTL_RCGCWD_R |= 0x01; // Enable UART5
  while((SYSCTL_PRWD_R&0x01)==0);
	WATCHDOG0_LOAD_R = WD_RELOAD;
	WATCHDOG0_CTL_R |= WDT_CTL_RESEN;
	WATCHDOG0_CTL_R |= WDT_CTL_INTEN;


	for (int i = 0; i < 8; i++) {
		strcpy(name, "wd_");
		name[3] = '0' + i;
		name[4] = '\0';
		OS_InitSemaphore(name, &watchdog_s[i], 0);
	}

	OS_AddThread("watchdog", &reset_thread, 128, 1);
	OS_AddThread("wd_t0", &reset_0, 96, 0);
	OS_AddThread("wd_t1", &reset_1, 96, 1);
	OS_AddThread("wd_t2", &reset_2, 96, 2);
	OS_AddThread("wd_t3", &reset_3, 96, 3);
	OS_AddThread("wd_t4", &reset_4, 96, 4);
	OS_AddThread("wd_t5", &reset_5, 96, 5);
	OS_AddThread("wd_t6", &reset_6, 96, 6);
	OS_AddThread("wd_t7", &reset_7, 96, 7);
}

void Watchdog_Reset(void) {
	WATCHDOG0_LOAD_R = WD_RELOAD;
}
