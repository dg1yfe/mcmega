/*
 * menu_top.h
 *
 *  Created on: 13.07.2012
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

#ifndef MENU_TOP_H_
#define MENU_TOP_H_

extern void m_frq_up(void);
extern void m_frq_down(void);
void m_sql_switch(void);
void m_tone(void);
extern void m_txshift(char key);
extern void mts_print(void);
extern void mts_switch(char key);
void mts_digit(char key);
void m_set_shift(void);
void m_prnt_rc(void);
void m_test(char key);
void m_tone_stop(void);
void m_prnt_tc(void);
void m_submenu(char key);
char m_freq_digit(char key);
void m_top(uint8_t key);
void m_debug(char key);


#endif /* MENU_TOP_H_ */
