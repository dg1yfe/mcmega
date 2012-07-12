/*
 * subs.c
 *
 *  Created on: 26.05.2012
 *      Author: F. Erckenbrecht
 *
 *****************************************************************************
    MCmega - Firmware for the Motorola MC micro radio
             to use it as an Amateur-Radio transceiver

     Copyright (C) 2012 Felix Erckenbrecht, DG1YFE

	 ( AVR port of "MC70"
      Copyright (C) 2004 - 2012  Felix Erckenbrecht, DG1YFE)

     This file is part of MCmega.

     MCmega is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     MCmega is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with MC70.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************************
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <util/crc16.h>
#include <alloca.h>

#include "regmem.h"

#include "FreeRTOS.h"
#include "task.h"

#include "io.h"
#include "display.h"

#include "macros.h"
#include "regmem.h"
#include "firmware.h"

void pwr_sw_chk(char cSaveSettings)
{
	// check state of power switch (SWB+)
	// Port bit is 0 if it is enabled
	if( SWBPORT & SWBDISABLED )
	{
		//TODO: Add 9,6V Power Fail check to EZA9 HW
		if (cSaveSettings)
		{
			//TODO: storeCurrent();
		}

		// disable RX Audio
		SetShiftReg((uint8_t)~SR_RXAUDIOEN, 0);

		// shut-down Radio
		// TODO: Check if further bits need to be set/reset e.g. Audio PA
		SetShiftReg(~SR_9V6, 0);
		while(1);

	}

	return;
}


void watchdog_toggle()
{
	PORTD ^= (1<<PORTD6);
}


void watchdog_toggle_ms()
{
	DDRD = (DDRD & ~(1<<PORTD6)) | ((tick_ms & 2)<< 5);
	PORTD &= ~(1<<PORTD6);
}


void wd_reset()
{
	if(!bus_busy)
	{
		wd_toggle_ms();
	}
}


/*
 * Return RX/TX state requested by PTT
 * Input gets debounced
 */

