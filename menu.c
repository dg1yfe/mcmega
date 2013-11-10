/*
 * menu.c
 *
 *  Created on: 29.05.2012
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
#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "menu.h"
#include "menu_sub.h"
#include "menu_input.h"
#include "menu_mem.h"
#include "menu_top.h"
#include "display.h"

const char m_ok_str[] PROGMEM = "OK";
const char m_no_lock_str[] PROGMEM =  "NO LOCK ";
const char m_out_str[] PROGMEM =  "out of";
const char m_range_str[] PROGMEM =  "Range ";
const char m_writing[] PROGMEM =  "writing";
const char m_stored_str[] PROGMEM =  "stored";
const const char m_failed_str[] PROGMEM =  "failed";
const char m_delete[] PROGMEM =  "deleting";
const char m_offset[] PROGMEM =  "TXSHIFT";
const char m_sq_on_str[] PROGMEM =  "SQ ON";
const char m_sq_off_str[] PROGMEM =  "SQ OFF";


//*****************************
// I N I T _ M E N U
//*****************************
void menu_init()
{
	// initialize state variable
	m_state = IDLE;
	// disable menu timeout timer
	m_timer_en = 0;

	// set squelch mode
	sql_mode = SQM_CARRIER;
	arrow_set(2,1);

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
void menu()
{
	char c;

	if( (sci_rx_m(&c) & 0x80) == 0 )
	{
		switch(m_state)
		{
		case F_IN:
			m_f_in(c);
			break;
		case MEM_SELECT_STORE:
		case MEM_SELECT:
			m_mem_select(c);
			break;
		case MEM_STORE:
			m_store_submenu(c);
			break;
		case MEM_RECALL_LOAD :
			m_recall_submenu(c);
			break;
		case TXSHIFT_SW :
			m_txshift(c);
			break;
		case TXSHIFT_DIGIT :
			mts_digit(c);
			break;
		case MENU_SELECT:
			m_submenu(c);
			break;
		case FREQ_DIGIT:
			m_freq_digit(c);
			break;
		case POWER_SELECT:
			m_power_submenu(c);
			break;
		case DEFCH_SELECT:
			m_defch_submenu(c);
			break;
		case CTCSS_SEL_RX:
		case CTCSS_SEL_TX:
			m_ctcss_submenu(c);
			break;
		case CAL:
			m_cal_submenu(c);
			break;
		case DEBUG:
			m_debug(c);
			break;
		case TEST:
			m_test(c);
		default:
		case IDLE:
			m_top(c);
			break;
		}
	}
	else
	if(m_state == DEBUG)
	{
		m_debug(0);
	}

	// M_END

	// check if menu timer is enabled
	if (m_timer_en)
	{
		// if it reached zero
		if(!m_timer)
		{	// disable timer
			m_timer_en = 0;
			// restore previous display content
			restore_dbuf();
			// go back to IDLE state
			m_state = IDLE;
		}
	}
}


void m_none(char key)
{
	return;
}

//**************************************
////Eingabe Timeout zurücksetzen

void m_reset_timer()
{
	m_timer = MENUTIMEOUT;
	m_timer_en = 1;
}


void m_norestore()
{
	m_timer_en = 0;
	// go back to IDLE state
	m_state = IDLE;
}
