/*
 * audio.c
 *
 *  Created on: 24.07.2012
 *      Author: F. Erckenbrecht
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "FreeRTOS.h"
#include "task.h"

#include "regmem.h"
#include "io.h"
#include "int.h"
#include "audio.h"

uint8_t sin_tab[] PROGMEM = {
	    8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10,
	   10, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 13, 13, 13,
	   13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	   15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	   15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	   14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13,
	   13, 13, 13, 12, 12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 10,
	   10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  8,  8,  8,  8,  8,
	    7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  6,  5,  5,  5,  5,  5,
	    5,  4,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,  2,  2,  2,
	    2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,
	    2,  2,  2,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  5,
	    5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7
};


uint16_t ctcss_tab[] PROGMEM =
{
	 670,  694,  719,  744,  770,  797,  825,  854,
	 885,  915,  948,  974,	1000, 1035, 1072, 1109,
	1148, 1188, 1230, 1273,	1318, 1365, 1413, 1462,
	1514, 1567, 1598, 1622, 1655, 1679, 1713, 1738,
	1773, 1799, 1835, 1862, 1899, 1928, 1966, 1995,
	2035, 2065, 2107, 2138, 2181, 2213, 2257, 2291,
	2336, 2371, 2418, 2455, 2503, 2541
};

/*
;**********************
; T O N E   S T A R T
;**********************
;
; Startet Ton Oszillator
;
; Parameter : D - Tonfrequenz in 1/10 Hz
;
;
; delta phase = f / 8000 * 64 * 256
; wegen Integer rechnung:
; dp = f*256*64 / 8000
; einfacher (da *65536 einem Shift um 2 ganze Bytes entspricht)
; dp = f*256*64*4 / (8000*4)
; dp = f*  65536  /  32000
;
; Frequenzabweichung maximal 8000 / (64 (Schritte in Sinus Tabelle) * 256 (8 Bit 'hinterm Komma')
; = 0,488 Hz
;
*/

/*
 * frequency in 1/10 Hz
 */
void tone_start_pl(unsigned int frequency)
{
	unsigned long p;
	ldiv_t divresult;

	p = (unsigned long) frequency << 16;

	divresult = ldiv(p, FS*40);

	PL_phase_delta = divresult.quot>>16;

	start_Timer2();
}


void tone_stop_pl()
{
	PL_phase_delta = 0;

	if(SEL_phase_delta==0)
	{
		stop_Timer2();
	}
}

/*
 * frequency in Hz
 */
void tone_start_sel(unsigned int frequency)
{
	unsigned long p;
	ldiv_t divresult;

	p = (unsigned long)frequency << 16;

	divresult = ldiv(p, FS*4);

	SEL_phase_delta = divresult.quot>>16;
	SEL_phase_delta2 = 0;

	start_Timer2();
}


void tone_stop_sel()
{
	taskENTER_CRITICAL();
	SEL_phase_delta2 = 0;
	SEL_phase_delta  = 0;
	taskEXIT_CRITICAL();
	SEL_phase=0;
	SEL_phase2=0;

	if(PL_phase_delta==0)
	{
		stop_Timer2();
	}
}


/*
 * frequency in Hz
 */
void dtone_start(unsigned int freq1, unsigned int freq2)
{
	unsigned long p1,p2;
	ldiv_t divresult;

	p1 = (unsigned long)freq1 << 16;
	divresult = ldiv(p1, FS);
	p1 = divresult.quot>>16;

	p2 = (unsigned long)freq2 << 16;
	divresult = ldiv(p2, FS);

	SEL_phase_delta = p1;
	SEL_phase_delta2 = divresult.quot>>16;

	start_Timer2();
}




/*

               ldd  #32000            ; Divisor  = Samplefrequenz * 4
;               ldd  #48000            ; Divisor  = Samplefrequenz * 4
               jsr  divide32          ; equivalent (Frequenz*256) / 16
               pulx
               pulx                   ; 'kleiner' (16 Bit) Quotient reicht aus

               std  osc1_pd           ; Quotient = delta f�r phase

               ldab Port6_DDR_buf
               andb #%10011111
               stab Port6_DDR_buf
               stab Port6_DDR

               ldab #0
               stab tasksw_en         ; disable preemptive task switching
               sei
               ldab tick_ms+1
tos_intloop
               cli
               nop                    ; don't remove these NOPs
               nop                    ; HD6303 needs at least 2 clock cycles between cli & sei
               sei                    ; otherwise interrupts aren't processed
               cmpb tick_ms+1
               beq  tos_intloop
               ldab #1
               stab oci_int_ctr       ; Interrupt counter auf 1
                                      ; (Bit is left shifted during Audio OCI, on zero 1ms OCI will be executed)
               ldab TCSR2
               ldd  OCR1
               std  OCR2
               subd #SYSCLK/1000
               addd #249*5            ; add 5 sample periods to ensure there is enough time
                                      ; before next interrupt occurs even on EVA9
               std  OCR1

               clra
               staa o2_en1
               staa o2_en2
               ldx  #OCI_OSC1
               stx  oci_vec           ; OCI Interrupt Vektor 'verbiegen'
                                      ; Ausgabe startet automatisch beim n�chsten OCI
                                      ; 1/8000 s Zeitintervall wird automatisch gesetzt
;               clr  tasksw_en         ; re-enable preemptive task switching
               ldd  #$1
               std  osc1_dither
               cli
               pulx
               pula
               pulb
               rts
 *
 */
