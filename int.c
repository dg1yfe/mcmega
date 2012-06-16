/*
 * int.c
 *
 *  Created on: 26.05.2012
 *      Author: eligs
 */
#include <avr/io.h>
#include <util/delay.h>
#include "FreeRTOS.h"
#include "task.h"

#include "regmem.h"

volatile uint8_t int_wd_reset;
volatile uint8_t int_lcd_timer_dec;

void vApplicationTickHook()
{

	tick_ms++;
	if(tick_ms == next_hms)
	{
		tick_hms++;
		next_hms+=100;
	}

	if(lcd_timer && int_lcd_timer_dec)
	{
		lcd_timer--;
	}

	if(bus_busy==0)
	{	// invert data line to reset HW watchdog
		SR_DATAPORT ^= SR_DATABIT;
	}

	gp_timer--;

}


void init_SIO()
{
	//TODO : Bitrate

	// enable UART RX interrupt
	//UCSR0B |= (1 << RXCIE0);
}


void init_OCI()
{
	tick_ms = 0;
	tick_hms = 0;
	int_lcd_timer_dec = 0;
}
