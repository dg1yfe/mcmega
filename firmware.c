/*
 * main.c
 *
 *  Created on: 26.05.2012
 *      Author: Felix Erckenbrecht
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

void vControlTask( void * pvParameters) __attribute__((noreturn));

xTaskHandle xUiTaskHandle, xControlTaskHandle;

int main(void)
{
	xTaskHandle xUiTaskHandle, xControlTaskHandle;

	// create Main Tasks
	xTaskCreate( vUiTask, (const signed char *) "User if", 384, NULL, 1, &xUiTaskHandle);
	xTaskCreate( vControlTask, (const signed char *) "Control", 256, NULL, 2, &xControlTaskHandle);
	// TODO: check if Task was created and try to display error

	init_io();
	// check if radio is switched on
	// power down if it is not
	pwr_sw_chk( 0 );

	// initialize timer interrupt stuff;
	init_SIO();
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
	char i;

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
	init_pll(FSTEP);

	lcd_h_reset();
	

	led_set(GRN_LED, LED_ON);
	
	eep_enable(1);

	if(freq_init())
		led_set(YEL_LED, LED_BLINK);

	receive();
	rfpwr_set(DEFAULT_RF_PWR);
	
	pll_timer = 1;
	//enable Audio PA, but disable RX Audio
	SetShiftReg(0, (uint8_t)~SR_RXAUDIOEN);
//	SetShiftReg(SR_RXAUDIOEN,0xff);
	audio_pa(1,1);

	s_timer_init();


	xLastWakeTime = xTaskGetTickCount();
	while (1)
    {
		pwr_sw_chk(0);
		s_timer_update();
		i=ptt_get_status();
		if(i & 0x80)
		{
			if(i & 0x7f)
				transmit();
			else
				receive();
		}
		squelch();

		frq_check();
		wd_reset();
		do
		{
			sci_rx_handler();
			sci_tx_handler();
		}while((UCSR0A & (1 << RXC0)));

		// if task did not block, wait here for 1 tick
		// give UI task the opportunity to execute
		vTaskDelayUntil( &xLastWakeTime, 1 );
    }
}
