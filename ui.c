/*
 * ui.c
 *
 *  Created on: 26.05.2012
 *      Author: F. Erckenbrecht
 */
#include "FreeRTOS.h"
#include "task.h"

#include "regmem.h"

void init_ui()
{
    ui_frequency = 0;
    ui_txshift = -1;
}



void vUiTask( void * pvParameters)
{

    for(;;)
	{
        __asm__ volatile("nop");
	}
}
