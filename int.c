/*
 * int.c
 *
 *  Created on: 26.05.2012
 *      Author: eligs
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

	if(bus_busy==0)
	{	// invert data line to reset HW watchdog
		// SR_DATAPORT ^= SR_DATABIT;
	}

	gp_timer--;

}


ISR(TIMER2_COMP_vect)
{
	uint8_t index;

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
#warning "OCR2 value exceeds 255"
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
	TCCR2 &= ~((1 << CS22) | (1 << CS21) | (1 << CS20));
}
