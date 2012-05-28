/*
 * macros.h
 *
 *  Created on: 26.05.2012
 *      Author: Felix Erckenbrecht
 */
#ifndef MACROS_H_
#define MACROS_H_

#include "regmem.h"

#define EZA9

//#define BAND_2M
#define BAND_70CM

// Timing
// Frequency of crystal in EVA5
#define XTAL F_CPU
// no scaling
#define SYSCLK XTAL
#define MENUTIMEOUT   40	// 4 sek Eingabetimeout
#define PLLCHKTIMEOUT 2		// 200ms Timeout für PLL Check
#define PTT_DEBOUNCE_VAL 20
#define TX_TO_RX_TIME 5		// 5 ms TX -> RX Umschaltung
#define RX_TO_TX_TIME 5  	// 5 ms RX -> TX Umschaltung

//************************
// Frequenzkram
//
#ifdef BAND_70CM
#define FBASE 430000000         // lowest frequency (for eeprom storage) = 140MHz (430 MHz with 70 cm)
#define FBASE_MEM_RECALL 400000000
#define FDEF  433500000         // Default Frequency
#define PRESCALER   127         // PLL Prescaler (40 f�r 2m, 127 f�r 70cm)
#endif

#ifdef BAND_2M
#define FBASE 140000000         // lowest frequency (for eeprom storage) = 140MHz (430 MHz with 70 cm)
#define FBASE_MEM_RECALL 140000000
#define FDEF  145500000         // Default Frequency
#define PRESCALER    40         // PLL Prescaler (40 f�r 2m, 127 f�r 70cm)
#endif

#if !defined BAND_2M && !defined BAND_70CM
#error No frequency band defined, define one in macros.h
#endif

//
#define RXZF   21400000         // 21,4 MHz IF (RX VCO has to be 21,4MHz below RX frequency)
#define FREF   14400000         // 14,4 MHz reference frequency
#define FOFF0         0         // Offset0
#define FOFF06  0600000         // Offset1
#define FOFF76  7600000         // Offset1
#define FSTEP     12500         // Schrittweite !> 3,5 kHz f�r f<458,3MHz ( mu� gr��er sein als Frequenz/(Vorteiler*1023) )
//
#define PLLREF FREF/FSTEP
#define PLLLOCKWAIT 200         // Maximale Wartezeit in ms f�r PLL um einzurasten

//#define FSTEP      6250        // Schrittweite !> 3,5 kHz f�r f<458,3MHz ( mu� gr��er sein als Frequenz/(128*1023) )
//#define PLLREF     1152
//#define PLLREF     2304
//
//************************
// Squelch
//
#define SQL_HYST   10           // define squelch hysteresis in 5 ms steps
//
#define LCDDELAY  42     		// 42 ms

//************************
//Clock toggle
#define SBUS_CT \
	    SR_CLKDDR |= SR_CLKBIT;  \
	    SR_CLKPORT&= ~SR_CLKBIT; \
		SR_CLKDDR &= SR_CLKBIT;  \
		SR_CLKPORT|= SR_CLKBIT;

// Clock Pin auf Ausgang schalten
#define SBUS_CO \
		SR_CLKDDR |= SR_CLKBIT;
// Clock Pin auf Eingang schalten
#define SBUS_CI \
		SR_CLKDDR &= SR_CLKBIT;

// Clock Pin auf Eingang schalten, Pull-Up Widerstand zieht Leitung auf Hi
#define SBUS_CH SBUS_CI

// Clock Pin auf Ausgang schalten und auf 0 setzen
#define SBUS_CL \
	    SR_CLKDDR |= SR_CLKBIT;  \
	    SR_CLKPORT&= ~SR_CLKBIT;
;
#define SBUS_CTGL SB_CH \
		nop(); nop(); \
		SB_CL

// Data Pin auf Eingang setzen, ( High durch PullUp )
#define SBUS_DI \
		SR_DATADDR &= SR_DATABIT;

// Data Pin auf Ausgang setzen
#define SBUS_DO \
		SR_DATADDR |= SR_CLKBIT;
// Data Hi
#define SBUS_DH SBUS_DI

// Data Lo
#define SBUS_DL \
		SR_DATADDR  |= SR_DATABIT;  \
		SR_DATAPORT &= ~SR_DATABIT;

// Data & Clock Lo
#define SBUS_CDL \
		SBUS_CL\
		SBUS_DL

// Data & Clock Hi
#define SBUS_CDH \
		SBUS_CH \
		SBUS_DH

// **************************************************************
#define RED_LED       0x33
#define YEL_LED       0x31
#define GRN_LED       0x32
#define LED_OFF       0
#define LED_ON        4
#define LED_BLINK     8
#define LED_INVERT    128

#define ARROW         0x6D
#define A_OFF           0
#define A_ON            1
#define A_BLINK         2

