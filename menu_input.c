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
// Parameter: A - Niedrigstes/Erstes Digit  (Bit 0-3)
//                H�chstes/Letztes Digit (Bit 4-7)
//            B - Mode :  0 - Dezimal
//                        1 - Alphanumerisch
//                        2 - Alphabet
//
// Ergebnis : X - Zeiger auf 0-terminierten String (f_in_buffer)
//            A - Status :  0 - OK
//                          1 - Abbruch
//
// changed Regs : A,B,X
//
// required Stack Space : 7+Subroutines
//
// Stack depth on entry : 4
//
// 4 - first pos
// 3 - last pos
// 2 - lower limit
// 1 - upper limit
// 0 - current pos
#define MDE_FIRST_POS 4
#define MDE_LAST_POS  3
#define MDE_LOWER_LIM 2
#define MDE_UPPER_LIM 1
#define MDE_CUR_POS   0
//
// returns :	0 - abort / timeout
//				1 - enter, string in f_in_buf
//
char m_digit_editor(char lopos, char highpos, char mode)
{
	static signed currentpos = 0;
	char hival, loval;
	char buf;
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
	}

	if(currentpos<0)
	{
		currentpos = lopos;
	}

	lcd_chr_mode(1);

	while(m_timer && !exit)
	{
		sci_trans_cmd();
		pll_led(0);
		led_update();
		taskYIELD();
		if (!(sci_rx_m(&buf) & 0x80))
		{
			m_reset_timer();

			if(cfg_head == 2)
			{
				switch(buf)
				{
					case HD2_ENTER:
						mde_enter();
						break;
					case HD2_EXIT:
						exit=2;
					default:
						break;
				}
			}
			else
			{
				switch(buf)
				{
					case HD3_ENTER:
						mde_enter();
						exit = 1;
						break;
					case HD3_EXIT:
						exit = 2;
					default:
						break;
				}
			}

			switch(buf)
			{
				case KC_D1:
					mde_up(currentpos, loval, hival);
					break;
				case KC_D2:
					mde_down(currentpos, loval, hival);
					break;
				case KC_D6:
					mde_next(&currentpos, lopos, hipos);
					break;
				case KC_D3:
					mde_next(&currentpos, lopos, hipos);
					break;
				default:
					break;
			}
		}
	}

	if(exit==2)
	{
		lcd_chr_mode(0);
		exit = 0;
	}

	return(exit);
}



inline void mde_up(uint8_t currentpos, char loval, char hival)
{
	if(dbuf[currentpos] == hival)
		dbuf[currentpos]=loval;
	else
		dbuf[currentpos]++;

	lcd_cpos(currentpos);
	pputchar('c',dbuf[currentpos] | 0x80,0);
}


inline void mde_down(uint8_t currentpos, char loval, char hival)
{
	if(dbuf[currentpos] == loval)
		dbuf[currentpos]=hival;
	else
		dbuf[currentpos]--;

	lcd_cpos(currentpos);
	pputchar('c',dbuf[currentpos] | 0x80,0);
}


//*************
inline void mde_next(uint8_t * currentpos, uint8_t lopos, uint8_t hipos)
{
	lcd_chr_mode(0);
	if(currentpos == lopos)
		currentpos = hipos;
	else
		currentpos++;

	lcd_chr_mode(currentpos);

}


//----------------
inline void mde_enter(uint8_t currentpos, const uint8_t lopos, const uint8_t hipos)
{
	uint8_t count;

	lcd_chr_mode(currentpos, 0);

	count=hipos - lopos + 1;
	memcpy(f_in_buf, &dbuf[lopos], count);
	f_in_buf[count] = 0;
}


