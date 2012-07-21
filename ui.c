/*
 * ui.c
 *
 *  Created on: 26.05.2012
 *      Author: F. Erckenbrecht
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

char version_str[] PROGMEM = "0.9";

void init_ui()
{
    ui_frequency = 0;
    ui_txshift = -1;
}


static reset_ui()
{
	freq_print(&frequency);
	vTaskDelay(15);
	freq_offset_print();
	pll_led(1);

	menu_init();
}


void vUiTask( void * pvParameters)
{

	lcd_s_reset();
//	int_lcd_timer_dec = 1;
	cfg_head = CONTROL_HEAD3;

   	led_update();
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

	reset_ui();

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
