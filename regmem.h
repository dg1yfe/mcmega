/*
 * regmem.h
 *
 *  Created on: 26.05.2012
 *      Author: F. Erckenbrecht
 */

#ifndef REGMEM_H_
#define REGMEM_H_
#include <stdint.h>
//****************************************************************************
//
//    MC70 - Firmware for the Motorola MC micro trunking radio
//           to use it as an Amateur-Radio transceiver
//
//    Copyright (C) 2004 - 2011  Felix Erckenbrecht, DG1YFE
//
//     This file is part of MC70.
//
//     MC70 is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     MC70 is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY// without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with MC70.  If not, see <http://www.gnu.org/licenses/>.
//
//
//
//****************************************************************************
//
// Port Function Macros
//
#define PTTPORT       Port6_Data
#define PTTBIT        (1<< 7)


//Squelch Input
#define SQPORT        PORTE
#define SQBIT         (1 << 7)

//
#define SQM_OFF       0
#define SQM_CARRIER   SQBIT

// Power Switch (0 = Power On)
#define SWBPORT		PORTD
#define SWBDISABLED	(1 << 0)

// Interface to shift register
#define SR_CLKPORT     PORTD
#define SR_CLKDDR      DDRD
#define SR_CLKBIT      (1 << 5)
#define SR_DATAPORT    PORTD
#define SR_DATADDR     DDRD
#define SR_DATABIT     (1 << 6)

#define SR_LATCHPORT   PORTG
#define SR_LATCHEN     (1 << 3)

#define SBUS_DINPORT	PINE
#define SBUS_DINBIT		(1 << 2)

#define PLL_LATCHPORT  PORTB
#define PLL_LATCHEN    (1 << 7)
//PLL Lock Input
#define PLL_LOCKPORT      PINE
#define PLL_LOCKBIT       (1 << 6)

// Shift register output
// 0 - Audio PA enable (1=enable)      (PIN 4 ) *
// 1 - STBY&9,6V (1=enable)            (PIN 5 )
// 2 - T/R Shift (0=TX, 1=TX)          (PIN 6 ) *
// 3 - Hi/Lo Power (1=Lo Power)        (PIN 7 ) *
// 4 - Ext. Alarm                      (PIN 14) *
// 5 - Sel.5 ATT   (1=Attenuated Tones)(PIN 13) *
// 6 - Mic enable  (1=enable)          (PIN 12) *
// 7 - Rx Audio enable (1=enable)      (PIN 11)
// EXT ALARM is controlled via SR in EZA9

#define SR_AUDIOPA    (1 << 0)
#define SR_9V6        (1 << 1)
#define SR_RXVCOSEL   (1 << 2)
#define SR_TXPWRLO    (1 << 3)
#define SR_nCLKSHIFT  (1 << 3)
#define SR_EXTALARM   (1 << 4)
#define SR_SELATT     (1 << 5)
#define SR_MICEN      (1 << 6)
#define SR_RXAUDIOEN  UINT8_C(1 << 7)

#define SQEXTBIT SR_EXTALARM

//*******************
// R E G I S T E R S
//*******************

extern char SR_data_buf;
extern char bus_busy;
extern void * oci_vec;
extern char tasksw;
extern char last_tasksw;
extern char tasksw_en;
extern int  start_task;

extern char pcc_cdiff_flag; 	// Flag

extern char led_buf;	// Bit 0 (1)  - gelb
						// Bit 1 (2)  - gelb blink
						// Bit 2 (4)  - gr�n
						// Bit 3 (8)  - gr�n blink
						// Bit 4 (16) - rot
						// Bit 5 (32) - rot blink
						// Bit 6 (64) - unused
						// Bit 7 (128)- change flag
extern char led_dbuf;

extern uint16_t arrow_buf;		// Bit  0 - Arrow 0
							// Bit  1 - Arrow 1
							// ...
							// Bit  8 - Arrow 0 blink
							// ...
							// Bit 14 - Arrow 6 blink

extern char dbuf[8];		// Main Display Buffer
extern uint8_t cpos;			// Cursorposition

extern char dbuf2[8];		// Display Buffer2 + Byte f�r CPOS
extern uint8_t cpos2;

extern char f_in_buf[9];	// 9 byte buffer

extern int  osc1_phase;		// dual use: frequency input
extern int  osc1_pd;		// & oscialltor 1 & 2 (1750 Hz & DTMF)
extern int  osc2_phase;
extern int  osc2_pd;
extern int  oci_int_ctr;

extern int f_step;			// Schrittweite in Hz

extern int tick_ms;			// 1ms Increment
extern int s_tick_ms;		// Software timer
extern int tick_hms;		// 100ms Increment
extern char gp_timer;		// General Purpose Timer, 1ms Decrement
extern char ui_timer;
extern char next_hms;
extern char lcd_timer;		// 1ms
extern char lcd_timer_en;

//trx_state       .db

extern long frequency;				// aktuelle Frequenz

extern long offset;				// Für RX/TX verwendete Shift (0/+TXS/-TXS)
extern long txshift;
extern long channel;				// aktuell in der PLL gesetzter Kanal
extern long ui_frequency;			// Über UI eingegebene Frequenz wird hier gespeichert
extern long ui_txshift;			// Über UI eingegebene Frequenz wird hier gespeichert

extern char rxtx_state;			// 0=RX

char pwr_mode;				// Lo / Hi Power

extern char ptt_debounce;

//cfg_defch_save
extern char ui_ptt_req;			//

extern char cfg_head;				// Type of Control Head

extern char m_state;
extern int  m_timer;				// 100ms
extern char m_timer_en;

extern char sql_timer;
extern char sql_mode;				// Mode ($80 = Carrier, $40 = RSSI, 0 = off)
extern char sql_ctr;

extern char pll_locked_flag;		// Bit 0 - PLL not locked
extern char pll_timer;

extern char tone_timer;
extern char tone_index;
extern char oci_ctr;

extern int ts_count;

extern int osc1_dither;
extern int osc3_phase;				// dual use: frequency input
extern char osc_buf;
extern int osc3_pd;				// & oscialltor 1 & 2 (1750 Hz & DTMF)
extern char o2_en_;
extern char o2_en1;
extern char o2_en2;
extern char o2_dither;

//*****************************
// I O   R I N G B U F F E R
//*****************************
extern volatile char tx_buf;
extern volatile char rx_char_buf;
extern volatile char rx_ack_buf;
extern volatile char rx_key_buf;

//****************
// E X T   R A M
//****************
#define SUBAUDIOBUF_LEN 24
extern char subaudiobuf[SUBAUDIOBUF_LEN];


#endif /* REGMEM_H_ */
