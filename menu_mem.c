/*
 * menu_mem.c
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
#include <stdio.h>
#include <avr/pgmspace.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "menu.h"
#include "menu_mem.h"
#include "menu_input.h"
#include "eeprom.h"
#include "pll_freq.h"
#include "display.h"
#include "subs.h"

#define MEM_SLOT_MAX 25

const char m_recall_str[] PROGMEM = "RECALL";
const char m_store_str[] PROGMEM = "STORE";
const char m_err_eep_str[] PROGMEM = "Err Slot";

//
//
//
// Neues Vorgehen für Speicherwahl:
// Eingestellten Kanal speichern:
// HD2/HD3:
// - "Enter" für Menu
// - "Mem Button" für "Store" oder per Up/Down "Store" auswählen
// - Per Up/Down oder "Button"/Taste Mem Channel auswählen
// - Enter speichert
//
// Kanal laden:
// - "Mem Button" (oder Menu & per up/down "Recall" auswählen
// - "Mem Button" wählt Bank, Ziffer wird hinzugefügt
//   alternativ: Weiterschalten per up/down
// - Anzeige: Frequenz (wenn vorhanden Name und Frequenz im Wechsel) und Mem Channel (Pos 7/8)
// - Enter übernimmt Kanal
//
//
//**************************************
// M   R E C A L L
//
// Speicherbank für Frequenzspeicherplätze wählen
//
void m_recall_submenu(char key)
{
	if(!m_timer_en)
	{
		save_dbuf();
	}

	m_reset_timer();

	if(key == KC_NONE)
	{
		lcd_cpos(0);
		printf_P(m_recall_str);
		lcd_fill();
		vTaskDelay(100);
		m_state = MEM_SELECT;
	}

	m_show_slot();
}


void m_store_submenu(char key)
{
	if(!m_timer_en)
	{
		save_dbuf();
	}

	m_reset_timer();

	if(key == KC_NONE)
	{
		lcd_cpos(0);
		printf_P(m_store_str);
		lcd_fill();
		vTaskDelay(100);
		m_state = MEM_SELECT_STORE;
	}

	m_show_slot();
}


void m_show_slot()
{
	unsigned long f;

	lcd_cpos(0);
	eep_rd_ch_freq(mem_bank, &f);
	{
		f -= FBASE_MEM_RECALL;
		// print frequency, truncate last 3 digits
		decout(0, 3, (char *)&f);
		while(cpos<6)
			pputchar('c',' ',0);

		lcd_cpos(6);
		// print bank & slot
		pputchar('d',mem_bank,0);
		lcd_fill();
	}
}


void m_mem_select(char key)
{
	m_reset_timer();

	if(key<10)
	{
		char i;

		i = mem_bank;
		i %= 10;

		i+= key;

		if(i<MEM_SLOT_MAX)
		{
			mem_bank = i;
			m_show_slot();
		}
	}
	else
	{
		switch(key)
		{
			case KC_D8:
			{
				// increment to next bank of 10 if max slot no is not exceeded
				if(mem_bank < MEM_SLOT_MAX-10)
					mem_bank += 10;
				else
				// saturate at 24 if bank 1 was active but slot was >=5
				if(mem_bank < 20)
					mem_bank = 24;
				// wrap around if bank 2 was selected
				else
					mem_bank -= 20;

				m_show_slot();
				break;
			}
			case KC_ENTER:
			{
				char err = 0;
				// are we in recall or store mode?
				if(m_state == MEM_SELECT_STORE)
				{
					lcd_cpos(0);
					if(mem_bank >= MEM_SLOT_MAX)
						err = 1;
					else
					{	// TODO: So richtig?
						lcd_cpos(0);
						if(store_ieep_ch(mem_bank))
						{
							printf_P(m_failed_str);
						}
						else
						{
							printf_P(m_stored_str);
						}
						lcd_fill();
						vTaskDelay(200);
					}
					m_timer = 0;
				}
				else
				{
					unsigned long f;
					if(eep_rd_ch_freq(mem_bank, &f))
					{
						err = 1;
					}
					else
					{
						frq_update(&f);
						lcd_clr(0);
						m_state = M_IDLE;

						printf_P(m_ok_str);
						vTaskDelay(200);
						m_frq_prnt();
					}
				}

				if(err)
				{
					lcd_cpos(0);
					printf_P(m_err_eep_str);
					lcd_fill();
					m_timer = 0;
				}
				break;
			}
			case KC_EXIT:
				m_timer = 0;
				break;
			case KC_D1:
				if(++mem_bank>=MEM_SLOT_MAX)
					mem_bank=0;
				m_show_slot();
				break;
			case KC_D2:
				if(!mem_bank)
					mem_bank=MEM_SLOT_MAX;

				mem_bank--;
				m_show_slot();
				break;
		}
	}
}


