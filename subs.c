/*
 * subs.c
 *
 *  Created on: 26.05.2012
 *****************************************************************************
 *	MCmega - Firmware for the Motorola MC micro radio
 *           to use it as an Amateur-Radio transceiver
 *
 * Copyright (C) 2013 Felix Erckenbrecht, DG1YFE
 *
 * ( AVR port of "MC70"
 *   Copyright (C) 2004 - 2013  Felix Erckenbrecht, DG1YFE)
 *
 * This file is part of MCmega.
 *
 * MCmega is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MCmega is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MCmega.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include <alloca.h>
#include <util/delay.h>
#include <avr/sleep.h>

#include "regmem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "display.h"
#include "firmware.h"
#include "pll_freq.h"
#include "eeprom.h"
#include "subs.h"
#include "audio.h"
#include "config.h"


uint8_t sql_timer;
uint8_t sql_flag = 0;
uint8_t sql_pin_flag;


void pwr_sw_chk(char cSaveSettings)
{
	// check state of power switch (SWB+)
	// Port bit is 0 if it is enabled
	// and 1 if disabled/switched off
	if( PIN_SWB & BIT_SWB )
	{
		//TODO: Add 9,6V Power Fail check to EZA9 HW
		if (cSaveSettings && (config.configAutosave & CONFIG_SAVE_AUTO))
		{
			//TODO: storeCurrent();
			config_validate();
			config_saveToEeprom(&config);
		}
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);

		xSemaphoreTakeRecursive(SerialBusMutex, portMAX_DELAY);
		taskENTER_CRITICAL();
		eep_enable(0);
		// disable RX Audio
		audio_pa(0,1);

		_delay_ms(10);
		// shut-down Radio
		// TODO: Check if further bits need to be set/reset e.g. Audio PA
		SetShiftReg(0,~SR_9V6);
		sleep_enable();
		// Halt CPU ... wake-up occurs by pulling #RESET low
		sleep_cpu();
		while(1);
	}

	return;
}



void watchdog_toggle()
{
	PORT_SBUS_DATA ^= BIT_SBUS_DATA;
}


void watchdog_toggle_ms()
{
	PORT_SBUS_DATA &= ~(BIT_SBUS_DATA);
	DDR_SBUS_DATA = (DDR_SBUS_DATA & ~(BIT_SBUS_DATA)) | ((tick_ms & 2)? BIT_SBUS_DATA : 0);
}


void wd_reset()
{
#ifndef DISABLE_WATCHDOG_RESET
	if(xSemaphoreTakeRecursive(SerialBusMutex,0))
	{
		watchdog_toggle_ms();
		xSemaphoreGiveRecursive(SerialBusMutex);
	}
#else
#warning HW WATCHDOG WILL NOT BE TRIGGERED! (DISABLE_WATCHDOG_RESET)
#endif	
}


/*
 * Return RX/TX state requested by PTT
 * Input gets debounced
 */

char ptt_get_status()
{
	char state;

	state = (PIN_PTT & BIT_PTT)? 1:0;

	state |= (ui_ptt_req & 1);

	if (state == rxtx_state)
	{
		ptt_debounce = 0;
	}
	else
	{
		if(++ptt_debounce >= PTT_DEBOUNCE_VAL)
		{
			ptt_debounce--;
			return (state | 0x80);
		}
		else
		{
			state = rxtx_state;
		}
	}
	return state;
}

/*
 * Select RX or TX VCO
 * vco = 1 - TX VCO
 */
static inline void vco_switch(char vco)
{
	if(vco)
	{
		SetShiftReg(SR_TXVCOSEL,0xff);
	}
	else
	{
		SetShiftReg(0,(uint8_t) ~SR_TXVCOSEL);
	}
}


void receive()
{
	led_set(YEL_LED, LED_OFF);

	PORT_DPTT |=  BIT_DPTT;			// Disable PA drive
	SetShiftReg(0, ~SR_MICEN);		// Disable Mic Amp

	vTaskDelay(TX_TO_RX_TIME);		// Wait TX to RX Time

	vco_switch(0);					// enable RX VCO

	set_rx_freq(&config.frequency);		// set VCO to RX frequency

	rxtx_state = 0;

	//SetShiftReg(SR_RXAUDIOEN, 0xff);

	// check squelch state immediately
	sql_flag ^= 0xff;
	// NOW
	sql_timer = 0;

}


