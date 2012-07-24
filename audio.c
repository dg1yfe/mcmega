/*
 * audio.c
 *
 *  Created on: 24.07.2012
 *      Author: F. Erckenbrecht
 */

#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>

#include "FreeRTOS.h"
#include "task.h"

#include "regmem.h"
#include "io.h"
#include "int.h"

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
#define FS 8000

void tone_start_pl(unsigned int frequency)
{
	unsigned long p;
	ldiv_t divresult;

	p = frequency << 16;

	divresult = ldiv(p, FS*4);

	PL_phase_delta = divresult.quot>>16;
}


void tone_stop_pl()
{
	PL_phase_delta = 0;
}


void tone_start_sel(unsigned int frequency)
{
	unsigned long p;
	ldiv_t divresult;

	p = frequency << 16;

	divresult = ldiv(p, FS*4);

	SEL_phase_delta = divresult.quot>>16;
}


void tone_stop_sell()
{
	PL_phase_delta = 0;
}


void dtone_start(unsigned int frequency)
{
	unsigned long p;
	ldiv_t divresult;

	p = frequency << 16;

	divresult = ldiv(p, FS);

	PL_phase_delta = divresult.quot>>16;
}


void tone_stop_pl()
{
	PL_phase_delta = 0;
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
