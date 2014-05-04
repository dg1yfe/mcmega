/*
 * audio.c
 *
 *  Created on: 24.07.2012
 *****************************************************************************
 *	MCmega -	Firmware for the Motorola MC micro radio
 *  to use it as an Amateur-Radio transceiver
 *
 * Copyright (C) 2013,2014 Felix Erckenbrecht, DG1YFE
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
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

#include "macros.h"
#include "regmem.h"
#include "io.h"
#include "int.h"
#include "audio.h"
//#include "math.h"

// 3.125 Hz resolution
// 0.32 s evaluation time
#define GOERTZEL_BLOCK 290

volatile float ge;
volatile float g_coeff;
static uint16_t g_block_ctr;
static uint16_t g_blocksize = GOERTZEL_BLOCK;

static float q1,q2;

volatile uint8_t tone_detect;
volatile uint8_t tone_detect_updated = 0;

volatile uint8_t samp_buf[SAMP_BUF_LEN];
volatile uint8_t samp_buf_count=0;
uint8_t samp_buf_r=0;

// globals for tone detection
int16_t cic_out;
int16_t cic_comb[4];
int16_t cic_int[4];
int16_t cic_mem[4];
uint8_t cic_ctr;




const uint8_t sin_tab[] PROGMEM = {
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


const uint16_t ctcss_tab[] PROGMEM =
{
	  -1,    0, 									// 'like tx/rx' , 'off'
	 670,  694,  719,  744,  770,  797,  825,  854,
	 885,  915,  948,  974,	1000, 1035, 1072, 1109,
	1148, 1188, 1230, 1273,	1318, 1365, 1413, 1462,
	1514, 1567, 1598, 1622, 1655, 1679, 1713, 1738,
	1773, 1799, 1835, 1862, 1899, 1928, 1966, 1995,
	2035, 2065, 2107, 2138, 2181, 2213, 2257, 2291,
	2336, 2371, 2418, 2455, 2503, 2541
};

#define DTMF_A 10
#define DTMF_B 11
#define DTMF_C 12
#define DTMF_D 13
#define DTMF_STAR 14
#define DTMF_HASH 15
const uint8_t dtmf_index_tab[] PROGMEM = 
{0x13, 0x00, 0x10,0x20,0x01,0x11,0x21,0x02,0x12,0x22,0x30,0x31,0x32,0x33,0x03,0x23};
//   0,   1,     2,   3,   4,   5,   6,   7,   8,   9,   A,   B,   C,   D,   *,   #

const uint16_t dtmf_tab_x[] PROGMEM = {1209, 1336, 1477, 1633 };
const uint16_t dtmf_tab_y[] PROGMEM = {697, 770, 852, 941};


void adc_init()
{
    ADMUX = (0 << REFS1) |      // AVCC external reference
			(1 << REFS0) |
			(1 << MUX1)  |
			(1 << MUX0)	 |		// ADC channel 3
			(1 << ADLAR);		// data is left adjusted
    
    ADCSRA = (1 << ADEN)  |
    (0 << ADSC)  |		
    (0 << ADIE)  |				// disable ADC interrupt
    (1 << ADPS2) |		
    (1 << ADPS1) |
    (0 << ADPS0);				// 125 kHz ADC input clock (F_CPU / 64)
    
    ADCSRB = 0;					// Free-Running mode
        
    ADCSRA |= (1 << ADSC);		// start first conversion now
	
}


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
; delta phase = f / 8000 * 256 * 256
; wegen Integer rechnung:
; dp = f*256*256 / 8000
; einfacher (da *65536 einem Shift um 2 ganze Bytes entspricht)
; fz = f*10 wegen 1/10 Hz, Kompensation durch Faktor 10 in Nenner
; dp = f*10 * 65536 / (8000 * 10)
; dp = fz  * 65536  /  80000
;
; Frequenzabweichung maximal 8000 / (256 (Schritte in Sinus Tabelle) * 256 (8 Bit 'hinterm Komma')
; = 0,122 Hz
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
	
	p+=40000;	// add 1/2 LSB to round instead of truncate

	divresult = ldiv(p, FS*10);

	PL_phase_delta = divresult.quot;

	start_Timer2();
}


/*
 *  Start or stop CTCSS tone by index
 */
void tone_start_pl_index(uint8_t index){
	uint16_t freq;
	if(index > CTCSS_INDEX_OFF && index < CTCSS_TABMAX){
		freq = pgm_read_word(&ctcss_tab[index]);
		tone_start_pl(freq);
	}
	else{
		tone_stop_pl();
	}
}


void tone_stop_pl()
{
	PL_phase_delta = 0;

	stop_Timer2();
}

/*
 * frequency in Hz
 */
