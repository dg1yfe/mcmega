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
#include "ui.h"
#include "subs.h"
#include "int.h"
#include "io.h"
#include "pll_freq.h"
#include "display.h"
#include "timer.h"

void vControlTask( void * pvParameters) __attribute__((noreturn));

xSemaphoreHandle SerialBusMutex;

int main(void)
{
	xTaskHandle xUiTaskHandle, xControlTaskHandle;

	// create Main Tasks
	xTaskCreate( vUiTask, (const signed char *) "User if", 384, NULL, 2, &xUiTaskHandle);
	xTaskCreate( vControlTask, (const signed char *) "Control", 256, NULL, 1, &xControlTaskHandle);
	// TODO: check if Task was created and try to display error

	init_io();
	// check if radio is switched on
	// power down if it is not
	pwr_sw_chk( 0 );

	// create Mutex for HW access
	SerialBusMutex = xSemaphoreCreateMutex();

	// initialize timer interrupt stuff;
	init_sci();
	init_SIO();
	init_OCI();
	init_ui();
	init_pll(FSTEP);

	sei();

	// start scheduler
	vTaskStartScheduler();

	// TODO: insert radio reset here

    return (1);	// should never happen
}


void vControlTask( void * pvParameters)
{
	char i;
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
	lcd_h_reset();
	lcd_s_reset();
	int_lcd_timer_dec = 1;
	cfg_head = 3;

	led_set(GRN_LED, LED_ON);
	//freq_init();

	taskYIELD();

	receive();
	pll_timer = 1;
	//enable Audio PA, but disable RX Audio
	SetShiftReg(SR_AUDIOPA, (uint8_t)~(SR_RXAUDIOEN));
	s_timer_init();

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
		taskYIELD();
		frq_check();
		wd_reset();
		sci_rx_handler();
		sci_tx_handler();
    }
}
