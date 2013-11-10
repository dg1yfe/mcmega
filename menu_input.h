/*
 * menu_input.h
 *
 *  Created on: 02.06.2012
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

#ifndef MENU_INPUT_H_
#define MENU_INPUT_H_
#include <stdint.h>

void m_start_input(char key);
void m_print(char key);
void m_f_in(char key);
void m_non_numeric(char key);
void m_backspace (void);
void m_clr_displ (void);
void m_set_freq (void);
void m_set_freq_x(void);
void m_frq_prnt(void);
char m_digit_editor(char key, char lopos, char highpos, int8_t mode);

#endif /* MENU_INPUT_H_ */
