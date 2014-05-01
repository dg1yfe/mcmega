/*
 * config.c
 *
 *  Created on: 01.08.2012
 *      Author: F. Erckenbrecht
 */
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <alloca.h>

#include "config.h"
#include "macros.h"

#define CONFIG_MAGIC 0x17ef
#define CONFIG_CRC_INIT 0xff;

#define CONFIG_EEPROM_BASE 0x10
#define CONFIG_VERSION 0

static void config_initDefault(T_Config * cfgPtr);

// Radio Configuration
// put in uninitialized space -> do not initialize to be able
// to keep config in RAM during reboots without power-loss
uint8_t  config_state = CONFIG_INVALID;
uint16_t config_magic	__attribute__ ((section (".noinit")));
T_Config config __attribute__ ((section (".noinit")));


/*
 * Calculate CRC-8 (x^8 + x^2 + x^1 + 1)
 * data = data byte
 * crc = ptr to previous crc value or init value
 */

uint8_t crc8_ccitt(const uint8_t data, const uint8_t crc)
{
	//
	uint8_t i;
	uint16_t r;
	r = crc;
	r <<= 8;
	r |= data;

	for(i=0;i<8;i++){
		if(r & 0x8000){
			r ^= 0x0700;
		}
		r <<= 1;
	}
	return (uint8_t) (r >> 8);
}


static uint8_t config_checkConfigCrc( T_Config * cfgPtr){
	uint8_t crc = CONFIG_CRC_INIT;
	uint8_t * crcPtr = &(cfgPtr->crc);
	uint8_t * d = (uint8_t *) cfgPtr;

	// calculate CRC for config data in SRAM
	while(cfgPtr <= (T_Config *) crcPtr){
		crc = crc8_ccitt(*(d++), crc);
	}
	return crc;
}


static void config_calcConfigCrc( T_Config * cfgPtr){
	uint8_t crc = CONFIG_CRC_INIT;
	uint8_t * crcPtr = &(cfgPtr->crc);
	uint8_t * d = (uint8_t *) cfgPtr;

	// calculate CRC for config data in SRAM
	while((uint8_t *)cfgPtr < crcPtr){
		crc = crc8_ccitt(*(d++), crc);
	}
	cfgPtr->crc = crc;
}


// initialize basic radio configuration
uint8_t config_basicRadio()
{
// check for magic word in memory
// ( check if there could be a valid config in SRAM)
	if(config_magic == CONFIG_MAGIC){
		// calculate CRC for config data in SRAM
		// if CRC is OK, there is a correct config in SRAM, we are done
		if(!config_checkConfigCrc(&config))
			return CONFIG_SRAM;
	}
	config_magic = CONFIG_MAGIC;

	// if CRC does not fit (!= 0), try initialization from EEPROM
	eeprom_read_block(&config,(void *)CONFIG_EEPROM_BASE, sizeof(config));
	if((config.version == CONFIG_VERSION) && !config_checkConfigCrc(&config))
		return CONFIG_EEPROM;

	// configure from flash
	config_initDefault(&config);
	return CONFIG_FLASH;
}


// default configuration, use this if EEPROM config is invalid or missing
static void config_initDefault(T_Config * cfgPtr)
{
	cfgPtr->version = CONFIG_VERSION;
	cfgPtr->controlHead = CONTROL_HEAD3;
	cfgPtr->ctcssIndexRx = 1;
	cfgPtr->ctcssIndexTx = 0;
	cfgPtr->defChanSave = 0;
	cfgPtr->powerMode = DEFAULT_RF_PWR;
	cfgPtr->reserved = 0;
	cfgPtr->squelchMode = SQM_CARRIER;
	cfgPtr->f_step = FSTEP;
	cfgPtr->frequency = FDEF;
	cfgPtr->tx_shift = FTXOFF;
	cfgPtr->shift_active = 0;
	config_calcConfigCrc(cfgPtr);
}


void config_saveToEeprom(T_Config * cfgPtr)
{
	eeprom_update_block(&config,(void *)CONFIG_EEPROM_BASE, sizeof(config));
}

inline void config_beginUpdate()
{
	config_state = CONFIG_UPDATE_PENDING;
}

inline void config_validate()
{
	config_state = CONFIG_UPDATED;
	config_calcConfigCrc(&config);
	config_state = CONFIG_VALID;
}
