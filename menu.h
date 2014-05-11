/*
 * menu.h
 *
 *  Created on: 29.05.2012
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

#ifndef MENU_H_
#define MENU_H_
#include <avr/pgmspace.h>

extern const char m_ok_str[] PROGMEM;
extern const  char m_no_lock_str[] PROGMEM;
extern const  char m_out_str[] PROGMEM;
extern const  char m_range_str[] PROGMEM;
extern const  char m_writing[] PROGMEM;
extern const  char m_stored_str[] PROGMEM;
extern const  char m_failed_str[] PROGMEM;
extern const  char m_delete[] PROGMEM;
extern const  char m_offset[] PROGMEM;
extern const  char m_sq_on_str[] PROGMEM;
extern const  char m_sq_off_str[] PROGMEM;
extern const  char m_menu_str[] PROGMEM;

enum {M_IDLE, F_IN, MEM_SELECT, MEM_STORE, MEM_RECALL_LOAD,
	  TXSHIFT_SW, MENU_SELECT, TXSHIFT_DIGIT,
	  FREQ_DIGIT, POWER_SELECT, CONFIG_SELECT, MEM_SELECT_RECALL,
	  MEM_SELECT_STORE, SHOW_VERSION, CTCSS_SEL_RX, CTCSS_SEL_TX,
	  CAL, TEST, DEBUG};
//#define MEM_SEL_DIGIT 5


void menu_init();
void menu();
void m_none(char key);
extern void m_reset_timer();
extern void m_norestore();


#endif /* MENU_H_ */
