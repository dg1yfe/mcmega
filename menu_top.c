/*
 * menu_top.c
 *
 *  Created on: 29.05.2012
 *      Author: F. Erckenbrecht
 */


#include <stdlib.h>
#include <avr/pgmspace.h>
#include "macros.h"
#include "regmem.h"

typedef struct PROGMEM
{
	char * label;
	void * fptr;
} const T_MENUITEM;

T_MENUITEM m_top_menu[] =
{
		{ PSTR("MENU    "), m_recall_submenu},
		{ PSTR("RECALL  "), m_recall_submenu},
		{ PSTR("STORE   "), m_store_submenu},
		{ PSTR("TX CTCSS"), m_none},
		{ PSTR("RX CTCSS"), m_none},
		{ PSTR("DTMF    "), m_none},
		{ PSTR("POWER   "), m_power_submenu}
};



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

	if(cfg_head == 3)
	{
// Control Head 3
//     ---------------------------  1   2   3
//     !                         !
// D1  !                         !  4   5   6
//     !                         !
// D2  !                         !  7   8   9
//     ---------------------------
//     D3  (D4)  D5  (D6)  D7  (D8) *   0   #
//
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
			case 0x10:
				m_none();
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
				m_none();
				break;
			case KC_D7:
				m_txshift(key);
				break;
			case KC_D8:
				m_recall();
				break;
			case 0x19:
				m_frq_store();
			default:
				m_none();
		}
	}
	else
	if(cfg_head == 2)
	{
// Control Head 2
//     ---------------------------
//     !                         !
// D1  !                         !    5
//     !                         !
// D2  !                         !    8
//     ---------------------------
//     D3   D4   D5   D6   D7   D8
//
		switch(key)
		{
			case 0:
				m_prnt_tc();
				break;
			case 1:
				m_test2();
				break;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				m_test();
				break;
			case 8:
				m_menu();
				break;
			case 9:
				m_test();
				break;
			case 0x10:
				m_tone_stop();
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
				break;
			case KC_D5:
				m_tone();
				break;
			case KC_D6:
				m_digit();
				break;
			case KC_D7:
				m_txshift(key);
				break;
			case KC_D8:
				m_recall();
				break;
			case 0x19:
				m_none();
			default:
				m_none();
		}
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
	if(m_timer_en==0)
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
	if(cfg_head==2)
	{
		switch(key)
		{
			case 5:
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
				mts_digit();
				break;
			case 8:
				m_timer=0;
				break;
			default:
				restore_dbuf();
				// TODO: m_top() ?
				break;
		}
	}
	else
	{
		switch(key)
		{
			case KC_STERN:
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
				mts_digit();
				break;
			case KC_RAUTE:
				m_timer=0;
				break;
			default:
				restore_dbuf();
				// TODO: m_top() ?
				break;
		}
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
	if(m_digit_editor(1,2,0))
		m_timer = 0;
	else
	{
		long f;
		f = atoi(dbuf);
		f *= 1000;
		txshift = f;
		ui_txshift = f;
		taskYIELD();
		m_state = IDLE;
		m_timer = 8;	// wait 800 ms before reverting to frequency
	}
}
//                ldaa #(1<<4)+2        // digits 1 & 2 editable
//                clrb                  // decimal mode
 //               jsr  m_digit_editor   // call digit editor
//                bra  msh_set_str      // set shift
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
void m_menu()
{
	if(!m_timer_en)
	{
		save_dbuf();
	}
	m_reset_timer();
	lcd_cpos(0);
	m_svar1 = 0;
	m_state = MENU_SELECT;
	printf(m_menu_str);
}

//*************
// M   M E N U
//
// Call Submenu
//
void m_menu_select(char key)
{
	m_reset_timer();
	if(cfg_head == 2)
	{
		switch(key)
		{
		case HD2_ENTER:
			mms_execute();
			break;
		case HD2_EXIT:
			mms_exit();
			break;
		case KC_D1:
			mms_cycle_up();
			break;
		case KC_D2:
			mms_cycle_down();
			break;
		default:
			break;
		}
	}
}

void mms_cycle_down()
{
	if(m_svar1==0)
	{
		m_svar1 = M_MENU_ENTRIES;
	}
	else
		m_svar1--;

	lcd_cpos(0);


	printf(m_top_menu[m_svar1].label);

}


//***************
void mms_execute()
{
	void *(fcptr)();

	fcptr = m_top_menu[m_svar1].fptr;
	m_svar1 = 0;
	fcptr();
}

//***************
inline void mms_exit()
{
	m_timer = 0;
	m_end();
}
//***************************
// M   D I G I T
//
// select frequency digit to alter using up/down
//
// Stack depth on entry: 2
//
char m_digit(void)
{
	if(!m_timer_en)
	{
		save_dbuf();
	}
	m_reset_timer();

	if(m_digit_editor(4,3,0))
	{
		long f;

		f = atol(f_in_buf);
		f *= 1000;
		frq_update(&f);

		taskYIELD();
		m_state = IDLE;
		m_timer = 0;	// wait 800 ms before reverting to frequency
	}
}
