/*
 * config.c
 *
 *  Created on: 01.08.2012
 *      Author: F. Erckenbrecht
 */
#include <stdint.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <alloca.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "config.h"
#include "macros.h"
#include "pll_freq.h"

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

static xQueueHandle xConfigQ;
T_ConfigUpdateMessage cfgUpdate;

/*
 * Calculate CRC-8 (x^8 + x^2 + x^1 + 1)
 * data = data byte
 * crc = ptr to previous crc value or init value
 */

static uint8_t crc8_ccitt(const uint8_t data, const uint8_t crc)
{
	//
	uint8_t i;
	uint8_t r;
	r = crc;
	r ^= data;

	for(i=0;i<8;i++){
		if(r & 0x80){
			r = (r<<1)^ 0x07;
		}else{
			r <<= 1;
		}		
	}
	return r;
}


uint8_t config_checkConfigCrc( T_Config * cfgPtr){
	uint8_t crc = CONFIG_CRC_INIT;
	uint8_t * crcPtr = &(cfgPtr->crc);
	uint8_t * d = (uint8_t *) cfgPtr;

	// calculate CRC for config data in SRAM
	while(d <= crcPtr){
		crc = crc8_ccitt(*(d++), crc);
	}
	return crc;
}


void config_calcConfigCrc( T_Config * cfgPtr){
	uint8_t crc = CONFIG_CRC_INIT;
	uint8_t * crcPtr = &(cfgPtr->crc);
	uint8_t * d = (uint8_t *) cfgPtr;

	// calculate CRC for config data in SRAM
	while(d < crcPtr){
		crc = crc8_ccitt(*(d++), crc);
	}
	cfgPtr->crc = crc;
}


// initialize basic radio configuration
// TODO: Apply config
uint8_t config_basicRadio()
{
	xConfigQ = xQueueCreate( 1, sizeof( T_ConfigUpdateMessage ) );

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
	cfgPtr->ctcssIndexTx = 1;
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
	config_calcConfigCrc(&config);
	eeprom_update_block(&config,(void *)CONFIG_EEPROM_BASE, sizeof(config));
}

inline void config_validate()
{
	config_state = CONFIG_UPDATED;
	config_calcConfigCrc(&config);
	config_state = CONFIG_VALID;
}


void config_sendUpdate(){
	xQueueSendToBack( xConfigQ, &cfgUpdate, 0);
}


void config_checkForUpdate(){
	T_ConfigUpdateMessage cfgm;
	if(xQueueReceive( xConfigQ, &cfgm, 0) == pdPASS){

		if(cfgm.updateMask & CONFIG_UM_TXSHIFT){
			config.tx_shift = (int32_t) cfgm.cfgdata;
		}

		if(cfgm.updateMask & CONFIG_UM_SHIFTACTIVE ){
			config.shift_active = cfgm.cfgdata;
		}

		if(cfgm.updateMask & CONFIG_UM_FREQUENCY){
			config.frequency = cfgm.cfgdata;
		}

		if(cfgm.updateMask & CONFIG_UM_FSTEP){
			// TODO: Implement update to frequency spacing
		}

		// On frequency updates in any case OR
		// on shift updates in TX
		// set the frequency
		if( (cfgm.updateMask & CONFIG_UM_FREQUENCY) ||
			(rxtx_state &&
			(cfgm.updateMask &(CONFIG_UM_SHIFTACTIVE | CONFIG_UM_TXSHIFT)))){
			set_freq(&config.frequency);
		}

		if(cfgm.updateMask & CONFIG_UM_DEFCHANSAVE){
			config.defChanSave = cfgm.cfgdata;
		}
		if(cfgm.updateMask & CONFIG_UM_SQUELCHMODE){
			config.squelchMode = cfgm.cfgdata;
		}

		config_calcConfigCrc(&config);
		config_state = CONFIG_VALID;
	}
}

