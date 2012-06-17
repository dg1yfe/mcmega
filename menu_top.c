/*
 * menu_top.c
 *
 *  Created on: 29.05.2012
 *      Author: F. Erckenbrecht
 */


#include <stdlib.h>
#include <stdint.h>

#include <avr/pgmspace.h>
#include "macros.h"
#include "regmem.h"

typedef struct PROGMEM
{
	char * label;
	void * fptr(char key);
} const T_MENUITEM;

T_MENUITEM m_submenu_list[] =
{
		{ PSTR("MENU    "), m_recall_submenu},
		{ PSTR("RECALL  "), m_recall_submenu},
		{ PSTR("STORE   "), m_store_submenu},
		{ PSTR("TX CTCSS"), m_none},
		{ PSTR("RX CTCSS"), m_none},
		{ PSTR("DTMF    "), m_none},
		{ PSTR("POWER   "), m_power_submenu}
};

static uint8_t menu_index;


//*****************************
// M E N U   I D L E
//*****************************
//
// Menu / IDLE Subroutines
//
// Main Menu / Top Level
//
// Parameter : none
//
// Ergebnis : none
//
// changed Regs : A,B,X
//
//
//************************
// Stack depth on entry: 1
//
//*******************************
// M   T O P
//
void m_top(uint8_t key)
{
	void (*fptr)(uint8_t key);

	switch(key)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			m_start_input(key);
			break;
		case KC_EXIT:
			break;
		case KC_D1:
			m_frq_up();
			break;
		case KC_D2:
			m_frq_down();
			break;
		case KC_D3:
			m_sql_switch();
			break;
		case KC_D4:
			m_test();
			//m_none();
			//m_prnt_tc();
			break;
		case KC_D5:
			m_tone();
			break;
		case KC_D6:
			if(cfg_head == CHD2)
			{
				if(!m_timer_en)
					save_dbuf();
				// initialize digit editor
				m_digit_editor(0, 0, 0, -1);
				m_state = FREQ_DIGIT;
			}
			break;
		case KC_D7:
			m_txshift(key);
			break;
		case KC_D8:
			m_recall();
			break;
		case KC_ENTER:
			m_submenu();
		default:
			break;
	}
}

//*******************************
// M   F R Q   U P
//
// Frequenz einen Kanal nach oben
//
inline void m_frq_up()
{
	frequency += FSTEP;
	frq_update();
	m_frq_prnt();
}

//*******************************
// M   F R Q   D O W N
//
// Frequenz einen Kanal nach unten
//
inline void m_frq_down()
{
	frequency -= FSETP;
	frq_update();
	m_frq_prnt();
}
//*******************************
// M   S Q L   S W I T C H
//
// Squelchumschaltung Carrier/RSSI/Aus
//
// Carrierlevel wird am Demod IC eingestellt,
// RSSI-Level auf der RSSI Platine
// Carriersquelch l�sst niedrigere Schwelle zu als RSSI Squelch
//
void m_sql_switch()
{
	if (sql_mode == SQM_CARRIER)
	{
		sql_mode = SQM_OFF;
		arrow_set(1,0);
	}
	else
	{
		sql_mode = SQM_CARRIER;
		arrow_set(1,1);
	}
}

//**************************************
// M   T O N E
//
// 1750 Hz Ton ausgeben
//
void m_tone()
{
	ui_ptt_req = 1;
	if(tone_timer==0)
	{
		tone_start();
	}
	tone_timer = 6;	// 600 ms
}

//**************************************
// M   T X   S H I F T
//
// Anzeige der aktuellen TX Shift
//
inline void m_txshift(char key)
{
	if(!m_timer_en)
	{
		save_dbuf();
	}
	if(m_state != TXSHIFT_SW)
	{
		m_state = TXSHIFT_SW;
		m_reset_timer();
		lcd_cpos(0);
		printf(m_offset);
		lcd_fill();
		vTaskDelay(100);
		mts_print();
	}
	else
		mts_switch(key);

}

inline void mts_print()
{
	lcd_cpos(0);
	decout(PRINTSIGN | 5, 3, offset);
	//TODO: Replace with printf ?
	lcd_fill();
	freq_offset_print();
}