//******************************************************************
#define P_SIGIN 1

//******************************************************************
// Blink Char
#define CHR_BLINK     0x80
// non printable chars
#define semikolon  0x3B
#define komma      0x2C
#define backslash  0x5C


//***********************
//
// Character Stuff
//
#define LCD_A     0x4A
#define LCD_ULINE 0x4B
#define LCD_SPACE 0x4C
//
//
// "segment type""pos hor""pos. vert""diagonal"
#define seg15left  0x4D
#define seg15right 0x4E
#define seg7       0x4E
#define segblink   0x10

#define seg15o   0x10
#define seg15lo  0x20
#define seg15lod 0x02
#define seg15lu  0x40
#define seg15u   0x01
#define seg15lud 0x08
#define seg15lm  0x04

#define seg15mo  0x20
#define seg15mu  0x40
#define seg15rod 0x04
#define seg15ro  0x01
#define seg15ru  0x02
#define seg15rud 0x10
#define seg15rm  0x08

#define seg7o  0x04
#define seg7m  0x08
#define seg7u  0x10
#define seg7lo 0x20
#define seg7lu 0x40
#define seg7ro 0x01
#define seg7ru 0x02

//4D - solid:
//5D - blink:
//Cursor bleibt stehen nach 0x4D/0x5D zur Ergänzung des Zeichens mit 0x4E/0x5E
//15Seg:
//    1010
//20 02 __ __ __
//20  02____  __
//  0404  ____
//40  08____  __
//40 08 __ __ __
//    0101

//4E - solid:
//5E - blink:
//Segment Codes

//7Seg:
//  04
//20  01
//  08
//40  02
//  10

//15Seg:
//     ____
//__ __ 20 04 01
//__  __2004  01
//  ____  0808
//__  __4010  02
//__ __ 40 10 02
//    ____
//


// Tastencodes - Dx = Taste unter Display, Nx=numerische Tasten
//
#define D1 0x01
#define D2 0x02
#define D3 0x03
#define D4 0x04
#define D5 0x05
#define D6 0x06
#define D7 0x07
#define D8 0x08
#define CLEAR 0x14
#define N1 0x11
#define N2 0x0D
#define N3 0x09
#define N4 0x12
#define N5 0x0E
#define N6 0x0A
#define N7 0x13
#define N8 0x0F
#define N9 0x0B
#define STERN 0x14
#define N0 0x10
#define RAUTE 0x0C

