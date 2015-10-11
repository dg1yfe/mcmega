/*
 * main.c
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
 
#include <stddef.h>
#include <avr/interrupt.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "regmem.h"
#include "macros.h"
#include "firmware.h"
#include "ui.h"
#include "subs.h"
#include "int.h"
#include "io.h"
#include "pll_freq.h"
#include "display.h"
#include "timer.h"
#include "eeprom.h"
#include "audio.h"
#include "config.h"

void vControlTask( void * pvParameters) __attribute__((noreturn));

#define TASK_PRIO_UI 1
#define TASK_PRIO_CONTROL 2

xTaskHandle xUiTaskHandle, xControlTaskHandle;

// Perform calibration of internal RC oscillator using the
// reset sequence (0x7e) of the control head as a reference
//
// 0x7e in 1200e1 -> (Start/0) 01111111 (Stop/1)
//                   --_________
// -> Pulse of 1,66 ms on line
// Pulse is inverted and presented to UART RX and F1/#F2 (PinA3)
void autocal(void){
	uint8_t tc1a, tc1b;
	tc1a = TCCR1A;
	tc1b = TCCR1B;
	uint8_t pulse = 1;
	uint16_t duration;
	cli();
	
	TCCR1A=0;
	TCNT1=0;
	TIFR |= (1 << TOV1);	// clear Overflow flag of timer 1
	TCCR1B=(1<<CS11);	// Timer 1 runs at CPU speed /8 (~ 1 MHz)
	
	// check 65 ms for Pulses (do not hang when control head is missing)
	while(!(TIFR & (1<<TOV1))){
		if ((pulse==0) && (PINA & (1<<3))){
			TCNT1 = 0;
			TCCR1B = (1 << CS10);	// Timer speed-up
			break;
		};	// check for falling edge
		pulse = PINA & (1<<3);		
	}
	pulse = 0;
	// Autocal only when pulses detected
	while(!(TIFR & (1<<TOV1))){
		if ((pulse==1) && !(PINA & (1<<3))){
			duration = TCNT1;
			break;
		};	// check for falling edge
		pulse = PINA & (1<<3);		
	}
	
	TCCR1B = 0;	// stop timer
	TCCR1A = tc1a;	// recall previous values
	TCCR1B = tc1b;
	
	sei();
	return;
}


int main(void)
{
	// These are the handles for both tasks this program is going to use
	xTaskHandle xUiTaskHandle, xControlTaskHandle;

	// create Main Tasks
	xTaskCreate( vUiTask, (const signed char *) "User if", 384, NULL, TASK_PRIO_UI, &xUiTaskHandle);
	xTaskCreate( vControlTask, (const signed char *) "Control", 256, NULL, TASK_PRIO_CONTROL, &xControlTaskHandle);
	// TODO: check if Task was created and try to display error

	init_io();
	// check if radio is switched on
	// power down if it is not
	pwr_sw_chk( 0 );

	// restore basic radio configuration data
	// and state where it came from (SRAM, EEPROM or FLASH)
	config_state = config_basicRadio();

	// initialize interrupt stuff;

	// no serial interrupt, this is handled within the main loop
	init_OCI();
	init_ui();

	sei();

	// start scheduler
	vTaskStartScheduler();

	// TODO: insert radio reset here

    return (1);	// should never happen
}


void vControlTask( void * pvParameters)
{
	// use own config structure to ensure validity at ANY time

	portTickType xLastWakeTime;
/*
 *              jsr  sci_init              ; serielle Schnittstelle aktivieren
                jsr  init_SIO              ; SIO Interrupt konfigurieren
                jsr  init_OCI              ; Timer Interrupt starten
                jsr  ui_init               ; 2. Task initialisieren (2. Stack)
                                           ; ab hier k�nnen I/O Funktionen verwendet werden

                ldd  #FSTEP                ; Kanalraster holen
                jsr  pll_init              ; PLL mit Kanalraster initialisieren

                ldab #1
                stab tasksw_en             ; Taskswitch verbieten

                cli
 *
 */

	init_sci();
	init_Timer2();

	// Set Channel spacing
	pll_setChannelSpacing(config.f_step);

	lcd_h_reset();

	led_set(GRN_LED, LED_ON);
	
	eep_enable(0);		// keep external EEPROM disabled

	if(config_state == CONFIG_FLASH)
		led_set(YEL_LED, LED_BLINK);

	// Initialize CTCSS TX & RX
	tone_start_pl_index(config.ctcssIndexTx);
	tone_decoder_start_index(config.ctcssIndexRx);

	// Squelch is already set, display state is shown from UI task
	// Power Mode is already set, will be applied when calling "transmit"
	// and shown by UI task (when it is allowed to run for the first time)

	// Frequency & Shift
	// set radio to RX / apply frequency
	receive();
	
	pll_timer = 1;
	//enable Audio PA, but disable RX Audio
	SetShiftReg(0, (uint8_t)~SR_RXAUDIOEN);
//	SetShiftReg(SR_RXAUDIOEN,0xff);
	audio_pa(1,1);

	s_timer_init();
	//OSCCAL = 0xbd;

	xLastWakeTime = xTaskGetTickCount();
	while (1)
    {
		uint8_t ptt_state ;
		// process samples from ADC (if active)
		tone_decode();

		// check power switch
		pwr_sw_chk(PWR_SAVECONFIG);
		// update software timer
		s_timer_update();
		// check PTT
		ptt_state = ptt_get_status();
		// check if PTT status changed
		if(ptt_state & PTT_EVENT_BIT)
		{
			if(ptt_state & ~PTT_EVENT_BIT)
				transmit();
			else
				receive();
		}
		// update squelch
		squelch();

		// reset hardware watchdog
		wd_reset();
		// run handlers for serial port
		do
		{
			sci_rx_handler();
			sci_tx_handler();
		}while((UCSR0A & (1 << RXC0)));

		// check for changes in configuration by the user
		// block for 1 tick (thus allowing UI task to run)
		config_checkForUpdate(1);

		// if task did not block, wait here for 1 tick
		// give UI task the opportunity to execute
		// but only if the sample buffer is at max 1/3 full
		if(samp_buf_count < 10)
		{
			vTaskDelayUntil( &xLastWakeTime, 1 );
		}
    }
}
