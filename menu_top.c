/*
 * menu_top.c
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


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "pll_freq.h"
#include "display.h"
#include "menu.h"
#include "menu_input.h"
#include "menu_sub.h"
#include "menu_mem.h"
#include "subs.h"
#include "audio.h"
#include "config.h"

typedef struct
{
	char label[9];
	void (* fptr)(char key);
} T_MENUITEM;


void m_set_shift(void);
void mts_switch(char key);
void mts_print(void);
static inline void m_frq_up(void);
static inline void m_frq_down(void);
static inline void m_sql_switch(void);
void m_tone(void);
void m_txshift(char key);
void m_submenu(char key);
void m_test(char c);
void m_debug(uint8_t key);
static void m_dtmf_direct(uint8_t key);


const T_MENUITEM m_submenu_list[] PROGMEM =
{
		{ "MENU    ", m_recall_submenu},
		{ "RECALL  ", m_recall_submenu},
		{ "STORE   ", m_store_submenu},
		{ "TX CTCSS", m_ctcss_tx},
		{ "RX CTCSS", m_ctcss_rx},
		{ "DTMF    ", m_none},
		{ "POWER   ", m_power_submenu},
		{ "DEF CHAN", m_cfgsave_submenu},
		{ "VERSION ", m_version_submenu},
		{ "CALIBRAT", m_cal_submenu}
};
#define M_MENU_ENTRIES sizeof(m_submenu_list) / sizeof(T_MENUITEM)



//*****************************
// M E N U   I D L E
//*****************************
//
// Menu / IDLE Subroutines
//
// Main Menu / Top Level
//
//*******************************
// M   T O P
//
void m_top(uint8_t key)
{
//	void (*fptr)(uint8_t key);

	if(rxtx_state){
		// TX
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
			case KC_ENTER:
			case KC_EXIT:
				m_dtmf_direct(key);
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
				m_power();
				//m_none();
				//m_prnt_tc();
				break;
			case KC_D5:
				m_tone();
			break;
			case KC_D6:
				m_debug(key);
				if(config.controlHead == CONTROL_HEAD2)
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
				m_recall_submenu(KC_NONE);
				break;
			default:
				break;
		}
	}
	else
	{
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
			m_test(key);
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
			m_power();
			//m_none();
			//m_prnt_tc();
			break;
			case KC_D5:
			m_tone();
			break;
			case KC_D6:
			m_debug(key);
			if(config.controlHead == CONTROL_HEAD2)
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
			m_recall_submenu(KC_NONE);
			break;
			case KC_ENTER:
			m_submenu(key);
			break;
			default:
			break;
		}
	}
}

//*************
// M   M E N U
//
// Call Submenu
//
void m_submenu(char key)
{
	static uint8_t m_index;

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
			m_state = MENU_SELECT;
			m_index = 0;
			printf_P(m_submenu_list[0].label);
			break;
		}
		case MENU_SELECT:
		{
			switch(key)
			{
				case KC_D1:
				{
					m_index++;
					if(m_index == M_MENU_ENTRIES)
						m_index = 1;
					lcd_cpos(0);
					printf_P(m_submenu_list[m_index].label);
					break;
				}
				case KC_D2:
				{
					if((m_index | 1) == 1)
						m_index = M_MENU_ENTRIES;

					m_index--;
					lcd_cpos(0);
					printf_P(m_submenu_list[m_index].label);
					break;
				}
				case KC_ENTER:
				{	
					void (* fptr)(char key);
					// get function pointer
					fptr = (void *) pgm_read_word(&m_submenu_list[m_index].fptr);
					// call menu function for current entry
					fptr(KC_NONE);
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



//*******************************
// M   F R Q   U P
//
// Frequenz einen Kanal nach oben
//
static inline void m_frq_up()
{
	config.frequency += config.f_step;
	frq_update(&config.frequency);
	m_frq_prnt();
}

//*******************************
// M   F R Q   D O W N
//
// Frequenz einen Kanal nach unten
//
static inline void m_frq_down()
{
	config.frequency -= config.f_step;
	frq_update(&config.frequency);
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
static inline void m_sql_switch()
{
	uint8_t squelchMode;
	if (config.squelchMode == SQM_CARRIER)
	{
		squelchMode = SQM_OFF;
		arrow_set(2,0);
	}
	else
	{
		squelchMode = SQM_CARRIER;
		arrow_set(2,1);
	}
	cfgUpdate.cfgdata = (uint32_t) squelchMode;
	cfgUpdate.updateMask = CONFIG_UM_SQUELCHMODE;
	config_sendUpdate();
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
		tone_start_sel(1750);
	}
	tone_timer = 6;	// 600 ms
}

//**************************************
// M   T X   S H I F T
//
// Anzeige der aktuellen TX Shift
//
void m_txshift(char key)
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
		printf_P(m_offset);
		lcd_fill();
		vTaskDelay(100);
		mts_print();
	}
	else
		mts_switch(key);

}

inline void mts_print()
{
	int32_t shift;
	lcd_cpos(0);
	shift = -config.tx_shift;
	decout(PRINTSIGN | 5, 3, (char *)&shift);
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
void mts_switch(char key)
{
	m_reset_timer();
	switch(key)
	{
		case KC_EXIT:
			// invert sign of TX shift
			if(config.tx_shift)
			{
				cfgUpdate.cfgdata = -config.tx_shift;
				cfgUpdate.updateMask = CONFIG_UM_TXSHIFT;
				config_sendUpdate();
			}
			mts_print();
			break;
		case KC_D7:
			// toggle shift state on/off
			cfgUpdate.cfgdata = ~config.shift_active;
			cfgUpdate.updateMask = CONFIG_UM_SHIFTACTIVE;
			config_sendUpdate();
			mts_print();
			break;
		case KC_D6:
			m_state = TXSHIFT_DIGIT;
			m_digit_editor(0,0,0,-1);
			break;
		case KC_ENTER:
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
void mts_digit(char key)
{
	char res;

	m_reset_timer();

	if((res = m_digit_editor(key, 1,2,0)))
	{
		if(res>0)
		{
			long f;
			f = atoi(dbuf);
			f *= 1000;
			if(f)
			{
				cfgUpdate.cfgdata = f;
				cfgUpdate.updateMask = CONFIG_UM_TXSHIFT;
			}
			else{
				cfgUpdate.cfgdata = 0;
				cfgUpdate.updateMask = CONFIG_UM_SHIFTACTIVE;
			}
			config_sendUpdate();
			m_state = M_IDLE;
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
	uint8_t i;
	
	// if input was less than 3 digits,
	// append '0' to expand to 4 digits total	
	if(cpos<3)
	{
		for(i=cpos;i<4;i++)
		{
			f_in_buf[i]='0';
		}
	}
	else{
		i=cpos;
	}
	// terminate input / appended input
	f_in_buf[i]=0;
	f = atol(f_in_buf);
	f *= 1000;

	if(f){
		cfgUpdate.cfgdata = f;
		cfgUpdate.updateMask = CONFIG_UM_TXSHIFT;
	}
	else{
		// instead of allowing a shift = 0, deactivate the shift
		cfgUpdate.cfgdata = 0;
		cfgUpdate.updateMask = CONFIG_UM_SHIFTACTIVE;
	}
	config_sendUpdate();

	m_state = M_IDLE;
	m_timer = 8;	// wait 800 ms before reverting to frequency
	mts_print();
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
void m_test(char c)
{
	//tone_start_sel(1750);
	if(!m_timer_en)
	{
		m_state = TEST;
		save_dbuf();
	}
	m_reset_timer();

	if(c == KC_D1)
		m_timer = 0;
	else
	{	
		lcd_cpos(0);
		printf_P(PSTR("%04x"), ((long)ge)>>16);
		lcd_fill();	
	}

}

void m_tone_stop()
{
	tone_stop_sel();
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





//***************************
// M   D I G I T
//
// select frequency digit to alter using up/down
//
char m_freq_digit(char key)
{
	signed char res;

	m_reset_timer();

	if( (res = m_digit_editor(key, 4,3,0)) )
	{
		unsigned long f;

		if(res>0)
		{
			f = atol(f_in_buf);
			f *= 1000;
			frq_update(&f);
			taskYIELD();
			printf_P(m_ok_str);
			vTaskDelay(200);
			m_frq_prnt();
		}
		else
		{
			m_timer = 0;	// wait 800 ms before reverting to frequency
		}
	}
	return res;
}


void m_debug(uint8_t key)
{
	float g=0;
	static uint8_t sc=0;
	static uint8_t mode = 0;
	static uint16_t t = 0;

	if(!m_timer_en)
	{
		m_state = DEBUG;
		save_dbuf();
	}
	
	m_reset_timer();

	switch(key)
	{
		case KC_EXIT:
			m_timer = 0;
			break;
		case 1:
			mode = 1;
			break;
		case 2:
			mode = 2;
			break;
		case 3:
			mode = 3;
			break;
		case 4:
			mode = 4;
			break;
		case 5:
			goertzel_init(770);
			break;
		case 6:
			tone_start_sel(1500);
			break;
		case 7:
			tone_start_sel(240);
			break;
		case 8:
			tone_start_sel(1700);
			break;
		default:
			break;
	}

	if(tone_detect)
	{
		led_set(YEL_LED, LED_ON);
	}
	else
		led_set(YEL_LED, LED_OFF);
		
	sc=sc<samp_buf_count ? samp_buf_count : sc;
	
	if(tick_hms > t)
	{
		g=ge;
		t = tick_hms + 8;
		lcd_cpos(0);
		switch(mode)
		{
			default:
			case 1:
				printf_P(PSTR("%2.0f"), g);
				break;
			case 2:			
				printf_P(PSTR("%d"), tone_detect);
				break;
			case 3:
				printf_P(PSTR("%d"), sc);
				break;
			case 4:
				printf_P(PSTR("%+3d"),(samp_buf[0]-129)>>1);
				break;
		}
		lcd_fill();
	}	
}



void m_dtmf_direct(uint8_t key){
	uint16_t fx,fy;
	dtmf_key_to_frequency(key,&fx,&fy);
	// check if tone is still active
	if(tone_timer){
		// stop tone
		tone_stop_sel();
		// wait 40 ms (minimum DTMF pause time)
		vTaskDelay(40/portTICK_RATE_MS);
	}
	tone_timer = 4;
	dtone_start(fx,fy);
}
