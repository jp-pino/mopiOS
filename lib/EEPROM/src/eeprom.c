#include "eeprom.h"

#include <stdint.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"

int EEPROM_Init(void) {
	// Activate EEPROM clock
	SYSCTL_RCGCEEPROM_R |= 0x01;
	// Delay

	// Wait for power on initialization
	while(EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING != 0x00);
	// Programming error
	if (EEPROM_EESUPP_R & EEPROM_EESUPP_PRETRY != 0x00)
		return EEPROM_EESUPP_PRETRY;
	// Erase error
	if (EEPROM_EESUPP_R & EEPROM_EESUPP_ERETRY != 0x00)
		return EEPROM_EESUPP_ERETRY;
	// Reset the EEPROM
	SYSCTL_SREEPROM_R |= SYSCTL_SREEPROM_R0;
	// Delay

	// Wait for power on initialization
	while(EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING != 0x00);
	// Programming error
	if (EEPROM_EESUPP_R & EEPROM_EESUPP_PRETRY != 0x00)
		return EEPROM_EESUPP_PRETRY;
	// Erase error
	if (EEPROM_EESUPP_R & EEPROM_EESUPP_ERETRY != 0x00)
		return EEPROM_EESUPP_ERETRY;
}
