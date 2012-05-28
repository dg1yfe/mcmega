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


void vControlTask( void * pvParameters) __attribute__((noreturn));

xSemaphoreHandle SerialBusMutex;

int main(void)
{
	xTaskHandle xUiTaskHandle, xControlTaskHandle;

	init_io();
	// check if radio is switched on
	// power down if it is not
	pwr_sw_chk( 0 );

	// create Mutex for HW access
	SerialBusMutex = xSemaphoreCreateMutex();
	// create Main Tasks
	xTaskCreate( vUiTask, "User if", 384, NULL, 2, &xUiTaskHandle);
	xTaskCreate( vControlTask, "Control", 256, NULL, 1, &xControlTaskHandle);
	// TODO: check if Task was created and try to display error

	// initialize timer interrupt stuff;
	init_sci();
	init_SIO();
	init_OCI();
	init_ui();
	//init_pll(FSTEP);

	sei();

	// start scheduler
	vTaskStartScheduler();

	// TODO: insert radio reset here

    return (1);	// should never happen
}


void vControlTask( void * pvParameters)
{
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
	// initialize UART Interrupt handling and enable irq

	while (1)
    {
        __asm__ volatile("nop");		// so the endless loop isn't optimized away
    }
}
