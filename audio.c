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
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

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

volatile uint32_t samp_buf;
volatile uint8_t samp_count=0;

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
	 0, 670,  694,  719,  744,  770,  797,  825,  854,
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
	p1 = divresult.quot>>16;

	p2 = (unsigned long)freq2 << 16;
	divresult = ldiv(p2, FS);

	SEL_phase_delta = p1;
	SEL_phase_delta2 = divresult.quot;

	start_Timer2();
}

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


/*
	Calculates Goertzel filter coefficient
	Input: Frequency in 1/10 Hz (e.g. 770 for 77.0 Hz)
*/
void goertzel_init(uint16_t ctcss_freq)
{	//                 2 * pi * f_t / Fs
	g_coeff = 2 * cos( 0.0002F * M_PI * (float)ctcss_freq );
	goertzel_reset(320);
	tone_detect = 0;
	start_Timer2();
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
	//   16,9% CPU load @ 4 MHz 
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

	if(po > 50)
		return GE_DETECTED;

	if(po < 40)
		return GE_NOTDETECTED;

	return GE_UNCERTAIN;

	// estimated CPU load:
	// 125 cycles per multiplication
	// 175 cycles per addition (75 - 275)
	//
	// cycles = 4*125 + 3*175
	//        = 1025 cycles
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
	//    0,7% CPU load @ 4 MHz 
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
// and additional alias rejection
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
	//   30,0% CPU load @ 4 MHz 
}



void tone_decode_stop()
{
	g_coeff = 0;
	stop_Timer2();
}



uint8_t tone_decode()
{
	uint8_t i;
	int8_t j;

	i=0;
	while(samp_count && (i++ < 28))
	{
		uint32_t buf;

		taskENTER_CRITICAL();
		samp_count--;
		j=samp_count;
		buf = samp_buf;
		taskEXIT_CRITICAL();
		buf >>= j;

		// convert to -1 / +1
		j = ((int8_t) buf & 1);

		// perform rate decimation from 8 kHz to 1 kHz
		// using a CIC filter (R=8, N=4, M=1)
		if(cic(j))
		{
			// after rate conversion, this only runs at 1 kHz Samplerate
			// and give us the time to perform the calculations using
			// floating-point precision
			float s;
			
			// get output of CIC filter
			s = (float) cic_comb[3];
			// apply chebychev low-pass for amplitude correction
			cheby(&s);
			
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
						j>>=1;
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
				goertzel_reset(GOERTZEL_BLOCK);			
			}		
		}
	}
	return tone_detect;
}

