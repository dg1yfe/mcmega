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

char version_str[] PROGMEM = "0.9";

void init_ui()
{
    ui_frequency = 0;
    ui_txshift = -1;
}

void vUiTask( void * pvParameters)
{

	lcd_s_reset();
//	int_lcd_timer_dec = 1;
	cfg_head = 3;

	printf_P(PSTR("DG1YFE"));
	lcd_fill();
	lcd_cpos(0);
	printf_P(PSTR("MCmega"));
	lcd_fill();
	lcd_cpos(0);
	vTaskDelay(150);
	printf(version_str);
	lcd_fill();
	lcd_cpos(0);
	vTaskDelay(150);

	freq_print(&frequency);
	vTaskDelay(150);
	freq_offset_print();
	pll_led(1);

	menu_init();

    for(;;)
	{
    	pll_led(0);
    	led_update();
    	menu();
    	taskYIELD();
	}
}