/*
 * ;****************************************************************************
;
;    MC70 - Firmware for the Motorola MC micro trunking radio
;           to use it as an Amateur-Radio transceiver
;
;    Copyright (C) 2004 - 2011  Felix Erckenbrecht, DG1YFE
;
;     This file is part of MC70.
;
;     MC70 is free software: you can redistribute it and/or modify
;     it under the terms of the GNU General Public License as published by
;     the Free Software Foundation, either version 3 of the License, or
;     (at your option) any later version.
;
;     MC70 is distributed in the hope that it will be useful,
;     but WITHOUT ANY WARRANTY; without even the implied warranty of
;     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;     GNU General Public License for more details.
;
;     You should have received a copy of the GNU General Public License
;     along with MC70.  If not, see <http://www.gnu.org/licenses/>.
;
;
;
;****************************************************************************
#DEFINE EVA5
;#DEFINE EVA9
;#DEFINE 2M
;#DEFINE 70CM

;******************************************************************
; Blink Char
#DEFINE CHR_BLINK     $80
; non printable chars
#DEFINE semikolon  $3B
#DEFINE komma      $2C
#DEFINE backslash  $5C


#DEFINE WAIT(ms)    pshx \ ldx  #ms \ jsr wait_ms \ pulx
#DEFINE LCDDELAY  42     ; 41ms

#DEFINE PCHAR(cmd)  ldaa #'c' \ ldab #cmd \ jsr putchar
#DEFINE PUTCHAR     ldaa #'c' \ jsr putchar
#DEFINE PINT(cmd)   ldaa #'u' \ ldab #cmd \ jsr putchar
#DEFINE PHEX(cmd)   ldaa #'x' \ ldab #cmd \ jsr putchar
#DEFINE PPLAIN(cmd) psha \ ldaa #'p' \ ldab #cmd \ jsr putchar \ pula

#DEFINE PRINTF(cmd) pshx \ ldx #cmd \ jsr printf \ pulx
; *******
; I 2 C
; *******
; Clock Toggle
#DEFINE I2C_CT oim #%100, Port2_Data \ aim #%11111011, Port2_Data
;
; Clock Pin auf Ausgang schalten
#DEFINE I2C_CO oim #%100, Port2_DDR_buf \ ldaa Port2_DDR_buf \ staa Port2_DDR \
;
; Clock In
; Clock Pin auf Eingang schalten
#DEFINE I2C_CI aim #%11111011, Port2_DDR_buf \ ldaa Port2_DDR_buf \ staa Port2_DDR
;
#DEFINE I2C_CIb aim #%11111011, Port2_DDR_buf \ ldab Port2_DDR_buf \ stab Port2_DDR
;
; Clock Pin auf Eingang schalten, Pull-Up Widerstand zieht Leitung auf Hi
#DEFINE I2C_CH I2C_CI
;
#DEFINE I2C_CHb I2C_CIb
;
; Clock Lo
; Clock Pin auf Ausgang schalten und auf 0 setzen
#DEFINE I2C_CL  aim #%11111011, Port2_Data \ oim #%100, Port2_DDR_buf \ ldaa Port2_DDR_buf \ staa Port2_DDR
;
#DEFINE I2C_CLb aim #%11111011, Port2_Data \ oim #%100, Port2_DDR_buf \ ldab Port2_DDR_buf \ stab Port2_DDR
;
#DEFINE I2C_CTGL psha \ I2C_CH \ nop \ nop \ I2C_CL \ pula
; Data in
; Data Pin auf Eingang setzen, ( High durch PullUp )
#DEFINE I2C_DI aim #%11111101, Port2_DDR_buf \ ldaa Port2_DDR_buf \ staa Port2_DDR
; Data out
; Data Pin auf Ausgang setzen
#DEFINE I2C_DO oim #%10, Port2_DDR_buf \ ldaa Port2_DDR_buf \ staa Port2_DDR
; Data Hi
#DEFINE I2C_DH I2C_DI
; Data Lo
#DEFINE I2C_DL  aim #%11111101, Port2_Data \ oim #%10, Port2_DDR_buf \ ldaa Port2_DDR_buf \ staa Port2_DDR
;
#DEFINE I2C_DLb aim #%11111101, Port2_Data \ oim #%10, Port2_DDR_buf \ ldab Port2_DDR_buf \ stab Port2_DDR
; Data & Clock Lo
#DEFINE I2C_CDL oim #%110, Port2_DDR_buf \ ldaa Port2_DDR_buf \ staa Port2_DDR \ aim #%11111001, Port2_Data
; Data & Clock Hi
#DEFINE I2C_CDH aim #%11111001, Port2_DDR_buf \ ldaa Port2_DDR_buf \ staa Port2_DDR
;***********************
;
; Character Stuff
;
#DEFINE LCD_A     $4A
#DEFINE LCD_ULINE $4B
#DEFINE LCD_SPACE $4C
;
;
; "segment type""pos hor""pos. vert""diagonal"
#DEFINE seg15left  $4D
#DEFINE seg15right $4E
#DEFINE seg7       $4E
#DEFINE segblink   $10

#DEFINE seg15o   $10
#DEFINE seg15lo  $20
#DEFINE seg15lod $02
#DEFINE seg15lu  $40
#DEFINE seg15u   $01
#DEFINE seg15lud $08
#DEFINE seg15lm  $04

#DEFINE seg15mo  $20
#DEFINE seg15mu  $40
#DEFINE seg15rod $04
#DEFINE seg15ro  $01
#DEFINE seg15ru  $02
#DEFINE seg15rud $10
#DEFINE seg15rm  $08

#DEFINE seg7o  $04
#DEFINE seg7m  $08
#DEFINE seg7u  $10
#DEFINE seg7lo $20
#DEFINE seg7lu $40
#DEFINE seg7ro $01
#DEFINE seg7ru $02

;4D - solid:
;5D - blink:
;Cursor bleibt stehen nach $xD zur Erg�nzung des Zeichens mit $xE
;15Seg:
;    1010
;20 02 __ __ __
;20  02____  __
;  0404  ____
;40  08____  __
;40 08 __ __ __
;    0101

;4E - solid:
;5E - blink:
;Segment Codes

;7Seg:
;  04
;20  01
;  08
;40  02
;  10

;15Seg:
;     ____
;__ __ 20 04 01
;__  __2004  01
;  ____  0808
;__  __4010  02
;__ __ 40 10 02
;    ____
;

; Tastencodes - Dx = Taste unter Display, Nx=numerische Tasten
;
#DEFINE D1 $01
#DEFINE D2 $02
#DEFINE D3 $03
#DEFINE D4 $04
#DEFINE D5 $05
#DEFINE D6 $06
#DEFINE D7 $07
#DEFINE D8 $08
#DEFINE CLEAR $14
#DEFINE N1 $11
#DEFINE N2 $0D
#DEFINE N3 $09
#DEFINE N4 $12
#DEFINE N5 $0E
#DEFINE N6 $0A
#DEFINE N7 $13
#DEFINE N8 $0F
#DEFINE N9 $0B
#DEFINE STERN $14
#DEFINE N0 $10
#DEFINE RAUTE $0C

 *
 */

#endif /* MACROS_H_ */
