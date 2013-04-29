/*
 * regmem.c
 *
 *  Created on: 27.05.2012
 *      Author: Felix Erckenbrecht
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
char bus_busy;
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

uint8_t ctcss_index;
int  oci_int_ctr;

int f_step;					// Schrittweite in Hz

int tick_ms;				// 1ms Increment
int s_tick_ms;				// Software timer
int tick_hms;				// 100ms Increment
int8_t next_hms;
char gp_timer;				// General Purpose Timer, 1ms Decrement
char ui_timer;
uint8_t lcd_timer;				// 1ms

unsigned long frequency;	// aktuelle Frequenz

long offset;				// Für RX/TX verwendete Shift (0/+TXS/-TXS)
long txshift;
long channel;				// aktuell in der PLL gesetzter Kanal
unsigned long ui_frequency;			// Über UI eingegebene Frequenz wird hier gespeichert
long ui_txshift;			// Über UI eingegebene Frequenz wird hier gespeichert

char rxtx_state;			// 0=RX

char cfg_pwr_mode;				// Lo / Hi Power

char ptt_debounce;

char cfg_defch_save;
char ui_ptt_req;			//

uint8_t cfg_head;				// Type of Control Head

char m_state;
int  m_timer = 0;			// 100ms
char m_timer_en;

char sql_timer;
char sql_mode;				// Mode ($80 = Carrier, $40 = RSSI, 0 = off)
char sql_flag;
uint8_t sql_pin_flag;

char mem_bank;

char pll_locked_flag;		// Bit 0 - PLL not locked
char pll_timer;

char tone_timer;
char tone_index;
char oci_ctr;

int ts_count;


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

