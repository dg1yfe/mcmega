/*
 * menu_input.c
 *
 *  Created on: 02.06.2012
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
 *
 */

//*******************************
// M   S T A R T   I N P U T
//
// Frequenzeingabe �ber ZIffernfeld starten
//
//

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "regmem.h"
#include "macros.h"
#include "io.h"
#include "display.h"
#include "menu.h"
#include "menu_top.h"
#include "pll_freq.h"


void mde_up(uint8_t currentpos, char loval, char hival);
void mde_down(uint8_t currentpos, char loval, char hival);
void mde_next(uint8_t * currentpos, uint8_t lopos, uint8_t hipos);
void mde_enter(uint8_t currentpos, const uint8_t lopos, const uint8_t hipos);
void m_print(char key);
void m_start_input(char key);
void m_backspace (void);
void m_clr_displ (void);
void m_set_freq (void);
void m_set_freq_x(void);
void m_frq_prnt(void);


static inline void m_non_numeric(char key);


inline void m_start_input(char key)
{
	save_dbuf();
	lcd_clr(0);
	memset(f_in_buf,0,sizeof(f_in_buf));	// clear input buffer;
	m_print(key);
	m_state = F_IN;
}


inline void m_print(char key)
{
	m_reset_timer();
	f_in_buf[cpos] = key + 0x30;
	pputchar('c', key + 0x30, 0);

}

//**********************************
// M   F   I N
//
// Frequenzeingabe, Eingabe entgegennehmen
//
void m_f_in(char key)
{
	if(key < KC_NON_NUMERIC)
	{
		if(cpos < 8)
		{
			m_print(key);
		}
	}
	else
	{
		m_non_numeric(key);
	}
}



static inline void m_non_numeric(char key)
{
	switch(key)
	{
	case KC_EXIT:
	{
		m_backspace();
		break;
	}
	case KC_D4:
	{
		m_clr_displ();
		break;
	}
	case KC_D7:
	{
		m_set_shift();
		break;
	}
	case KC_ENTER:
	{
		m_set_freq();
		break;
	}
	default:
	{
		m_none(key);
		break;
	}
	}
}
//
//**********************************
// M   B A C K S P A C E
//
// eingegebenes Zeichen l�schen
//
void m_backspace ()
{
	m_reset_timer();
	lcd_backspace();
	f_in_buf[cpos]=0;
}

//**********************************
// M   C L R   D I S P L
//
// Display und Eingabe l�schen
//
void m_clr_displ ()
{
	m_reset_timer();
	lcd_clr(0);
	f_in_buf[0] = 0;
}

//*******************************
//
// M   S E T   F R E Q
//
// eingegebene Frequenz setzen
//
void m_set_freq ()
{
	uint8_t i;
	// append zeros
	for(i=cpos;i<6;i++)
	{
		f_in_buf[i] = '0';
	}
	f_in_buf[i] = 0;
	m_set_freq_x();
}


inline void m_set_freq_x()
{
	unsigned long f;
	f = atol(f_in_buf);
	// convert frequency input in kHz to Hz
	f *= 1000;
	frq_update(&f);
	lcd_clr(0);
	m_state = IDLE;

	printf_P(m_ok_str);
	vTaskDelay(200);
	m_frq_prnt();
}

inline void m_frq_prnt()
{
	lcd_cpos(0);
	freq_print(&frequency);
	freq_offset_print();
	lcd_fill();
	m_timer_en = 0;
	pll_timer = 0;
}

//*******************************
//
// M   D I G I T   E D I T O R
//
// Editiert einen Bereich im Display
//
// Parameter: lopos - Niedrigstes/Erstes Digit
//            hipos - H�chstes/Letztes Digit
//        	  Mode :  0 - Dezimal
//                    1 - Alphanumerisch
//                    2 - Alphabet
//					 -1 - Init
//
// Ergebnis : Status :  0 - continue editig
//                      1 - input ok (f_in_buf)
//					   -1 - abort
//
#define MDE_FIRST_POS 4
#define MDE_LAST_POS  3
#define MDE_LOWER_LIM 2
#define MDE_UPPER_LIM 1
#define MDE_CUR_POS   0
//
char m_digit_editor(char key, uint8_t lopos, char hipos, int8_t mode)
{
	static signed currentpos = 0;
	char hival = 0, loval = 0;
	char exit = 0;

	switch(mode)
	{
		case 0:
			hival = '9';
			loval = '0';
			break;
			// 0 - num
			// 1 - alphanum
			// 2 - alpha
		case 1:
			hival = 'z';
			loval = '0';
			break;
		case 2:
			hival = 'z';
			loval = 'a';
			break;
		case -1:
		{
			lcd_cpos(lopos);
			currentpos = lopos;
			// make character blink
			lcd_chr_mode(currentpos, 1);
			return 0;
		}
	}

	switch(key)
	{
		case KC_D1:
			if(dbuf[currentpos] >= hival)
				dbuf[currentpos]=loval;
			else
				dbuf[currentpos]++;

			lcd_cpos(currentpos);
			pputchar('c',dbuf[currentpos] | CHR_BLINK,0);
			break;
		case KC_D2:
			if(dbuf[currentpos] <= loval)
				dbuf[currentpos]=hival;
			else
				dbuf[currentpos]--;

			lcd_cpos(currentpos);
			pputchar('c',dbuf[currentpos] | CHR_BLINK,0);
			break;
		case KC_D6:
		case KC_D3:
			lcd_chr_mode(currentpos,0);
			if(currentpos <= lopos)
				currentpos = hipos;
			else
				currentpos++;
			lcd_chr_mode(currentpos,1);
			break;
		case KC_ENTER:
		{
			uint8_t count;

			lcd_chr_mode(currentpos, 0);

			count=hipos - lopos + 1;
			memcpy(f_in_buf, &dbuf[lopos], count);
			f_in_buf[count] = 0;
			exit = 1;
			break;
		}
		case KC_EXIT:
			lcd_chr_mode(currentpos, 0);
			exit=-1;
			break;
		default:
			break;
	}

	return(exit);
}
