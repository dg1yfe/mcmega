/*
 * menu_input.c
 *
 *  Created on: 02.06.2012
 *      Author: F. Erckenbrecht
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

#include "FreeRTOS.h"
#include "task.h"

#include "regmem.h"
#include "macros.h"
#include "io.h"
#include "display.h"


inline void mde_up(uint8_t currentpos, char loval, char hival);
inline void mde_down(uint8_t currentpos, char loval, char hival);
inline void mde_next(uint8_t * currentpos, uint8_t lopos, uint8_t hipos);
inline void mde_enter(uint8_t currentpos, const uint8_t lopos, const uint8_t hipos);


inline void m_start_input(char key)
{
	save_dbuf();
	lcd_clr(0);
	m_print(key);
}


inline void m_print(char key)
{
	m_reset_timer();
	m_state = F_IN;
	pputchar('c', key + 0x30, 0);
	f_in_buf[cpos] = key + 0x30;

}

//**********************************
// M   F   I N
//
// Frequenzeingabe, Eingabe entgegennehmen
//
inline void m_f_in(char key)
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

	}
}

inline m_non_numeric(char key)
{
	switch(key)
	{
	case 0:
	{
		m_backspace();
		break;
	}
	case 4:
	{
		m_clr_display();
		break;
	}
	case 7:
	{
		m_set_shift();
		break;
	}
	case 9:
	{
		m_set_freq();
		break;
	}
	default:
	{
		m_none();
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
	m_state=F_IN;
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
inline void m_set_freq ()
{
	f_in_buf[cpos] = 0;
	m_set_freq_x();
}


inline void m_set_freq_x()
{
	long f;
	f = atol(f_in_buf);
	frq_update(&f);
	lcd_clr(0);
	m_state = IDLE;

	printf(m_ok);
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
char m_digit_editor(char key, char lopos, char highpos, char mode)
{
	static signed currentpos = 0;
	char hival, loval;
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

	switch(buf)
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
