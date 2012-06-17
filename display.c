/*
 * display.c
 *
 *  Created on: 28.05.2012
 *      Author: Felix Erckenbrecht
 */
#include "stdint.h"
#include "string.h"

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"

void lcd_clr(char clear_leds);
void lcd_cpos(unsigned char pos);


//***********************
// L C D _ H _ R E S E T
//***********************
//
// Hardware Reset des LCD
// (Initialisierung / Software Reset zus�tzlich n�tig)
//
void lcd_h_reset()
{
	sci_tx(0);	// EZA9 hat keinen HW Reset Ausgang f�r das LCD
	sci_tx(0);	// LCD per Kommando zur�cksetzen
}


//***********************
// L C D _ S _ R E S E T
//***********************
//
// LCD Warmstart - Reset Kommando an LCD Controller senden
//
char lcd_s_reset()
{
	char h;
	char c;
	char ret;

	lcd_timer = 0;
	lcd_timer_en = 1;
	// clear RX buffer
	while(sci_rx(NULL));
	h = tick_hms + 20; 	// 2 sek timeout

	ret = -1;
	do
	{
		if(check_inbuf() )
		{
			sci_read(&c);
			if(c == 0x7e)
			{
				// acknowledge LCD reset
				sci_tx(c);
				// set lcd_timer to long timeout
				lcd_timer = LCDDELAY*4;
				// clear LCD and LEDs
				lcd_clr(1);
				// return success;
				ret = 0;
				break;
			}
		}
		else
			taskYIELD();
	}while(h != tick_hms);

	return ret;
}

//*******************
// L C D _ C L R
//*******************
//
// L�scht Inhalt des Displays und des Buffers
//
//
// Parameter    : A - 1 = LEDs l�schen, 0 = nur Display
//
// Ergebnis     : none
//
// changed Regs : none
//
void lcd_clr(char clear_leds)
{
	pputchar('p',0x78,0);
	memset(dbuf,0,8);
	arrow_buf = 0;

	if (clear_leds)
	{
		pputchar('p',0x7a,0);
		led_dbuf = 0;
	}
}

//***************************
// L C D _ B A C K S P A C E
//***************************
void lcd_backspace()
{
	// decrease cursor position
	lcd_cpos(--cpos);
	// overwrite char with space
	pputchar('c',' ',0);
	// decrease cursor position again
	lcd_cpos(--cpos);
}


//*******************
// S A V E _ D B U F
//*******************
//
// Parameter : X - Zieladresse
//
void save_dbuf()
{
	// copy dbuf & cpos
	memcpy(dbuf2,dbuf,8);
	cpos2 = cpos;
}


//*************************
// R E S T O R E _ D B U F
//*************************
//
// Parameter : X - Quelladresse
//
void restore_dbuf()
{
	uint8_t i;

	// set LCD cursor to pos 0
	lcd_cpos(0);
	// restore LCD content
	for(i=0;i<8;i++)
	{
		pputchar('c', dbuf2[i], 0);
	}
	// restore cursor position
	lcd_cpos(cpos2);
}


//**********************************
// L C D   T I M E R   R E S E T
//**********************************
//
//  Parameter :
//
inline void lcd_timer_reset()
{
	lcd_timer = LCDDELAY;
}

//******************
// L C D   C P O S
//******************
//
//  Parameter : B - Cursorposition (0-7)
//
//  Ergebnis : none
//
//  changed Regs : none
//
void lcd_cpos(unsigned char pos)
{
	if(pos < 8)
	{
		if(pos!=cpos)
		{
			pputchar('p', pos + 0x60, 0 );
			cpos = pos;
		}
	}
}

//******************
// L C D   F I L L
//******************
//
//  Fills LCD from current Cursorposition until end with Spaces
//  Positions containing Space are ignored to gain Speed with the
//  slow Hitachi Display
//  -> simulates LCD Clear which would be slower in cases with less than
//  4 characters to clear
//
//  Parameter : none
//
//  Ergebnis : none
//
//  changed Regs : none
//
void lcd_fill()
{
	uint8_t pos;

	pos = cpos;
	for(;cpos < 8;cpos++)
	{
		pputchar('c',' ',0);
	}
	lcd_cpos(pos);
}
//****************
// L E D   S E T
//****************
//
// Setzt Bits in LED Buffer entsprechend Parameter
// Der Buffer wird zyklisch im UI Task abgefragt und eine �nderung
// an das Display ausgegeben.
// Achtung: Durch die langsame Kommunikation mit dem Display kann es
//          vorkommen, dass schnelle �nderungen nicht oder unvollst�ndig
//          dargestellt werden
//
//
// Parameter : B - LED + Status (RED_LED/YEL_LED/GRN_LED + OFF/ON/BLINK/INVERT)
//
//                 RED_LED $33 - 00110011
//                 YEL_LED $31 - 00110001
//                 GRN_LED $32 - 00110010
//                 OFF       0 - 00000000
//                 ON        4 - 00000100
//                 BLINK     8 - 00001000
//                 INVERT  128 - 10000000
//
//
// Returns : nothing
//
// changed Regs: A,B,X
//
void led_set(char led, char status)
{
#define LED_ALLOFF 0
#define LED_ALLON 0b00010101
#define LED_ALLBLINK 0b00101010
#define LED_BUFCHANGED 0x80
	char led_mask;
	char buf;
	/*
	 * char led_buf;	// Bit 0 (1)  - yellow
						// Bit 1 (2)  - yellow blink
						// Bit 2 (4)  - green
						// Bit 3 (8)  - green blink
						// Bit 4 (16) - red
						// Bit 5 (32) - red blink
						// Bit 6 (64) - unused
						// Bit 7 (128)- change flag
	 *
	 */
	buf = led_buf;

	// build bitmask depending on selected LED
	switch(led)
	{
	case RED_LED:
		led_mask = 0x30;
		break;
	case YEL_LED:
		led_mask = 0x03;
		break;
	case GRN_LED:
		led_mask = 0x0c;
	default:
		led_mask = 0;
	}

	// reset selected LED
	buf &= ~led_mask;

	if(status & LED_INVERT)
	{
		led_mask = (led_buf & led_mask) ^ LED_ALLON;
	}
	else
	{
		if(status & (LED_BLINK | LED_ON))
		{
			if(status & LED_ON)
			{
				led_mask &= LED_ALLON;
			}

			if(status & LED_BLINK)
			{
				led_mask &= (LED_ALLBLINK | LED_ALLON);
			}
		}
		else
		{
			led_mask = 0;
		}
	}

	buf |= led_mask;

	// propagate update only if something changed
	if(buf != led_dbuf)
		 buf |= LED_BUFCHANGED;

	led_buf = buf;
}

