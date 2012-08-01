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
#include "math.h"

static int16_t q[3];
static int16_t z[2];
uint16_t c;
static uint16_t N;

volatile uint8_t tone_detect;

volatile uint32_t samp_buf;
volatile uint8_t samp_count=0;
volatile long ge;

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
	 0, 670,  694,  719,  744,  770,  797,  825,  854,
	 885,  915,  948,  974,	1000, 1035, 1072, 1109,
	1148, 1188, 1230, 1273,	1318, 1365, 1413, 1462,
	1514, 1567, 1598, 1622, 1655, 1679, 1713, 1738,
	1773, 1799, 1835, 1862, 1899, 1928, 1966, 1995,
	2035, 2065, 2107, 2138, 2181, 2213, 2257, 2291,
	2336, 2371, 2418, 2455, 2503, 2541
};



// c = (2 - coeff) * 65536
uint16_t ctcss_coeff_tab[] PROGMEM =
{
		0, 32723, 32719, 32716, 32712, 32708, 32704, 32699, 32694, 32689,
		32683, 32677, 32672, 32667, 32660, 32652, 32644, 32635, 32625,
		32615, 32604, 32593, 32580, 32566, 32552, 32537, 32520, 32510,
		32502, 32492, 32484, 32472, 32463, 32451, 32441, 32428, 32418,
		32404, 32393, 32378, 32367, 32350, 32338, 32320, 32307, 32288,
		32274, 32255, 32239, 32218, 32201, 32179, 32161, 32137, 32118
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

/* fsr = 8000
 * fi = CTCSS Ton
 *
 * coeffi = 2 * cosine (2 * π * fi / fsr)
 *
 * 	Q0i = coeffi * Q1i - Q2i + x (n)
	Q2i = Q1i
	Q1i = Q0i (3)
 */

static inline void goertzel_reset(void)
{
	uint8_t i;

	for(i=0;i<3;i++)
	{
		q[i]=0;
	}
	N=4000;
}

void goertzel_init(uint8_t ctcss_index)
{
	c = pgm_read_word(&ctcss_coeff_tab[ctcss_index]);
	goertzel_reset();
	tone_detect = 0;
	start_Timer2();
}


static inline int8_t iir_tp270(int8_t xn) __attribute__ ((always_inline));
static inline int8_t iir_tp270(int8_t xn)
{
/*b0=1;
 b1=int32(-376);
 b2=1;

 a0=1;
 a1=int32(-464);
 a2=int32(217);
 *
 *
y(i)= x(i) + z0;
z0  = int32((x(i) * b1)/256) - int32(y(i)*a1/256) + z1;
z1  = x(i) - int32(y(i)*a2/256);
 *
 */
	int16_t y,t;

	y= xn + z[0];
	t = -376;		// b1
	MultiSU16X8toH16(z[0], t, xn);
	z[0] += z[1];
	t = -464;		// a1
	MultiS16X16toH16(t,y,t);
	z[0] += t;
	t = 217;
	MultiS16X16toH16(t,y,t);
	z[1] = xn - t;

	return (int8_t) (y>>8);
}


static inline void goertzel_process(int8_t xn) __attribute__ ((always_inline));
static inline void goertzel_process(int8_t xn)
{

	//q[0] = (( (long) c * (long) q[1]) >> 14) - q[2] + xn;
	MultiSU16X16toH16(q[0], q[1]<<2, c);
	q[0] = q[0] - q[2] + xn;
	q[2] = q[1];
	q[1] = q[0];
	N--;
}


static inline long goertzel_eval(void) __attribute__ ((always_inline));
static inline long goertzel_eval()
{
	signed long y, y2;

	//y = ((long)q[0] * (long)q[0]);
	SquareS16to32(y, q[0]);
//	y -=  (c * (long) q[0] * (long) q[2])>>14;
	MultiSU16X16toH16(y2, q[0]<<2, c);
	y -= y2;
	//y += (long)q[1] * (long)q[1];
	SquareS16to32(y2, q[1]);
	y += y2;

	if(y<0)
		y=0;
	return y;
}


void tone_decode_stop()
{
	c = 0;
	stop_Timer2();
}


uint8_t tone_decode()
{
	uint8_t i,j;
	uint32_t buf;

	i=0;
	while(samp_count && (i++ < 20))
	{
		taskENTER_CRITICAL();
		samp_count--;
		j=samp_count;
		buf = samp_buf;
		taskEXIT_CRITICAL();
		buf >>= j;

		j = (uint8_t) buf & 1;
		j = iir_tp270(j);

		goertzel_process(j);

		if(!N)
		{
			ge = goertzel_eval();
			if( ((char) (ge >> 24)) > 10 )
			{
				tone_detect=5;
			}
			else
			{
				j=tone_detect;
				if(j)
				{
					j--;
					tone_detect=j;
				}
			}
			goertzel_reset();
		}
	}
	return tone_detect;
}

