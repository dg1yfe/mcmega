/*
 * config.h
 *
 *  Created on: 01.08.2012
 *      Author: F. Erckenbrecht
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#include <stdint.h>
#include "FreeRTOS.h"

#define CONFIG_POWER_HIGH 0
#define CONFIG_POWER_LOW  1

enum { CONFIG_OK, CONFIG_SRAM, CONFIG_EEPROM, CONFIG_FLASH };

#define CONFIG_VALID 0x0f
#define CONFIG_INVALID 0xf0
#define CONFIG_UPDATE_PENDING 0x5a
#define CONFIG_UPDATED 0xa5

#define CONFIG_SAVE_MANUAL	0
#define CONFIG_SAVE_AUTO	1
#define CONFIG_SAVE_NOW		2

typedef struct {
	uint32_t frequency;
	int32_t	 tx_shift;
	unsigned shift_active:1;		// Shift active Bit
	unsigned ctcssIndexRx:6;
	unsigned ctcssIndexTx:6;
	unsigned version:3;
	char	 name[6];
}T_CHANNEL;


//*******************
// Type Definitions
//*******************
typedef struct {
	unsigned	version:2;				// Config struct version
	unsigned	powerMode:1;			// Lo / Hi Power
	unsigned	squelchMode:2;			// Squelch mode
	unsigned	configAutosave:2;			// save current frequency as default channel?
	unsigned	shift_active:1;
	unsigned	controlHead:3;			// control head
	unsigned	ctcssIndexRx:6;			// CTCSS
	unsigned	ctcssIndexTx:6;
	unsigned	reserved:1;				// write 0
	uint32_t	frequency;
	int32_t		tx_shift;
	uint16_t	f_step;
	uint8_t		crc;					// CRC-8 (x^8+x^2+x1+1)
} T_Config;

#define CONFIG_UM_FREQUENCY 1
#define CONFIG_UM_TXSHIFT	2
#define CONFIG_UM_FSTEP		4
#define CONFIG_UM_CONFIGAUTOSAVE 8
#define CONFIG_UM_SQUELCHMODE 16
#define CONFIG_UM_POWERMODE 32
#define CONFIG_UM_SHIFTACTIVE 64
#define CONFIG_UM_CTCSS 128
#define CONFIG_UM_SAVE_TO_EEPROM UINT32_C(1<<15)

typedef struct {
	uint32_t cfgdata;
	uint16_t updateMask;
}T_ConfigUpdateMessage;

extern T_Config config;			// Radio Configuration
extern T_ConfigUpdateMessage cfgUpdate;
extern uint8_t  config_state;

uint8_t config_basicRadio(void);// configure Radio from SRAM, EEPROM or Flash
void config_saveToEeprom(T_Config * cfgPtr);
void config_validate(void);
void config_sendUpdate(void);
void config_checkForUpdate(portTickType ticksToWait);

#endif /* CONFIG_H_ */
