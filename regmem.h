/*
 * regmem.h
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
#ifndef REGMEM_H_
#define REGMEM_H_
#include <stdint.h>
//
// Port Function Macros
//
#define PORT_PTT	PORTD
#define PIN_PTT		PIND
#define BIT_PTT		(1 << 1)


//Squelch Modes
//
#define SQM_OFF       0
#define SQM_CARRIER   1
#define SQM_CTCSS     2

// Power Switch (0 = Power On)
#define PORT_SWB	PORTD
#define PIN_SWB		PIND
#define BIT_SWB		(1 << 0)

// Interface to shift register
#define PORT_SBUS_CLK  PORTD
#define DDR_SBUS_CLK   DDRD
#define BIT_SBUS_CLK   (1 << 5)


#define PORT_SR_LATCH   PORTG
#define BIT_SR_LATCH    (1 << 3)

#define DDR_SBUS_DATA	DDRE
#define PORT_SBUS_DATA	PORTE
#define PIN_SBUS_DATA	PINE
#define BIT_SBUS_DATA	(1 << 2)

#define PORT_PLL_LATCH PORTB
#define BIT_PLL_LATCH  (1 << 7)
//PLL Lock Input
#define PIN_PLL_LOCK   PINE
#define BIT_PLL_LOCK   (1 << 6)

// Shift register output
// 0 - Audio PA enable (1=enable)      (PIN 4 ) *
// 1 - STBY&9,6V (1=enable)            (PIN 5 )
// 2 - T/R Shift (0=RX, 1=TX)          (PIN 6 ) *
// 3 - Hi/Lo Power (1=Lo Power)        (PIN 7 ) *
// 4 - Ext. Alarm                      (PIN 14) *
// 5 - Sel.5 ATT   (1=Attenuated Tones)(PIN 13) *
// 6 - Mic enable  (1=enable)          (PIN 12) *
// 7 - Rx Audio enable (1=enable)      (PIN 11)
// EXT ALARM is controlled via SR in EZA9

#define SR_AUDIOPA    UINT8_C(1 << 0)
#define SR_9V6        UINT8_C(1 << 1)
#define SR_TXVCOSEL   UINT8_C(1 << 2)
#define SR_TXPWRLO    UINT8_C(1 << 3)
#define SR_nCLKSHIFT  UINT8_C(1 << 3)
#define SR_EXTALARM   UINT8_C(1 << 4)
#define SR_SELATT     UINT8_C(1 << 5)
#define SR_MICEN      UINT8_C(1 << 6)
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

extern uint8_t ctcss_index;
extern int  oci_int_ctr;

extern int f_step;			// Schrittweite in Hz

extern int tick_ms;			// 1ms Increment
extern int s_tick_ms;		// Software timer
extern int tick_hms;		// 100ms Increment
extern int8_t next_hms;
extern char gp_timer;		// General Purpose Timer, 1ms Decrement
extern char ui_timer;
extern uint8_t lcd_timer;		// 1ms
extern char lcd_timer_en;

//trx_state       .db

extern unsigned long frequency;	// aktuelle Frequenz

extern long offset;				// Für RX/TX verwendete Shift (0/+TXS/-TXS)
extern long txshift;
extern long channel;			// aktuell in der PLL gesetzter Kanal
extern unsigned long ui_frequency;	// Über UI eingegebene Frequenz wird hier gespeichert
extern long ui_txshift;			// Über UI eingegebene Frequenz wird hier gespeichert

extern char rxtx_state;			// 0=RX

char cfg_pwr_mode;				// Lo / Hi Power

extern char ptt_debounce;

extern char cfg_defch_save;
extern char ui_ptt_req;			//

extern uint8_t cfg_head;				// Type of Control Head

extern char m_state;
extern int  m_timer;				// 100ms
extern char m_timer_en;

extern char sql_timer;
extern char sql_mode;				// Mode ($80 = Carrier, $40 = RSSI, 0 = off)
extern char sql_flag;
extern uint8_t sql_pin_flag;

extern char mem_bank;

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