void transmit()
{
	led_set(YEL_LED, LED_ON);
	vco_switch(1);
	set_tx_freq(&config.frequency);
	SetShiftReg(0, (uint8_t)~(SR_RXAUDIOEN));
	vTaskDelay(RX_TO_TX_TIME);	// Wait RX to TX Time

	if(config.powerMode)
	{
		SetShiftReg(SR_MICEN, ~SR_TXPWRLO);
	}
	else
	{
		SetShiftReg(SR_MICEN | SR_TXPWRLO,0xff);
	}

	PORT_DPTT &=  ~BIT_DPTT;	// Enable PA drive

	rxtx_state = 1;
}


void squelch()
{
	// return if timer is non-zero or we are in TX
	if(!sql_timer && !rxtx_state)
	{
		char state;
		char pin_state;
		
		sql_timer = SQL_INTERVAL;

		// squelch open if carrier detected (bit_sql = 1)
		// or squelch deactivated (sql_mode == SQM_OFF)
		// or carrier detected & tone detected
		pin_state = (PIN_SQL & BIT_SQL);

		// if a signal appeared or disappeared
		if(pin_state ^ sql_pin_flag)
		{
			tone_decoder_reset();	// reset tone decoder to start NOW
		}
		sql_pin_flag = pin_state;

		state = pin_state || (config.squelchMode == SQM_OFF);
		if((config.squelchMode != SQM_OFF) && g_coeff)
		{
			state = (PIN_SQL & BIT_SQL);
			// disregard tone detection state until detector output was updated
			if(tone_detect_updated)
				 state = state && tone_detect;
		}		
		

		if(state != sql_flag)
		{
			sql_flag = state;
			if(state)
			{
				// Enable Audio, Pull Ext Alarm low
				SetShiftReg(SR_RXAUDIOEN, (uint8_t)~SR_EXTALARM);
				// enable yellow LED
				led_set(YEL_LED, LED_ON);
			}
			else
			{	// disable Audio, set Ext Alarm high
				SetShiftReg(SR_EXTALARM, (uint8_t)~SR_RXAUDIOEN);
				// disable yellow LED
				led_set(YEL_LED, LED_OFF);
			}
		}
	}
}


/*
 * Calculate CRC-16
 */
unsigned int crc16(unsigned int bytecount, void * data, unsigned int init)
{
	uint16_t crc;

	crc = init;
	while(bytecount--)
	{
		crc = _crc16_update(crc, *((char *)data++));
	}
	return crc;
}


/*
 *  Read frequency and settings from internal EEPROM
 *  slot = slot number
 *  dest = address of destination
 */
char read_iep_ch(uint16_t slot, long * freq)
{
	void * buf;
	long fbuf;

	buf = alloca((size_t)10);

	if(slot > 24)
		return -1;

	slot = slot * 10;
	slot += 0x100;
	
	eeprom_read_block(buf, (void *) slot, 10);

	// get channel
	fbuf = *((uint16_t *) buf);
	fbuf >>=3;		// 13 significant Bits
	fbuf *= 1250;	// multiply by 1250 to obtain frequency
	fbuf += FBASE; // add Base frequency
	*freq = fbuf;

	buf++;
	// get TX shift, stored in 12.5 kHz Steps
	fbuf = *((uint16_t *) buf);
	fbuf &= 0x1ff;	// use 9 bit, max 12,775 MHz Shift
	fbuf *= 25000;	// multiply by 25 kHz
	config.tx_shift = fbuf;

	return 0;
}



/*
 *  Read frequency and settings from external EEPROM
 *  slot = slot number
 *  dest = address of destination
 */
