/*
 * menu_sub.c
 *
 *  Created on: 17.06.2012
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
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "menu.h"
#include "display.h"
#include "eeprom.h"
#include "subs.h"
#include "audio.h"
#include "ui.h"

void m_ctcss_submenu(char key);

// Small Submenus
//
//
//
//
//
//*******************************
// M   P O W E R
//
// set high or low power via menu

// Power Umschaltung
//
// ---------
// ONLY EVA9
// ---------
//
//
void m_power()
{
	if(cfg_pwr_mode)
	{
		// pwr is low, set to high
		rfpwr_set(1);
	}
	else
	{
		// pwr is high, set to low
		rfpwr_set(0);
	}
	rfpwr_print();
}



void m_power_submenu(char key)
{
	char print = 0;

	m_reset_timer();

	if(m_state != POWER_SELECT)
	{
		m_state = POWER_SELECT;
		print = 1;
	}
	else
	{
		switch(key)
		{
			case KC_D1:
			case KC_D2:
				cfg_pwr_mode ^= 8;
				print = 1;
				break;
			case KC_ENTER:
// TODO: Revert to initial value on "exit"
			case KC_EXIT:
				m_timer = 0;
				break;
			default:
				break;
		}
	}

	if(print)
	{
		lcd_cpos(0);
		if(cfg_pwr_mode)
		{
			printf_P(PSTR("pwr lo"));
			arrow_set(3, 0);
		}
		else
		{
			printf_P(PSTR("pwr hi"));
			arrow_set(3, 1);
		}

		lcd_fill();
	}
}

//**************************************
//
//

void m_defch_submenu(char key)
{
	static char index;
	char print = 1;

	m_reset_timer();

	if (m_state != DEFCH_SELECT)
	{
		index = cfg_defch_save & 2;
		m_state = DEFCH_SELECT;
	}
	else
	{
		switch(key)
		{
			case KC_D1:
			{
				index = index > 1 ? 0 : index+1;
				break;
			}
			case KC_D2:
			{
				index = index ? index-1 : 2;
				break;
			}
			case KC_ENTER:
			{
				print = 0;
				if(index)
				{
					uint8_t buf;

					taskENTER_CRITICAL();
					buf = cfg_defch_save;
					buf &= ~2;
					buf |= (index & 2);
					cfg_defch_save = buf;
					taskEXIT_CRITICAL();
					eeprom_update_byte((uint8_t *) 0x1fd, buf);
					lcd_cpos(0);
					printf_P(m_ok_str);
					lcd_fill();
					vTaskDelay(500);
					m_timer = 0;
				}
				else
				{
					char err;
					// Store current frequency and shift to EEPROM as power-up default
					if(!(err = store_current()))
					{
						lcd_cpos(0);
						printf_P(m_ok_str);
						lcd_fill();
						vTaskDelay(500);
						lcd_cpos(0);
						printf_P(m_stored_str);
					}
					else
					{
						lcd_cpos(0);
						printf_P(m_failed_str);
						lcd_fill();
						vTaskDelay(500);
						lcd_cpos(0);
						pputchar('x', err, 0);
					}
					lcd_fill();
					vTaskDelay(1000);
					m_timer = 0;
				}
				break;
			}
			case KC_EXIT:
			{
				print = 0;
				m_timer = 0;
				break;
			}
		}
	}

	if(print)
	{
		lcd_cpos(0);
		switch(index)
		{
		case 0: // store now
			printf_P(PSTR("STORE"));
			break;
		case 1: // manual
			printf_P(PSTR("MANUAL"));
			break;
		case 2: // automatic
			printf_P(PSTR("AUTO"));
			break;
		}
		lcd_fill();
	}

}


//**************************************
//
//
void m_version_submenu(char key)
{
	m_state=SHOW_VERSION;
	m_reset_timer();
	lcd_cpos(0);
	printf_P(version_str);
	lcd_fill();
	if(key == KC_EXIT)
		m_timer=0;
}



//**************************************
//
//
void m_ctcss_tx(char key)
{
	m_state = CTCSS_SEL_TX;
	m_ctcss_submenu(-1);
}


void m_ctcss_rx(char key)
{
	m_state = CTCSS_SEL_RX;
	m_ctcss_submenu(-1);
}


void m_ctcss_submenu(char key)
{
	char print = 1;

	m_reset_timer();

	if (key != -1)
	{
		switch(key)
		{
			case KC_D1:
			{
				ctcss_index = ctcss_index < CTCSS_TABMAX-1 ? ctcss_index+1 : 0;
				break;
			}
			case KC_D2:
			{
				ctcss_index = ctcss_index ? ctcss_index-1 : CTCSS_TABMAX-1;
				break;
			}
			case KC_ENTER:
			{
				print = 0;

				if(m_state == CTCSS_SEL_TX)
				{
					uint16_t freq;

					freq = pgm_read_word(&ctcss_tab[ctcss_index]);
					if(freq)
					{
						tone_start_pl(freq);
					}
					else
					{
						tone_stop_pl();
						tone_decode_stop();
						lcd_cpos(0);
						printf_P(PSTR("TX CTCSS"));
						lcd_fill();
						lcd_cpos(0);
						vTaskDelay(100);
						printf_P(PSTR("OFF"));
						lcd_fill();
					}
				}
				else
				{
					// CTCSS_SEL_RX
					if(ctcss_index)
					{
						uint16_t freq;

						freq = pgm_read_word(&ctcss_tab[ctcss_index]);
						goertzel_init(freq);
					}
					else
					{
						tone_decode_stop();
						lcd_cpos(0);
						printf_P(PSTR("RX CTCSS"));
						lcd_fill();
						lcd_cpos(0);
						vTaskDelay(100);
						printf_P(PSTR("OFF"));
						lcd_fill();
					}
				}

				lcd_cpos(0);
				printf_P(m_ok_str);
				lcd_fill();
				m_timer=0;
				break;
			}
			case 0:
			{
				tone_stop_pl();
				tone_decode_stop();
				lcd_cpos(0);
				printf_P(PSTR("TONE OFF"));
				lcd_fill();
				vTaskDelay(200);
			}
			case KC_EXIT:
			{
				print = 0;
				m_timer = 0;
				break;
			}
		}
	}

	if(print)
	{
		uint16_t freq;
		char c[6];

		memset(c,0,sizeof c);
		freq = pgm_read_word(&ctcss_tab[ctcss_index]);
		itoa(freq,c,10);
		lcd_cpos(0);
		if(freq==0)
		{
			printf_P(PSTR("OFF"));
		}
		else
		{
			if (freq<1000)
			{
				c[3] = c[2];
				c[2] = '_';
			}
			else
			{
				c[4] = c[3];
				c[3] = '_';
			}
			printf_P(PSTR("%s Hz"),c);
		}
		lcd_fill();
	}

}




void m_cal_submenu(char key)
{
	uint8_t cal;

	m_reset_timer();
	m_state=CAL;
	cal = OSCCAL;

	switch(key)
	{
		case KC_D1:
		{
			cal++;
			break;
		}
		case KC_D2:
		{
			cal--;
			break;
		}
		case KC_EXIT:
		{
			m_timer=0;
			break;
		}
	}
	OSCCAL = cal;
	lcd_cpos(0);
	pputchar('x',cal,0);
	lcd_fill();

}
