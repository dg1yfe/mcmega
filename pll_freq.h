/*
 * pll_freq.h
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

#ifndef PLL_FREQ_H_
#define PLL_FREQ_H_

#include <stdint.h>

uint8_t freq_init(void);
char init_pll(unsigned int spacing);
void pll_led(char force);
char pll_lock_chk(void);
void pll_set_channel(unsigned long channel);
void set_freq(unsigned long * freq);
void set_tx_freq(unsigned long * freq);
void set_rx_freq(unsigned long * freq);
unsigned long frq_cv_freq_ch(unsigned long * freq);
unsigned long frq_cv_ch_freq(unsigned long ch);
unsigned long frq_get_freq(void);
unsigned long frq_calc_freq(char * str);
void frq_update(unsigned long *freq);
void freq_print(unsigned long * freq);
void freq_offset_print(void);
void frq_check(void);




#endif /* PLL_FREQ_H_ */
