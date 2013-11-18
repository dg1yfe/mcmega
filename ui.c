/*
 * ui.c
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
#include <stdio.h>
#include <avr/pgmspace.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "firmware.h"
#include "display.h"
#include "pll_freq.h"
#include "menu.h"
#include "int.h"
#include "io.h"
#include "subs.h"
#include "audio.h"

const char version_str[] PROGMEM = "13_2beta";

void init_ui()
{
    ui_frequency = 0;
    ui_txshift = -1;
	sql_mode = SQM_CARRIER;
}


static void reset_ui(void)
{
	freq_print(&frequency);
	vTaskDelay(15);
	freq_offset_print();
	rfpwr_print();
	pll_led(1);

	menu_init();
}


void vUiTask( void * pvParameters)
{
	lcd_s_reset();
//	int_lcd_timer_dec = 1;
	cfg_head = CONTROL_HEAD3;

   	led_update();
#ifndef DEBUG
	printf_P(PSTR("DG1YFE"));
	lcd_fill();
	lcd_cpos(0);
	printf_P(PSTR("MCmega"));
	lcd_fill();
	lcd_cpos(0);
	vTaskDelay(150);
	printf_P(version_str);
	lcd_fill();
	lcd_cpos(0);
	vTaskDelay(150);
#endif
	reset_ui();

	lcd_cpos(0);

    for(;;)
	{
    	pll_led(0);
    	led_update();
    	menu();
    	taskYIELD();
		
		// check if reset of control head was detected
		// (certain amount of 0x7e reset messages was received)
		if(!ch_reset_detected)
		{
			lcd_s_reset();
			reset_ui();
		}		
	}
}
