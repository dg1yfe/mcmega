/*
 * pll_freq.c
 *
 *  Created on: 27.05.2012
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
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "display.h"
#include "subs.h"
#include "firmware.h"

void set_rx_freq(uint32_t * freq);
void set_tx_freq(uint32_t * freq);
unsigned long frq_get_freq(void);


//***************************************************
// P L L   S E T C H A N N E L S P A C I N G
//***************************************************
//
// Initializes PLL reference divider
// (the one which defines the channel spacing)
//
// Result : 0=OK
//          1=Error (Spacing too narrow / divider exceeds 14 bit)
//
uint8_t pll_setChannelSpacing(unsigned int spacing)
{
	ldiv_t divresult;

	divresult = ldiv(FREF, spacing);

	// REF divider is 14 Bits
	if(divresult.quot > 0x3fff)
		return 1;

	// set reference divider
	SetPLL(1, 0 , divresult.quot);

	rxtx_state = 0;
	ptt_debounce = 0;
	return 0;
}


//***************
// P L L   L E D
//***************
//
// �berpr�ft PLL Lock wenn PLL Timer abgelaufen ist
// aktiviert rote LED, wenn PLL nicht eingerastet ist
//
// Parameter    : Keine
//
// Ergebnis     : Nichts
//
// changed Regs : A,B
//
void pll_led(char force)
{
	if((pll_timer==0) || force)
	{
		char lock;

		pll_timer = PLLCHKTIMEOUT;

		lock = PIN_PLL_LOCK & BIT_PLL_LOCK;

		if((lock != pll_locked_flag) || force)
		{
			// show PLL status via red led
			if(!lock)
				led_set(RED_LED, LED_BLINK);
			else
				led_set(RED_LED, LED_OFF);
			// remember current status
			pll_locked_flag = lock;
		}
	}
}

//
//************************
// P L L   L O C K   C H K
//************************
//
// Liest PLL Lock Status von PLL IC ein
//
// Parameter : None
//
// Returns : B - Status
//                 0 = NOT locked
//               $40 = PLL locked
//
// changed Regs: B
//
char pll_lock_chk()
{
	char lock;

	lock = PIN_PLL_LOCK & BIT_PLL_LOCK ? 1 : 0;
	return lock;
}


//*******************************
// P L L   S E T   C H A N N E L
//*******************************
//
// programmiert die PLL auf den gegebenen Kanal
//
// Parameter    : Kanal
//
// Ergebnis     : Nichts
//
//
void pll_setChannel(unsigned long ch)
{
	ldiv_t divresult;

	channel = ch;
	divresult = ldiv(ch, PRESCALER);

	SetPLL(0, divresult.rem & 127, divresult.quot & 1023);
	SetPLL(1, 0, PLLREF);
}

//**************************
// S E T   R X   F R E Q
//**************************
//
// Setzt die Frequenz zum Empfang (Frequenz - 21,4MHz ZF)
//
// Parameter    : X - Zeiger auf Frequenz (32 Bit)
//
// Ergebnis     : Nichts
//
// changed Regs : none
//
//
void set_rx_freq(uint32_t * freq)
{
	unsigned long f;
	ldiv_t divresult;

	// subtract IF (intermediate frequency) from desired frequency
	f = (unsigned long) *freq - RXZF;
	divresult = ldiv(f, cconf.f_step);
	f = divresult.quot;

	if(divresult.rem > (cconf.f_step))
	{
		f++;
	}
	pll_setChannel(f);

	f = frq_get_freq();
	f += RXZF;
	cconf.frequency = f;
}

//**************************
// S E T   T X   F R E Q
//**************************
//
// Control Task
//
// Setzt die Frequenz zum Senden
//
// Parameter    : X - Zeiger auf Frequenz (32 Bit)
//
// Ergebnis     : Nichts
//
// changed Regs : none
//
//
void set_tx_freq(uint32_t * freq)
{
	unsigned long f;
	ldiv_t divresult;

	f = (unsigned long) *freq - cconf.active_tx_shift;
	divresult = ldiv(f, cconf.f_step);
	f = divresult.quot;

	if(divresult.rem > (cconf.f_step>>1))
	{
		f++;
	}
	pll_setChannel(f);

	f = frq_get_freq();

	f += cconf.active_tx_shift;
	cconf.frequency = f;
}


//**************************
// S E T   F R E Q
//**************************
// Control Task
//
// Setzt die Frequenz auf die X zeigt, pr�ft vorher ob gesendet oder empfangen wird
//
// Parameter    : X - Zeiger auf Frequenz (32 Bit)
//
// Ergebnis     : Nichts
//
// changed Regs : none
//
//
void set_freq(uint32_t * freq)
{
	if(rxtx_state)
		set_tx_freq(freq);
	else
		set_rx_freq(freq);
}

//******************************
// F R Q   C V   F R E Q   C H
//******************************
//
// Umrechnung Frequenz in Kanal
//
// Parameter    : X - Zeiger auf Frequenz DWord
//
// Ergebnis     : X:D - Kanal (Kanalabstand = f_step)
//
// changed Regs : D,X
//
//
unsigned long frq_cv_freq_ch(uint32_t * freq)
{
	ldiv_t divresult;
	unsigned long ch;

	divresult = ldiv((unsigned long) *freq, cconf.f_step);
	ch = divresult.quot;
	if(divresult.rem > (cconf.f_step>>1))
	{
		ch++;
	}
	return(ch);
}

//******************************
// F R Q   C V   C H   F R E Q
//******************************
//
// Umrechnung Kanal in Frequenz
//
//  Parameter    : X:D - Kanal
//
//  Ergebnis     : X:D - Frequenz des aktuell eingestellten Kanals
//
//  changed Regs : D,X
//
//
uint32_t frq_cv_ch_freq(unsigned long ch)
{
	return ( (uint32_t)ch * cconf.f_step);
}

//**************************
// F R Q   G E T   F R E Q
//**************************
//
//  Liefert die aktuell eingestellte Frequenz
//
//
//  Parameter    : none
//
//  Ergebnis     : X:D - Frequenz des aktuell eingestellten Kanals
//
//  changed Regs : D,X
//
//
uint32_t frq_get_freq(void)
{
	return((uint32_t)channel * cconf.f_step);
}

//***************************
// F R Q   C A L C   F R E Q
//***************************
//
// Umrechnung Eingabe -> Frequenz (long)
//
// Parameter    : D  - Adresse vom Eingabe-Buffer (Eingabe = Nullterminierter String)
//                X  - Adresse f�r Ergebnis (Frequenz, DWord)
//
// Ergebnis     : *X - 32Bit Frequenzwert
//
// changed Regs : A, B , X
//
// TODO: replace with atol
unsigned long frq_calc_freq(char * str)
{
	return(atol(str));
}


//**********************
// F R Q   U P D A T E
//**********************
//
// Funktion wird von UI Task aufgerufen. Teilt Control Task mit, dass eine neue Frequenz gesetzt wurde
//
// Parameter    : X - Zeiger auf Frequenzwort
//
// Returns      : Nothing
//
// changed Regs : None
//
void frq_update(uint32_t *freq)
{
	cfgUpdate.cfgdata.frequency = *freq;
	cfgUpdate.updateMask = CONFIG_UM_FREQUENCY;
	config_sendUpdate();
}

//********************
// F R E Q   P R I N T
//********************
//
// UI - Task
//
// Gibt Frequenz auf Display aus
//
// Parameter    : X - Zeiger auf Frequenzwort
//
// Returns      : Nothing
//
// changed Regs : None
//
void freq_print(unsigned long * freq)
{
	pputchar('l',3, (char * ) freq);
}

//
//********************************
// F R E Q   S E T   O F F S E T
//********************************
//
// UI - Task
//
// Gibt aktuelles Offset auf Display aus
//
// Parameter    : None
//
// Returns      : Nothing
//
// changed Regs : None
//


//
//***********************************
// F R E Q   O F F S E T   P R I N T
//***********************************
//
// UI - Task
//
// Gibt aktuelles Offset auf Display aus
//
// Parameter    : None
//
// Returns      : Nothing
//
// changed Regs : None
//
void freq_offset_print()
{
	if(cconf.active_tx_shift)
	{
		if(cconf.active_tx_shift>0)
			arrow_set(6,1);
		else
			arrow_set(6,2);
	}
	else
		arrow_set(6,0);
}

//
//**********************
// F R E Q   C H E C K
//**********************
//
// Control - Task
//
// Pr�ft auf �nderung der Frequenz und der TX Shift durch UI Task
// Setzt ggf. Frequenz und/oder Offset neu
//
// Parameter    : None
//
// Returns      : Nothing
//
// chanegd Regs : A, B, X
//
/*
 *  replaced by config_checkForUpdate()
void frq_check()
{
	// TODO: Set shift before frequency
	if(ui_frequency)
	{
		set_freq(&ui_frequency);
		ui_frequency = 0;
	}
	if(ui_txshift != -1)
	{
		offset = ui_txshift;
		ui_txshift = -1;
	}
}
*/
