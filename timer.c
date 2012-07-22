/*
 * timer.c
 *
 *  Created on: 10.07.2012
 *      Author: F. Erckenbrecht
 */
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

#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "menu.h"
#include "subs.h"
#include "int.h"

/*
 * Initialisiert Software Timer
 */
void s_timer_init()
{
	// do not decrease LCD timer in ISR
	int_lcd_timer_dec = 0;
	s_tick_ms = tick_ms;
	next_hms = 100;
}

/*
 * Update of software timers
 */
void s_timer_update()
{
	signed int tickdiff;

	tickdiff = tick_ms - s_tick_ms;
	if(tickdiff)
	{
		s_tick_ms += tickdiff;

		// decrement LCD timer, if timer is >0
		if(lcd_timer)
		{	// decrement and saturate at 0
			lcd_timer = (lcd_timer > tickdiff) ? lcd_timer - tickdiff : 0;
		}

		if(ui_timer)
		{
			ui_timer = (ui_timer > tickdiff) ? ui_timer - tickdiff : 0;
		}

		if(sql_timer)
		{
			sql_timer = (sql_timer > tickdiff) ? sql_timer - tickdiff : 0;
		}

		next_hms -= (int8_t) tickdiff;
		// decrement 100 ms timer
		if(next_hms<=0)
		{
			next_hms += 100;
			tick_hms++;

			if(m_timer)
				m_timer--;

			if(pll_timer)
				pll_timer--;

			// decrement tone timer if > 0
			if(tone_timer)
			{
				tone_timer--;
				// stop tone output if 0 was reached
				if(!tone_timer)
				{
					tone_stop();
				}
			}

		}
	}
}


void wait_ms(unsigned int ticks)
{
	vTaskDelay(ticks);
}

