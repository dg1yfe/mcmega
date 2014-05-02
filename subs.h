/*
 * subs.h
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
 */

#ifndef SUBS_H_
#define SUBS_H_
#include <stdint.h>

#define PWR_DONTSAVECONFIG 0
#define PWR_SAVECONFIG 1
void pwr_sw_chk( char cSaveSettings);

void receive(void);
void transmit(void);

#define PTT_EVENT_BIT 0x80
char ptt_get_status(void);
//void vco_switch(char vco);
unsigned int crc16(unsigned int bytecount, void * data, unsigned int init);

char read_eeep_ch(uint16_t slot, long * freq);
char read_ieep_ch(uint16_t slot, long * freq);
char store_eeep_ch(uint16_t slot);
char store_ieep_ch(uint16_t slot);
char store_current(void);
uint8_t read_current(T_Config * cfgPtr);

void squelch(void);
void squelch_print(void);

void wd_reset(void);

void audio_pa(uint8_t enable, uint8_t withrxaudio);

void rfpwr_set(uint8_t enable_hi_power);
void rfpwr_print(void);

#endif /* SUBS_H_ */