//*********************
// M T S   S W I T C H
//
// Aktivieren/Deaktivieren der Ablage, Vorzeichenwahl (+/-)
//
//
inline void mts_switch(char key)
{
	m_reset_timer();
	switch(key)
	{
		case KC_ENTER:
			offset = -offset;
			txshift = offset;
			ui_txshift = offset;
			taskYIELD();
			mts_print();
			break;
		case KC_D7:
			mts_toggle();
			m_reset_timer();
			if(offset)
			{
				ui_txshift = 0;
				taskYIELD();
			}
			else
			{
				ui_txshift = txshift;
				taskYIELD();
			}
			mts_print();
			break;
		case KC_D6:
			m_state = TXSHIFT_DIGIT;
			m_digit_editor(0,0,0,-1);
			break;
		case KC_EXIT:
			m_timer=0;
			break;
		default:
			restore_dbuf();
			// TODO: m_top() ?
			break;
	}
}


//*******************
// M T S   D I G I T
//
// TX Shift per Digit Eingabe setzen
//
// Stack depth on entry: 1
//
void mts_digit()
{
	char res;

	m_reset_timer();

	if(res = m_digit_editor(key, 1,2,0))
	{
		if(res>0)
		{
			long f;
			f = atoi(dbuf);
			f *= 1000;
			txshift = f;
			ui_txshift = f;
			taskYIELD();
			m_state = IDLE;
			m_timer = 8;	// wait 800 ms before reverting to frequency
//TODO Print "OK"
		}
		else
			m_timer = 0;
	}
}


//**************************************
// M   S E T   S H I F T
//
// TX Shift per Direkteingabe setzen
//
void m_set_shift()
{
	long f;
	f = atol(f_in_buf);
	f *= 1000;
	txshift = f;
	ui_txshift = f;
	taskYIELD();
	m_state = IDLE;
	m_timer = 8;	// wait 800 ms before reverting to frequency
}



//**************************************
// M   P R N T   R C
//
// Anzahl der Hauptschleifendurchl�ufe in der letzten Sekunde anzeigen
//
void m_prnt_rc()
{
	if(!m_timer_en)
	{
		save_dbuf();
	}
	m_reset_timer();
	lcd_cpos(0);
	//pputchar('l',0,&rc_last_sec);
	lcd_fill();
}

//**************************************
// M   T E S T
//
//
void m_test()
{
	tone_start(123*4);
}

void m_tone_stop()
{
	tone_stop();
}


//**************************************
// M   P R N T   T C
//
// Anzahl der Taskswitches in der letzten Sekunde anzeigen
//
void m_prnt_tc()
{
	if(!m_timer_en)
	{
		save_dbuf();
	}
	m_reset_timer();
	lcd_cpos(0);
	pputchar('l',0,&sql_timer);
	lcd_fill();
}


//*************
// M   M E N U
//
// Call Submenu
//
inline void m_submenu(char key)
{
	if(!m_timer_en)
	{
		save_dbuf();
	}
	m_reset_timer();
	switch(m_state)
	{
		default:
		{
			lcd_cpos(0);
			menu = 0;
			m_state = MENU_SELECT;
			menu_index = 0;
			printf(m_menu_str);
			break;
		}
		case MENU_SELECT:
		{
			switch(key)
			{
				case KC_D1:
				{
					if(m_index == M_MENU_ENTRIES)
						m_index = 1;
					else
						m_index++;
					lcd_cpos(0);
					printf(m_submenu_list[m_index].label);
					break;
				}
				case KC_D2:
				{
					if(m_index == 0)
						m_index = M_MENU_ENTRIES;
					else
						m_index--;
					lcd_cpos(0);
					printf(m_submenu_list[m_index].label);
					break;
				}
				case KC_ENTER:
				{
					m_submenu_list[m_index].fptr();
					break;
				}
				case KC_EXIT:
				{
					m_timer = 0;
					break;
				}
			}
		}
	}
}


//***************************
// M   D I G I T
//
// select frequency digit to alter using up/down
//
// Stack depth on entry: 2
//
char m_freq_digit(void)
{
	signed char res;

	m_reset_timer();

	if(res = m_digit_editor(key, 4,3,0))
	{
		long f;

		if(res>0)
		{
			f = atol(f_in_buf);
			f *= 1000;
			frq_update(&f);
			taskYIELD();
			printf(m_ok);
			vTaskDelay(200);
			m_frq_prnt();
		}
		else
		{
			m_timer = 0;	// wait 800 ms before reverting to frequency
		}
	}
}
