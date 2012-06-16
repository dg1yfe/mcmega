/*
 * menu.c
 *
 *  Created on: 29.05.2012
 *      Author: Felix Erckenbrecht, DG1YFE
 */
#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"

char m_ok_str[] PROGMEM = "OK";
char m_no_lock_str[] PROGMEM =  "NO LOCK ";
char m_out_str[] PROGMEM =  "out of";
char m_range_str[] PROGMEM =  "Range ";
char m_writing[] PROGMEM =  "writing";
char m_stored[] PROGMEM =  "stored";
char m_failed[] PROGMEM =  "failed";
char m_delete[] PROGMEM =  "deleting";
char m_offset[] PROGMEM =  "TXSHIFT";
char m_sq_on_str[] PROGMEM =  "SQ ON";
char m_sq_off_str[] PROGMEM =  "SQ OFF";


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

	// set squelch mode
	sql_mode = SQM_CARRIER;
	arrow_set(2,1);

}
//; Menu
//#DEFINE IDLE  	     0
//#DEFINE F_IN 	     1
//#DEFINE MEM_SELECT   2
//#DEFINE MEM_STORE    3
//#DEFINE MEM_RECALL_LOAD 4
//#DEFINE TXSHIFT_SW   5
//#DEFINE MENU_SELECT  6
//;#DEFINE MEM_SEL_DIGIT 5
//;


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
void menu()
{
	char c;

	if( (sci_rx_m(&c) & 0x80) == 0 )
	{
		switch(m_state)
		{
		case F_IN:
			m_f_in();
			break;
		case MEM_SELECT:
			m_mem_select();
			break;
		case MEM_STORE:
			m_store();
			break;
		case MEM_RECALL_LOAD :
			m_recall_load();
			break;
		case TXSHIFT_SW :
			m_txshift();
			break;
		case MENU_SELECT:
			m_menu_select();
			break;
		default:
		case IDLE:
			m_top();
			break;
		}
	}

	// M_END

	// check if menu timer is enabled
	if (m_timer_en)
	{
		// if it reached zero
		if(m_timer==0)
		{	// disable timer
			m_timer_en = 0;
			// restore previous display content
			restore_dbuf();
			// go back to IDLE state
			m_state = IDLE;
			// reset menu variables
			m_svar1 = 0;
			m_svar2 = 0;
		}
	}
}


//**************************************
////Eingabe Timeout zurücksetzen
inline void m_reset_timer()
{
	m_timer = MENUTIMEOUT;
	m_timer_en = 1;
}
