/*
 * int.c
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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "FreeRTOS.h"
#include "task.h"

#include "regmem.h"
#include "io.h"
#include "audio.h"

volatile uint8_t int_wd_reset;
volatile uint8_t int_lcd_timer_dec;
volatile uint16_t PL_phase_delta;
volatile uint16_t SEL_phase_delta;
volatile uint16_t SEL_phase_delta2;
volatile unsigned int PL_phase;
volatile unsigned int SEL_phase;
volatile unsigned int SEL_phase2;


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

	gp_timer--;

}


ISR(TIMER2_COMP_vect)
{
	uint8_t index;
	static uint8_t samp_buf_w=0;

	if(!rxtx_state)	// process sample input in RX
	{
		if(g_coeff)
		{
			uint8_t sc;
/*
			uint32_t sb;
			sb=samp_buf;
			sb <<= 1;
			if(PIN_SIGIN & BIT_SIGIN)
			{
				sb |= 1;
			}
			samp_buf = sb;
*/
			samp_buf[samp_buf_w++] = ADCH;
			ADCSRA |= (1 << ADSC);		// start next conversion now
			samp_buf_w &= SAMP_BUF_LEN -1;
			sc = samp_buf_count;
			sc++;
			samp_buf_count = sc;
		}
	}
	else	// Tone oscillators are only used in TX
	{
		if(PL_phase_delta)
		{
			PL_phase += PL_phase_delta;
			index = PL_phase >> 8;
			index = pgm_read_byte(&sin_tab[index]);
			index >>=1;
			index |= PORT_PL_DAC & (~MASK_PL_DAC);
			PORT_PL_DAC = index;
		}

		if(SEL_phase_delta2)
		{
			uint8_t index2;
			SEL_phase  += SEL_phase_delta;
			index = SEL_phase >> 8;
			index = pgm_read_byte(&sin_tab[index]);

			SEL_phase2 += SEL_phase_delta2;
			index2 = SEL_phase2 >> 8;
			index2 = pgm_read_byte(&sin_tab[index2]);

			index+= index2;
			index >>= 1;
			index |= PORT_SEL_DAC & (~MASK_SEL_DAC);
			PORT_SEL_DAC = index;
		}
		else
		if(SEL_phase_delta)
		{
			SEL_phase += SEL_phase_delta;
			index = SEL_phase >> 8;
			index = pgm_read_byte(&sin_tab[index]);
			index |= PORT_SEL_DAC & (~MASK_SEL_DAC);
			PORT_SEL_DAC = index;
		}
	}
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

/*
 * Timer for DDS oscillator
 */
void init_Timer2()
{
#if (F_CPU / FS / 8) > 255
#warning "OCR2 value exceeds 255 - (OCR2 is the result of F_CPU / FS / 8 )"
#endif
	OCR2 = (uint8_t) (F_CPU/FS / 8) - 1;
	TCCR2 |= (1 << WGM21) | (0 << WGM20) ;
	TIMSK |= (1 << OCIE2);
}


void start_Timer2()
{
	TCCR2 |= ((0 << CS22) | (1 << CS21) | (0 << CS20));
}


void stop_Timer2()
{
	if(!(SEL_phase_delta || PL_phase_delta || g_coeff))
	{
		TCCR2 &= ~((1 << CS22) | (1 << CS21) | (1 << CS20));
	}
}