char read_eeep_ch(uint16_t slot, long * freq)
{
	void * buf;
	char err;
	long fbuf;

	buf = alloca((size_t)10);

	if(slot > 24)
		return -1;

	slot = slot * 10;
	slot += 0x100;
	err = eep_seq_read(10, slot, buf, NULL);

	if(err)
		return(err);

	// get channel
	fbuf = *((uint16_t *) buf);
	fbuf >>=3;		// 13 significant Bits
	fbuf *= 1250;	// multiply by 1250 to obtain frequency
	fbuf += FBASE; // add Base frequency
	*freq = fbuf;

	buf++;
	// get TX shift, stored in 12.5 kHz Steps
	fbuf = *((uint16_t *) buf);
	fbuf &= 0x1ff;	// use 9 bit, max 12,775 MHz Shift
	fbuf *= 25000;	// multiply by 25 kHz
	config.tx_shift = fbuf;

	return 0;
}



char store_ieep_ch(uint16_t slot)
{
	void * buf;
	long fbuf;
	ldiv_t fdiv;

	buf = alloca((size_t)10);

	if(slot > 24)
		return -1;

	fbuf = config.frequency;
	fbuf -= FBASE;				// subtract Base Frequency

	fdiv = ldiv(fbuf, 1250);	// divide by 1250 Hz
	fbuf = fdiv.quot;
	fbuf <<= 3;
	*((long *) buf) = fbuf;		// store 13 Bit in Buffer

	fbuf = config.tx_shift;		// get stored TX shift (if used or not)
	fdiv = ldiv(fbuf, 25000);	// divide by 25 kHz
	fbuf = fdiv.quot;
	fbuf &= 0x1ff;				// reduce to 9 active bits

	buf++;						// combine with frequency data
	*((uint16_t *) buf) |= (uint16_t) fbuf;	// and store in temporal buffer
	buf+=2;

	*((char *)buf) = 0;			// do not store any designation yet

	buf -=3;					// set pointer to original address


	slot *= 10;
	slot += 0x100;				// calculate EEPROM address from slot no.
	eeprom_update_block(buf,(void *) slot, 10);	// write data to EEPROM
	return 0;
}



char store_eeep_ch(uint16_t slot)
{
	void * buf;
	long fbuf;
	ldiv_t fdiv;

	buf = alloca((size_t)10);

	if(slot > 24)
		return -1;

	fbuf = config.frequency;
	fbuf -= FBASE;				// subtract Base Frequency

	fdiv = ldiv(fbuf, 1250);	// divide by 1250 Hz
	fbuf = fdiv.quot;
	fbuf <<= 3;
	*((long *) buf) = fbuf;		// store 13 Bit in Buffer

	fbuf = config.tx_shift;		// get stored TX shift (if used or not)
	fdiv = ldiv(fbuf, 25000);	// divide by 25 kHz
	fbuf = fdiv.quot;
	fbuf &= 0x1ff;				// reduce to 9 active bits

	buf++;						// combine with frequency data
	*((uint16_t *) buf) |= (uint16_t) fbuf;	// and store in temporal buffer
	buf+=2;

	*((char *)buf) = 0;			// do not store any designation yet

	buf -=3;					// set pointer to original address


	slot *= 10;
	slot += 0x100;				// calculate EEPROM address from slot no.
	return eep_write_seq(slot, 10, buf);	// write data to EEPROM

}



char store_current(void)
{
	void * buf;
	long fbuf;
	ldiv_t fdiv;
/*
 * 0 - freq hi        (Bit 7-0)
 * 1 - freq lo        (Bit 7-3)
 *     txshift active (Bit 2)
 *     txshift sign   (Bit 1)
 *     txshift hi     (Bit 0)
 * 2 - txshift lo     (Bit 7-0)
 *
 */
	buf = alloca((size_t)3);

	if(buf == NULL)
		return -1;

	fbuf = config.frequency;
	fbuf -= FBASE;				// subtract Base Frequency

	fdiv = ldiv(fbuf, 1250);	// divide by 1250 Hz
	fbuf = fdiv.quot;
	fbuf = ((uint16_t) fbuf) << 3;
	*((uint16_t *) buf) = (uint16_t) fbuf;	// store 13 Bit in Buffer

	buf++;

	fbuf=config.tx_shift;
	if(fbuf < 0)
	{
		fbuf *= -1;
		*((char *) buf) = 2;	// Byte 2, Bit 1 = sign of TX shift
	}

	if(!config.shift_active)	// check if tx shift is used
	{
		*((char *) buf) |= 4;	// Byte 2, Bit 2 = state of TX shift
	}

	fdiv = ldiv(fbuf, 25000);	// divide by 25 kHz
	fbuf = fdiv.quot;

	fbuf = ((uint16_t) fbuf) & 0x1ff;	// reduce to 9 active bits

	*((uint16_t *) buf) |= (uint16_t) fbuf;	// and store in temporal buffer
	buf+=2;

	*((char *)buf) = 0;			// do not store any designation yet

	buf-=3;						// set buf ptr to pos 0;

 	eeprom_update_block(buf,(void *) 0x1fa, 3);	// write data to EEPROM
	return 0;

}