void tone_start_sel(unsigned int frequency)
{
	unsigned long p;
	ldiv_t divresult;

	p = (unsigned long)frequency << 16;

	divresult = ldiv(p, FS);

	SEL_phase_delta = divresult.quot;
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

	stop_Timer2();
}



void attenuate_sel(char att)
{
	if(att)
	{
		SetShiftReg(SR_SELATT, 0xff);
	}
	else
	{
		SetShiftReg(0,~SR_SELATT);
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
	p1 = divresult.quot;

	p2 = (unsigned long)freq2 << 16;
	divresult = ldiv(p2, FS);

	SEL_phase_delta = p1;
	SEL_phase_delta2 = divresult.quot;

	start_Timer2();
}


void dtmf_key_to_frequency(uint8_t key, uint16_t * const freqx, uint16_t * const freqy){
	uint8_t pos;
	
	switch(key){
		case KC_HASH:
			key = DTMF_HASH;
			break;
		case KC_STAR:
			key = DTMF_STAR;
			break;
		case KC_D7:
			key = DTMF_A;
			break;
		case KC_D8:
			key = DTMF_B;
			break;
		case KC_D3:
			key = DTMF_C;
			break;
		case KC_D4:
			key = DTMF_D;
			break;
	}
	key &= 0x0f;	// restrict key index to range 0-15
	pos = pgm_read_byte(&dtmf_index_tab[key]);
	
	*freqx = pgm_read_word(&dtmf_tab_x[pos >> 4]);
	*freqy = pgm_read_word(&dtmf_tab_y[pos & 0x0f]);	
}
/*
; DTMF frequency matrix
;
;     1209  1336  1477  1633
;
; 697   1     2     3     A
;
; 770   4     5     6     B
;
; 852   7     8     9     C
;
; 941   *     0     #     D
;
*/


//******************************************************
// Decoder
//
/* fsr = 1000
 * fi = CTCSS Ton
 *
 * coeffi = 2 * cosine (2 * π * fi / fsr)
 *
 * 	Q0i = coeffi * Q1i - Q2i + x (n)
	Q2i = Q1i
	Q1i = Q0i (3)
	
	detection window size (bin size):
	    fsr / N
	-> 1000 / 200 =  5.0 Hz (0.2 s)
	   1000 / 400 =  2.5 Hz (0.4 s)
	   1000 / 100 = 10.0 Hz (0.1 s)
 */

static inline void goertzel_reset(uint16_t block_size)
{
	q1=0;
	q2=0;
	g_blocksize = block_size;
	g_block_ctr = block_size;
}


void tone_decoder_reset()
{
	taskENTER_CRITICAL();	
	goertzel_reset(GOERTZEL_BLOCK);
	tone_detect_updated = 0;
	taskEXIT_CRITICAL();
}

/*
	Calculates Goertzel filter coefficient
	Input: Frequency in 1/10 Hz (e.g. 770 for 77.0 Hz)
*/
void goertzel_init(uint16_t ctcss_freq)
{	//                 2 * pi * f_t / Fs
	//	Fs = 1 kHz -> 2/Fs = 0.002 * 1/10 Hz = 0.0002
	g_coeff = 2 * cos( 0.2F * M_PI * (float)ctcss_freq / (float)FS);
	goertzel_reset(GOERTZEL_BLOCK);
	tone_detect = 0;
	adc_init();
	start_Timer2();
}

void tone_decoder_start_index(uint8_t index){
	uint16_t freq;
	if(index > CTCSS_INDEX_OFF && index < CTCSS_TABMAX){
		freq = pgm_read_word(&ctcss_tab[index]);
		goertzel_init(freq);
	}
	else{
		tone_decoder_stop();
	}
}


uint8_t goertzel_process(float * s)
{
	float q0;

	q0 = g_coeff * q1 - q2 + *s;
	q2 = q1;
	q1 = q0;

	return (--g_block_ctr)? 0 : 1;
	
	// estimated CPU load:
	// 125 cycles per multiplication
	// 275 cycles per addition (75 - 275)
	//
	// cycles = 125 + 275 + 275
	//        = 675 cycles per goertzel sample
	// Fs_g = 1 kHz
	//
	// -> 675000 cycles / second
	// -> 8,4% CPU load @ 8 MHz
	//   13,8% CPU load @ 4.9 MHz
}


enum {GE_DETECTED, GE_NOTDETECTED, GE_UNCERTAIN};

uint8_t goertzel_eval()
{
	float p;
	int8_t po;

	p = square(q2) + square(q1) - g_coeff * q1 * q2;
	
	// normalize output with respect to blocksize
	p = 10*( log10(p) - 2*log10((float)g_blocksize));
	ge = p;
	po = (int8_t) p;

	if(po > 62)
		return GE_DETECTED;

	if(po < 55)
		return GE_NOTDETECTED;

	return GE_UNCERTAIN;

	// estimated CPU load:
	// 125 cycles per multiplication
	// 175 cycles per addition (75 - 275)
	//
	// cycles = 4*125 + 3*175
	//        = 1025 cycles per GOERTZEL_BLOCK samples
}


// CIC multirate filter
// perform decimation by factor 8
uint8_t cic(int8_t x)
{
	// integration part
	// performed every sample
	cic_int[0] += x;
	cic_int[1] += cic_int[0];
	cic_int[2] += cic_int[1];
	cic_int[3] += cic_int[2];

	if(--cic_ctr == 0)
	{
		cic_ctr = 8;
		// decimation
		// performed at Fs/8
		cic_comb[0] = cic_int[3] - cic_mem[0];
		cic_comb[1] = cic_comb[0]   - cic_mem[1];
		cic_comb[2] = cic_comb[1]   - cic_mem[2];
		cic_comb[3] = cic_comb[2]   - cic_mem[3];

		// update storage
		cic_mem[0] = cic_int[3];
		cic_mem[1] = cic_comb[0];
		cic_mem[2] = cic_comb[1];
		cic_mem[3] = cic_comb[2];
		return 1;
	}
	return 0;
	// estimated CPU load:
	// 6 cycles per addition
	// 2 cycles per move
	//
	// cycles = 4*6 + 1/8 * ( 4*6 + 4*2 )
	//        = 28 cycles per sample
	// Fs   = 8 kHz
	//
	// ->  28000 cycles / second
	// -> 0,4% CPU load @ 8 MHz
	//    0,6% CPU load @ 4.9 MHz
}

// Chebychev type 1 (passband ripple)
// 2. order
// f_g = 0.58 (normalized)
// ripple = 3 dB 
#define CHEB_COEF_A1 0.117433F
#define CHEB_COEF_A2 0.447211F
#define CHEB_COEF_GAIN 0.276920F
// 2nd order IIR low-pass-filter, Direct Form II
// implementation: chebychev low-pass as CIC amplitude compensation filter
void cheby(float * x)
{
	float t;
	static float cheb1=0;
	static float cheb2=0;

	t = *x - cheb1 * CHEB_COEF_A1 - cheb2 * CHEB_COEF_A2;
	*x = (t + cheb1 * 2 + cheb2) * CHEB_COEF_GAIN;
		
	cheb2 = cheb1;
	cheb1 = t;
	
	// estimated CPU load:
	// 125 cycles per multiplication
	// 175 cycles per addition (75 - 275)
	//
	// cycles = 4*125 + 4*175
	//        = 1200 cycles per sample
	// Fs_g = 1 kHz
	//
	// ->1200000 cycles / second
	// ->15,0% CPU load @ 8 MHz
	//   24,5% CPU load @ 4.9 MHz
}



void tone_decoder_stop()
{
	g_coeff = 0;
	stop_Timer2();
	tone_decoder_reset();
}



uint8_t tone_decode()
{
	uint8_t i;
	int8_t j;

	i=0;
	while(samp_buf_count && (i++ < 28))
	{
		int8_t buf;

		taskENTER_CRITICAL();
		// these operation has to be atomic
		j=samp_buf_count-1;
		samp_buf_count=j;
		taskEXIT_CRITICAL();

		buf = samp_buf[samp_buf_r++]-129;
		samp_buf_r &= SAMP_BUF_LEN-1;

		buf >>= 1;	// reduce to about 4 bit value

		// perform rate decimation from 8 kHz to 1 kHz
		// using a CIC filter (R=8, N=4, M=1)
		// maximum input bit-length = 4 bit !
		// ( CIC operates on 16 bit values and enlarges values by 12 bit)
		if(cic(buf))
		{
			// after rate conversion, this only runs at 1 kHz Samplerate
			// and give us the time to perform the calculations using
			// floating-point precision
			float s;
			
			// get output of CIC filter and cast to float
			s = (float) cic_comb[3];

			// apply chebychev low-pass for amplitude correction
			// This is optional, remove this to lower CPU load
			// detection of tones > 200 Hz might then be slightly degraded
			// cheby(&s);
			
			// process sample in goertzel
			if(goertzel_process(&s))
			{
				// start evaluation after an entire block of samples
				// has been processed
				j = tone_detect;
				switch(goertzel_eval())
				{
					// certain misdetection
					case GE_NOTDETECTED:
						j=0;
						break;
					// certain detection
					case GE_DETECTED:
						j = 8;
						break;
					// result has a great uncertainty, do nothing
					default:
						j = j>0 ? j-1 : 0;
						break;

				}
				tone_detect = j;
				tone_detect_updated = 1;
				goertzel_reset(GOERTZEL_BLOCK);			
			}		
		}
	}
	return tone_detect;
}