//**********************
// L E D   U P D A T E
//**********************
//
// Pr�ft LED Buffer auf Ver�nderung, steuert ggf. LEDs an
//
// Parameter : none
//
// Returns : nothing
//
// call from UI task only !
void led_update()
{
	char buf;
	char led;
	char mask;

	taskENTER_CRITICAL();
	buf = led_buf;
	led_buf &= ~LED_BUFCHANGED;
	taskEXIT_CRITICAL();

	led = YEL_LED;
	mask = 3;

	if(buf & LED_BUFCHANGED)
	{
		char diff;

		diff = led_dbuf ^ buf;
		for(led=YEL_LED; led <= RED_LED; led++)
		{
			// is there a difference in ON or BLINK state?
			if(diff & mask)
			{
				// check if 'ON' bit is set
				if(buf & mask & LED_ALLON)
				{
					// is LED on, but blinking?
					if(buf & mask & LED_ALLBLINK)
						// activate LED in blink mode
						pputchar('p', led | LED_BLINK, 0);
					else
						// activate LED non-blinking
						pputchar('p', led | LED_ON, 0);
				}
				else
				{
					// deactivate LED
					pputchar('p', led | LED_OFF, 0);
				}
			}
			// check difference for next LED
			mask <<= 2;
		}
		// store new value in led_dbuf but without "changed" bit
		led_dbuf = buf & ~LED_BUFCHANGED;
	}
}
//
//********************
// A R R O W   S E T
//********************
//
// Parameter : B - Nummer    (0-7)
//             A - Reset/Set/Blink
//                 0 = Reset,
//                 1 = Set
//                 2 = Blink
//                 3 = Invert (off->on->off, blink->off->blink, on->off->on)
//
// Returns : nothing
//
void arrow_set(char pos, char state)
{
	char cmd;
	uint16_t mask;
	char cursor_pos;

	cmd = 0;
	mask = (1<<pos);

	switch(state)
	{
	case 1:
		if(!(arrow_buf & mask))
		{
			cmd = ARROW + A_ON;
			// delete blink bit
			arrow_buf &= ~(mask << 8);
		}
		break;
	case 2:
		// if blink or on bit are 0
		if(!((arrow_buf & (mask<<8)) && (arrow_buf & mask)) )
		{	// set arrow to on
			cmd = ARROW + A_ON;
			// enable blink bit and on bit
			arrow_buf |= (mask << 8) | mask;
		}
		break;
	case 3:
		if(arrow_buf & mask)
		{
			cmd = ARROW + A_OFF;
			// delete on bit
			arrow_buf &= ~mask;
		}
		else
		{	// check if arrow was blinking
			if(arrow_buf & (mask << 8))
			{
				cmd = ARROW + A_BLINK;
			}
			else
			{
				cmd = ARROW + A_ON;

			}
			// re-enable on bit
			arrow_buf |= mask;
		}
		break;
	case 0:
	default:
		// check if arrow is on
		if(arrow_buf & mask)
		{
			cmd = ARROW + A_OFF;
			// delete blink bit and on bit
			arrow_buf &= ~((mask << 8) | mask);
		}
		break;
	}
	// remember current cursor position
	cursor_pos = cpos;
	// set position for arrow
	lcd_cpos(pos);
	pputchar('p',cmd,0);
	// restore cursor position
	lcd_cpos(cursor_pos);

	// TODO: Implement arrow setting like LED setting using set & update function
}

#define LCD_SOLID 0
#define LCD_BLINK 1
//*************************
// L C D   C H R   M O D E
//*************************
//
// Parameter : B - Position  (0-7)
//             A - Solid/Blink
//                 0 = Solid,
//                 1 = Blink
//
// Returns : nothing
//
// required Stack Space : 6+Subroutines
//
//
void lcd_chr_mode(uint8_t position, char mode)
{
	if (position < 8)
	{
		char c;

		c = dbuf[position];

		if(mode)
			c |= CHR_BLINK;
		else
			c &= ~CHR_BLINK;
		lcd_cpos(position);
		pputchar('c', c, 0);
	}
}

