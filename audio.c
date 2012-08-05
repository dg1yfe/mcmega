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


static int32_t q1,q2;
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
		0, 65445, 65439, 65432, 65424, 65416, 65408, 65398, 65389, 65378,
		65367, 65354, 65344, 65334, 65320, 65304, 65288, 65270, 65251,
		65230, 65209, 65185, 65160, 65133, 65104, 65073, 65040, 65021,
		65005, 64983, 64967, 64944, 64926, 64902, 64883, 64857, 64836,
		64808, 64786, 64756, 64733, 64701, 64676, 64641, 64614, 64577,
		64549, 64509, 64478, 64436, 64403, 64358, 64322, 64274, 64235
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
	q1=0;
	q2=0;
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
	int32_t q;
/*
 * q1 - 32 Bit 8 1.7 8 8
 *              9  23
 */

	// q =  c   *  q1   -   q2   +  xn;
	//q2 = q1;
	//q1 = q;
// multiply 1.15 * 9.15
// =       10.30 - 5 Byte
//          9.31 - 5 Byte
//
/*
 *  0 = q  (signed 9.23)     | 8   1.7   8 | 8
 *  1 = c  (unsigned 1.15)   |     1.7   8
 *  2 = q1 (signed 9.23)     | 8   1.7   8 | 8
 *
 *
 *q	 D		 C		 B		 A
 *   8       8       8       8       8
 * 	9..2 |1.0..7 | 8..15 |16..23 |24..31
 *     	 |       |       |       |
 *  						q1_0 *  c_0  ignored
 * 					q1_0 *  c_1
 *					q1_1 *  c_0
 *			q1_1 *  c_1
 *          q1_2 *  c_0
 *	q1_2 *  c_1
 *
 *
 */
asm volatile ( \
	"clr     r2      \n\t" \
	"fmulsu %D2, %B1 \n\t" \
	"movw   %C0,  r0 \n\t" \
	"fmul   %A2, %B1 \n\t" \
	"movw   %A0,  r0 \n\t" \
	"fmul   %B2, %A1 \n\t" \
	"add    %A0,  r0 \n\t" \
	"adc    %B0,  r1 \n\t" \
	"fmulsu %D2, %A1 \n\t" \
	"sbc    %D0,  r2 \n\t" \
	"add    %B0,  r0 \n\t" \
	"adc    %C0,  r1 \n\t" \
	"adc    %D0,  r2 \n\t" \
	"fmul   %B2, %B1 \n\t" \
	"add    %B0,  r0 \n\t" \
	"adc    %C0,  r1 \n\t" \
	"adc    %D0,  r2 \n\t" \
	"clr     r1      \n\t"
	: \
	"=&w" (q) \
	: \
	"a" (c), \
	"a" (q1) \
	: \
	  "r2" \
);

	q -= q2;
// q += xn<<8;
asm volatile ( \
	"add    %B0, %A1 \n\t" \
	"adc    %C0, __zero_reg__ \n\t" \
	"adc    %D0, __zero_reg__ \n\t" \
	: \
	"+w" (q) \
	: \
	"a" (xn) \
	: \
);

	q2 = q1;

	N--;
}


static inline long goertzel_eval(void) __attribute__ ((always_inline));
static inline long goertzel_eval()
{
	int32_t y;
	int32_t y2;
/*
 * magnitude^2 = q1^2 + q2^2 - q1 * q2 * c
 */
//	y -=  (c * (long) q1 * (long) q2)>>14;
	y = ((long)q1 * (long)q1);

/*
 *  9.23 * 9.23 = 18.46
 *  9.7  * 9.7  = 18.14
 */
asm volatile ( \
	"clr   r2 		\n\t" \
	"clr   r3		\n\t" \
	"mul   %A1, %A1 \n\t" \
	"movw  %A0, r0  \n\t" \
	"muls  %B1, %B1 \n\t" \
	"movw  %C0, r0  \n\t" \
	"mulsu %B1, %A1 \n\t" \
	"sbc   r3 , r2  \n\t" \
	"lsl   r0 		\n\t" \
	"rol   r1 		\n\t" \
	"rol   r3 		\n\t" \
	"sub %D0, r3 	\n\t" \
	"add %B0, r0 	\n\t" \
	"adc %C0, r1 	\n\t" \
	"adc %D0, r2 	\n\t" \
	"clr r1 \n\t" \
	: \
	"=&r" (y) \
	: \
	"a" ((int16_t) (q1>>16) ) \
	: \
	"r2", \
	"r3" \
);

asm volatile ( \
	"clr   r2 		\n\t" \
	"clr   r3		\n\t" \
	"mul   %A1, %A1 \n\t" \
	"movw  %A0, r0  \n\t" \
	"muls  %B1, %B1 \n\t" \
	"movw  %C0, r0  \n\t" \
	"mulsu %B1, %A1 \n\t" \
	"sbc   r3 , r2  \n\t" \
	"lsl   r0 		\n\t" \
	"rol   r1 		\n\t" \
	"rol   r3 		\n\t" \
	"sub %D0, r3 	\n\t" \
	"add %B0, r0 	\n\t" \
	"adc %C0, r1 	\n\t" \
	"adc %D0, r2 	\n\t" \
	"clr r1 \n\t" \
	: \
	"=&r" (y2) \
	: \
	"a" ( (int16_t) (q2>>16) ) \
	: \
	"r2", \
	"r3" \
);

	//SquareS16to32(y, (int16_t (q1>>12)) );
	//y += (long)q2 * (long)q2;
	//SquareS16to32(y2, (int16_t (q2>>12)));
	y += y2;

	MultiSU16X16toH16(y2, ((int16_t) (q1>>16)), c);
	MultiS16X16toH16(y2, y2, ((int16_t) (q2>>16)));
	// c is 2.14 , result was right shifted 16 bits
	// adjust by left shift twice
	// y2 <<=2;
	y -= y2;
	//y -=  ((c * (long) q1) >> 14) * (long) q2;


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
		//j = iir_tp270(j);

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