char ptt_get_status()
{
	char state;

	state = (PTTPORT & PTTBIT)? 1:0;

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


void receive()
{
	led_set(YEL_LED, LED_OFF);

	PORT_DPTT |= (1 << DPTT);	// Disable PA drive
	SetShiftReg(0, ~SR_MICEN);	// Disable Mic Amp

	vTaskDelay(TX_TO_RX_TIME);	// Wait TX to RX Time

	vco_switch(0);				// enabe RX VCO

	set_rx_freq(&frequency);	// set VCO to RX frequency

	rxtx_state = 0;

	SetShiftReg(SR_AUDIOPA, 0xff);

	// check squelch state immediately
	sql_flag ^= 0xff;
	// NOW
	sql_timer = 0;

}


void transmit()
{
	led_set(YEL_LED, LED_ON);
	vco_switch(1);
	set_tx_freq(&frequency);
	SetShiftReg(SR_RXVCOSEL, ~SR_RXAUDIOEN);
	vTaskDelay(RX_TO_TX_TIME);	// Wait RX to TX Time

	if(pwr_mode)
	{
		SetShiftReg(SR_MICEN,SR_TXPWRLO);
	}
	else
	{
		SetShiftReg(SR_MICEN | SR_TXPWRLO,0xff);
	}
	rxtx_state = 1;
}


void squelch()
{
	// return if timer is non-zeor or we are in TX
	if(!(sql_timer || rxtx_state))
	{
		sql_timer = SQL_INTERVAL;
		if(sql_mode)
		{
			char state = PIN_SQL & BIT_SQL;
			if(state != sql_flag)
			{
				sql_flag = state;
				if(state)
				{
					// Enable Audio, Pull Ext Alarm low
					SetShiftReg(SR_RXAUDIOEN,SR_EXTALARM);
				}
				else
				{	// disable Audio, set Ext Alarm high
					SetShiftReg(SR_EXTALARM,~SR_RXAUDIOEN,);
				}
			}
		}
	}
}

/*
 * Select RX or TX VCO
 * vco = 1 - TX VCO
 */
inline void vco_switch(char vco)
{
	if(vco)
	{
		SetShiftReg(0,~SR_RXVCOSEL);
	}
	else
	{
		SetShiftReg(SR_RXVCOSEL,0xff);
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
		crc = _crc16_update(crc, *data++);
	}
	return crc;
}


void tone_start()
{
	// TODO : Tonausgabe
}


void tone_stop()
{
	// TODO: Beendet Tonausgabe
}

/*
 *  Read frequency and settings from EEPROM
 *  slot = slot number
 *  dest = address of destination
 */
char read_eep_ch(uint16_t slot, long * freq)
{
	void * buf;
	char err;
	long fbuf;

	buf = alloca((size_t)10);

	if(slot > 24)
		return -1;

	slot = slot * 10;
	slot += 0x100;
	err = eep_seq_read(10, slot, buf);

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
	fbuf = *((uint16_t) buf);
	fbuf &= 0x1ff;	// use 9 bit, max 12,775 MHz Shift
	fbuf *= 25000;	// multiply by 25 kHz
	ui_txshift = fbuf;

	return 0;
}


char store_eep_ch(uint16_t slot)
{
	void * buf;
	long fbuf;
	ldiv_t fdiv;

	buf = alloca((size_t)10);

	if(slot > 24)
		return -1;

	fbuf = frequency;
	fbuf -= FBASE;				// subtract Base Frequency

	fdiv = ldiv(fbuf, 1250);	// divide by 1250 Hz
	fbuf = fdiv.quot;
	fbuf <<= 3;
	*((long *) buf) = fbuf;		// store 13 Bit in Buffer

	fbuf = offset;				// get stored TX shift (if used or not)
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
	return eep_write_seq(10, slot, buf);	// write data to EEPROM

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

	fbuf = frequency;
	fbuf -= FBASE;				// subtract Base Frequency

	fdiv = ldiv(fbuf, 1250);	// divide by 1250 Hz
	fbuf = fdiv.quot;
	((uint16_t) fbuf) <<= 3;
	*((uint16_t *) buf) = (uint16_t) fbuf;	// store 13 Bit in Buffer

	buf++;

	fbuf = tx_shift;			// get active TX shift
	if(fbuf < 0)
	{
		fbuf *= -1;
		*((char *) buf) = 2;	// Byte 2, Bit 1 = sign of TX shift
	}

	if(offset)					// check if tx shift is used
	{
		*((char *) buf) |= 4;	// Byte 2, Bit 2 = state of TX shift
	}

	fdiv = ldiv(fbuf, 25000);	// divide by 25 kHz
	fbuf = fdiv.quot;
	((uint16_t) fbuf) &= 0x1ff;	// reduce to 9 active bits

	*((uint16_t *) buf) |= (uint16_t) fbuf;	// and store in temporal buffer
	buf+=2;

	*((char *)buf) = 0;			// do not store any designation yet

	buf-=3;						// set buf ptr to pos 0;

	return eep_write_seq(3, 0x1fa , buf);	// write data to EEPROM

}



char read_current(const long * freq,const long * txshift,const long * offset)
{
	void * buf;
	char err;
	long fbuf;

	buf = alloca((size_t)3);
	if(buf == NULL)
		return -1;

	err = eep_seq_read(3, 0x1fa, buf);

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
	fbuf = *((uint16_t) buf);
	fbuf &= 0x1ff;	// use 9 bit, max 12,775 MHz Shift
	fbuf *= 25000;	// multiply by 25 kHz

	if(*((char*)buf) & 2)
	{
		fbuf *= -1;
	}

	*tx_shift = fbuf;

	if(*((char*)buf) & 4)
	{
		*offset = fbuf;
	}
	else
	{
		*offset = 0;
	}

	return 0;
}




