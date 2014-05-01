/*
 * regmem.c
 *
 *  Created on: 27.05.2012
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
#include <stdint.h>
#include "regmem.h"

//*******************
// R E G I S T E R S
//*******************

char SR_data_buf;
void * oci_vec;
char tasksw;
char last_tasksw;
char tasksw_en;
int  start_task;
char pcc_cdiff_flag; 	// Flag

char led_buf;			// Bit 0 (1)  - gelb
						// Bit 1 (2)  - gelb blink
						// Bit 2 (4)  - gr�n
						// Bit 3 (8)  - gr�n blink
						// Bit 4 (16) - rot
						// Bit 5 (32) - rot blink
						// Bit 6 (64) - unused
						// Bit 7 (128)- change flag
char led_dbuf;

uint16_t arrow_buf;			// Bit  0 - Arrow 0
							// Bit  1 - Arrow 1
							// ...
							// Bit  8 - Arrow 0 blink
							// ...
							// Bit 14 - Arrow 6 blink

char dbuf[8];				// Main Display Buffer
uint8_t cpos;					// Cursorposition

char dbuf2[8];				// Display Buffer2 + Byte f�r CPOS
uint8_t cpos2;

char f_in_buf[9];			// 9 byte buffer

int  oci_int_ctr;

int f_step;					// Schrittweite in Hz

int tick_ms;				// 1ms Increment
int s_tick_ms;				// Software timer
int tick_hms;				// 100ms Increment
int8_t next_hms;
char gp_timer;				// General Purpose Timer, 1ms Decrement
char ui_timer;
uint8_t lcd_timer;				// 1ms

long offset;				// Für RX/TX verwendete Shift (0/+TXS/-TXS)
long txshift;
long channel;				// aktuell in der PLL gesetzter Kanal
uint32_t ui_frequency;			// Über UI eingegebene Frequenz wird hier gespeichert
int32_t ui_txshift;			// Über UI eingegebene Frequenz wird hier gespeichert

char rxtx_state;			// 0=RX
char ui_ptt_req;			//

char ptt_debounce;

char m_state;
int  m_timer = 0;			// 100ms
char m_timer_en;

char sql_timer;
char sql_flag;

uint8_t sql_pin_flag;

char mem_bank;

char pll_locked_flag;		// Bit 0 - PLL not locked
char pll_timer;

char tone_timer;
char tone_index;
char oci_ctr;

int ts_count;

typedef struct {
	unsigned rxfreq : 13;	// 1,25 kHz Steps, 10240 kHz max
	signed   offset : 10;	// 25 kHz Steps for Offset, signed
	unsigned ofs_active: 1;	// Offset activated?
	unsigned ctcss_index: 6;	// CTCSS frequency (index)
	unsigned ctcss_squelch: 1;	// CTCSS squelch active
	unsigned reserved:1;
	char	 name[6];
	} T_MemChannel;


T_Config config;


//*****************************
// I O   R I N G B U F F E R
//*****************************
volatile char tx_buf;
volatile char rx_char_buf;
volatile char rx_ack_buf;
volatile char rx_key_buf;
//****************
// E X T   R A M
//****************
char subaudiobuf[SUBAUDIOBUF_LEN];

