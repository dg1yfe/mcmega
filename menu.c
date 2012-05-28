/*
 * menu.c
 *
 *  Created on: 29.05.2012
 *      Author: Felix Erckenbrecht, DG1YFE
 */
#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"

//*****************************
// I N I T _ M E N U
//*****************************
void menu_init()
{
	// initialize state variable
	m_state = IDLE;
	// disable menu timeout timer
	m_timer_en = 0;
	// reset menu variables
	m_svar1 = 0;
	m_svar2 = 0;
	// reset menu input buffer pointer
	io_menubuf_r = 0;
	io_menubuf_w = 0;

	// set squelch mode
	sql_mode = SQM_CARRIER;
	arrow_set(2,1);

}


void menu()
{
	char c;

	if( (sci_rx_m(&c) & 0x80) == 0 )
	{

	}
}
//
//*****************************
// M E N U
//*****************************
//
// "Men�" Subroutine
//
// Steuert die komplette Bedienung des Ger�tes
// Frequenzeingabe, Speicherkanalwahl, etc.
//
// Parameter : none
//
// Ergebnis : none
//
// changed Regs : A,B,X
//
//
//************************
// Stack depth on entry: 2
//
menu
                jsr  sci_rx_m
                tsta
                bpl  m_keypressed
                jmp  m_end
m_keypressed
                pshb                             // save key

		ldab m_state                     // Status holen
                aslb
                ldx  #m_state_tab                // Tabellenbasisadresse holen
                abx
                pulb                             // Tastenwert wiederholen
                cpx  #m_state_tab_end
                bcc  m_break                     // sicher gehen dass nur existierende States aufgerufen werden
                ldx  0,x                         // Adresseintrag aus Tabelle lesen
                jmp  0,x                         // Zu Funktion verzweigen
m_break
                jmp  m_end

m_state_tab
                .dw m_top             // Top Menu
                .dw m_f_in            // Frequenzeingabe
                .dw m_mem_select      // Memory Slot ausw�hlen
                .dw m_store
                .dw m_recall_load
                .dw m_txshift
                .dw m_menu_select
m_state_tab_end
