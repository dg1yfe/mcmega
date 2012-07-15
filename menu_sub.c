/*
 * menu_sub.c
 *
 *  Created on: 17.06.2012
 *      Author: F. Erckenbrecht
 */
#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "menu.h"
#include "display.h"
#include "eeprom.h"
#include "subs.h"

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
	if(pwr_mode)
	{
		// pwr is low, set to high
		pwr_mode &= 8;
		arrow_set(3, 1);
	}
	else
	{
		// pwr is high, set to low
		pwr_mode |= 8;
		arrow_set(3, 0);
	}
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
				pwr_mode ^= 8;
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
		if(pwr_mode)
		{
			printf_P(PSTR("LO PWR"));
			arrow_set(3, 0);
		}
		else
		{
			printf_P(PSTR("HI PWR"));
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
	char print = 0;

	m_reset_timer();

	if (m_state != DEFCH_SELECT)
	{
		index = cfg_defch_save & 2;
		m_state = DEFCH_SELECT;
		print = 1;
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
				if(index)
				{
					char buf;

					taskENTER_CRITICAL();
					buf = cfg_defch_save;
					buf &= ~2;
					buf |= (index & 2);
					cfg_defch_save = buf;
					taskEXIT_CRITICAL();
					if((buf = eep_write(0x1fd, buf)))
					{
						lcd_cpos(0);
						printf_P(m_failed_str);
						lcd_fill();
						vTaskDelay(500);
						lcd_cpos(0);
						pputchar('x', buf, 0);
						lcd_fill();
						vTaskDelay(1000);
						m_timer = 0;
					}
					else
					{
						lcd_cpos(0);
						printf_P(m_ok_str);
						lcd_fill();
						vTaskDelay(500);
						m_timer = 0;
					}
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
						lcd_fill();
						vTaskDelay(1000);
						m_timer = 0;
					}
					else
					{
						lcd_cpos(0);
						printf_P(m_failed_str);
						lcd_fill();
						vTaskDelay(500);
						lcd_cpos(0);
						pputchar('x', err, 0);
						lcd_fill();
						vTaskDelay(1000);
						m_timer = 0;
					}
				}
				break;
			}
			case KC_EXIT:
			{
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
	}

}


//**************************************
//
//
void m_version_submenu(char key)
{
	m_reset_timer();
	lcd_cpos(0);
	printf_P(PSTR("12.001"));
	lcd_fill();
}