uint8_t read_current(T_Config * cfgPtr) // unsigned long * freq,int32_t * txshift, uint8_t * shift_active)
{
	void * buf;
	long fbuf;

	buf = alloca((size_t)3);
	if(buf == NULL)
		return -1;

	eeprom_read_block(buf,(void *) 0x1fa, 3);

	// get channel
	fbuf = *((uint16_t *) buf);
	fbuf >>=3;		// 13 significant Bits
	fbuf *= 1250;	// multiply by 1250 to obtain frequency
	fbuf += FBASE; // add Base frequency
	cfgPtr->frequency = fbuf;

	buf++;
	// get TX shift, stored in 12.5 kHz Steps
	fbuf = *((uint16_t *) buf);
	fbuf &= 0x1ff;	// use 9 bit, max 12,775 MHz Shift
	fbuf *= 25000;	// multiply by 25 kHz

	if(*((char*)buf) & 2)
	{
		fbuf *= -1;
	}

	cfgPtr->tx_shift = fbuf;
	
	// check if offset should be activated
	if(*((char*)buf) & 4)
	{
		cfgPtr->shift_active = 1;
	}
	else
	{
		cfgPtr->shift_active = 0;
	}

	return 0;
}


void audio_pa(uint8_t enable, uint8_t withrxaudio)
{
#define pwmmax 50
#define pwmmaxoff 400
	uint16_t i;
	uint16_t j;
	uint8_t rxaudio;

	// save rxaudio state
	rxaudio = SR_data_buf & SR_RXAUDIOEN;
	
	// enable or disable rxaudio for switching
	if (withrxaudio)
		SetShiftReg(SR_RXAUDIOEN, 0xff);
	else		
		SetShiftReg(0, (uint8_t)~SR_RXAUDIOEN);


	if(enable)
	{
		for(i=0;i<pwmmax;i+=2)
		{
			SetShiftReg(0, (uint8_t)~SR_AUDIOPA);
			for(j=i;j<pwmmax;j++){
				_delay_loop_1(2);
			}
			SetShiftReg(SR_AUDIOPA, 0xff);
		}
		_delay_loop_1(255);
	}
	else
	{
		for(i=1;i<pwmmaxoff;i+=10)
		{
			SetShiftReg(SR_AUDIOPA, 0xff);
//			for(j=i;j<pwmmaxoff;j++){
//				_delay_loop_1(2);
//			}
			SetShiftReg(0, ~SR_AUDIOPA);
			for(j=pwmmaxoff-i;j<pwmmaxoff;j++){
				_delay_loop_1(3);
			}
		}
	
	}

	// restore rx audio state
	SetShiftReg(rxaudio, (uint8_t)~SR_RXAUDIOEN);
}


void rfpwr_apply()
{
	if(config.powerMode == CONFIG_POWER_LOW)
	{
		SetShiftReg(SR_TXPWRLO, 0xff);
	}
	else
	{
		SetShiftReg(0, (uint8_t) ~SR_TXPWRLO);
	}
}


void rfpwr_set(uint8_t enable_hi_power)
{
	if(enable_hi_power)
	{
		cfgUpdate.cfgdata = 0;
	}
	else
	{
		cfgUpdate.cfgdata = 1;
	}
	cfgUpdate.updateMask = CONFIG_UM_POWERMODE;
	config_sendUpdate();
}


void rfpwr_print()
{
	if(config.powerMode)
		arrow_set(3, 0);
	else
		arrow_set(3, 1);
}


void squelch_print()
{
	if(config.squelchMode == SQM_CARRIER)
		arrow_set(2, 1);
	else
	if(config.squelchMode == SQM_OFF)
		arrow_set(2, 0);
}


