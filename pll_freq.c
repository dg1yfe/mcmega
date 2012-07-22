/*
 * pll_freq.c
 *
 *  Created on: 27.05.2012
 *      Author: F. Erckenbrecht
 */
#include <stdlib.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "display.h"

void set_rx_freq(unsigned long * freq);
void set_tx_freq(unsigned long * freq);
unsigned long frq_get_freq(void);

uint8_t freq_init_eep(void);
uint8_t freq_init_rom(void);


/*
;********************
; F R E Q   I N I T
;********************
;
; Frequenzeinstellungen setzen
;
; Versucht zun�chst Frequenzeinstellungen aus EEPROM zu laden,
; schl�gt dies fehl wird aus dem ROM initialisiert
; (in UI Task gelbe LED blinken lassen)
;
; Parameter    : none
;
; Return value : 0 = OK (Init aus EEPROM)
*/
uint8_t freq_init()
{
	uint8_t ret;

// Try to read frequency settings from eeprom
	ret=read_current(&frequency, &txshift, &offset);
//	if(ret)
	{
		// default frequency
		frequency = FDEF;
		// default tx shift 
		txshift = FTXOFF;
		// tx shift currently deactivated
		offset = FOFF0;
	}
	return ret;
}



//*****************
// P L L   I N I T
//*****************
//
// Initialisiert PLL
//
// Parameter: D - Kanalraster
//
// Ergebnis : A - Status: 0=OK
//                        1=Fehler (Kanalraster zu klein)
//
// changed Regs: A,B
//
char init_pll(unsigned int spacing)
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
	return(lock);
}


//*******************************
// P L L   S E T   C H A N N E L
//*******************************
//
// programmiert die PLL auf den in X:D gegebenen Kanal
//
// Parameter    : X:D - Kanal
//
// Ergebnis     : Nichts
//
// changed Regs : none
//
//
void pll_set_channel(unsigned long ch)
{
	ldiv_t divresult;

	channel = ch;
	divresult = ldiv(ch, PRESCALER);

	SetPLL(0, divresult.rem & 127, divresult.quot & 1023);
	SetPLL(1, 0, PLLREF);
}

//**************************
// P L L   S E T   F R E Q
//**************************
//
// programmiert die PLL auf Frequenz
//
// Parameter    : X - Zeiger auf Frequenz (32 Bit)
//
// Ergebnis     : Nichts
//
// changed Regs : none
//
//
// pll_set_freq
//                 jsr  frq_cv_freq_ch         // Frequenz in Kanal mit Schrittweite f_step umrechnen
//                                             // Kanal kommt in X:D
//                 jsr  pll_set_channel        // PLL programmieren
//
//                 rts

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
void set_rx_freq(unsigned long * freq)
{
	unsigned long f;
	ldiv_t divresult;

	f = *freq - RXZF;
	divresult = ldiv(f, FSTEP);
	f = divresult.quot;

	if(divresult.rem > (FSTEP>>1))
	{
		f++;
	}
	pll_set_channel(f);

	f = frq_get_freq();
	f += RXZF;
	frequency = f;
}

//**************************
// S E T   T X   F R E Q
//**************************
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
void set_tx_freq(unsigned long * freq)
{
	unsigned long f;
	ldiv_t divresult;

	f = *freq - offset;
	divresult = ldiv(f, FSTEP);
	f = divresult.quot;

	if(divresult.rem > (FSTEP>>1))
	{
		f++;
	}
	pll_set_channel(f);

	f = frq_get_freq();

	f += offset;
	frequency = f;
}


//**************************
// S E T   F R E Q
//**************************
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
void set_freq(unsigned long * freq)
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
unsigned long frq_cv_freq_ch(unsigned long * freq)
{
	ldiv_t divresult;
	unsigned long ch;

	divresult = ldiv(*freq, FSTEP);
	ch = divresult.quot;
	if(divresult.rem > (FSTEP>>1))
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
unsigned long frq_cv_ch_freq(unsigned long ch)
{
	return (ch * FSTEP);
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
unsigned long frq_get_freq(void)
{
	return((unsigned long)channel * FSTEP);
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
void frq_update(unsigned long *freq)
{
	// TODO: Replace with message
	ui_frequency = *freq;
	vTaskDelay(1);
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
	if(offset)
	{
		if(offset>0)
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
