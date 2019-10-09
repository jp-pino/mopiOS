#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#define WD_RELOAD TIME_1MS * 5000

void Watchdog_Init(void);
void Watchdog_Reset(void);

#endif
