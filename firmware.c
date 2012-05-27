/*
 * main.c
 *
 *  Created on: 26.05.2012
 *      Author: Felix Erckenbrecht
 */
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "regmem.h"
#include "macros.h"
#include "ui.h"
#include "subs.h"

void vControlTask( void * pvParameters) __attribute__((noreturn));

xSemaphoreHandle SerialBusMutex;

int main(void)
{
	xTaskHandle xUiTaskHandle, xControlTaskHandle;

	// check if radio is switched on
	// power down if it is not
	pwr_sw_chk( 0 );

	// TODO: check if Task was created and try to display error
	SerialBusMutex = xSemaphoreCreateMutex();
	xTaskCreate( vUiTask, "User if", 384, NULL, 2, &xUiTaskHandle);
	xTaskCreate( vControlTask, "Control", 256, NULL, 2, &xControlTaskHandle);

	vTaskStartScheduler();

	// TODO: insert radio reset here

    return (1);	// should never happen
}


void vControlTask( void * pvParameters)
{
	while (1)
    {
        __asm__ volatile("nop");		// so the endless loop isn't optimized away
    }
}
