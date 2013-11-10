/*
 * int.h
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

#ifndef INT_H_
#define INT_H_

void init_SIO(void);
void init_OCI(void);

void init_Timer2();
void start_Timer2();
void stop_Timer2();


extern volatile uint8_t int_lcd_timer_dec;
extern volatile uint8_t int_wd_reset;
extern volatile uint16_t PL_phase_delta;
extern volatile uint16_t SEL_phase_delta;
extern volatile uint16_t SEL_phase_delta2;
extern volatile unsigned int PL_phase;
extern volatile unsigned int SEL_phase;
extern volatile unsigned int SEL_phase2;


#endif /* INT_H_ */
