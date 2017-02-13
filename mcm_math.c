/*
 * math.c
 *
 *  Created on: 10.07.2012
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
#include "mcm_math.h"

#include <stdint.h>
#include <math.h>
#include <avr/pgmspace.h>
//
//***********************
// RAISE
//
// Potenziert 2 mit Parameter
//
// Parameter:
//            B - Exponent (0-7)
// Ergebnis:
//            B - Potenz (Bereich 1 - 128 / 2^0 - 2^7 )
//


long exp10_9 = 1000000000;
//           // Tabelle um 10 zu potenzieren - 32Bit Eintr�ge
long exp10tab[] = { 100000000,
				10000000,
				1000000,
				100000,
				10000,
				1000,
				100,
				10,
				1};


//************
// R A I S E
//************
//
// Potenziert 2 mit Parameter
//
// Parameter: B - Exponent (0-7)
//
// Ergebnis:  B - Potenz (Bereich 1 - 128, 2^0 - 2^7 )
//
//
uint8_t raise(uint8_t power)
{
	uint8_t buf = 1;

	buf <<= power;
	return buf;
}


/////////////////////////////////////////////////////////
// Convert a standard float to a short-float
/////////////////////////////////////////////////////////
ffp_t fp2sfp(float a)
{
	union u_tag{
		int32_t t;
		float f;
	    ffp_t fp;
		} temp ;
	ffp_t sfp_out ;
	temp.f = a ;
	if (fabs(a) < 1e-18)
	{
		temp.t = 0;
		return temp.fp;
	}
	else
	{
		// isolate and shift sign to bit 23
		sfp_out.sign =  ((uint32_t)temp.t>>31)<<7;
		// form exp by converting from excess 128 to excess 64
		// then shift to bits [30:24]
		sfp_out.exponent = (( (uint8_t) (temp.t>>23) & 0xff)-0x40);
		// form matissa by getting 15 bits from ieee mantissa
		// and prepending a 1. (which is implicit in ieee)
		sfp_out.significant = ((temp.t>>8) | 0x8000);

		return sfp_out ;
	}
}

/////////////////////////////////////////////////////////
// Convert a unsigned integer to a short-float
/////////////////////////////////////////////////////////
ffp_t uint2sfp(const uint16_t  i)
{
	union{
		ffp_t f;
		int8_t b[4];
		uint32_t l;
	}sfp_out;

	union{
		uint8_t b[2];
		int16_t sw;
		uint16_t uw;
	}j;


	if(i==0){
		sfp_out.l = 0;
	}
	else
	{
		j.uw=i;
		// normalize
		if(j.b[1] == 0){
			j.uw <<= 8;
			sfp_out.f.exponent = FFP_EXPONENT_BIAS-7;
		}
		else
		{
			sfp_out.f.exponent = FFP_EXPONENT_BIAS+1;
		}

		if((j.b[1] & 0xf0) == 0){
			j.uw <<= 4;
			sfp_out.f.exponent -= 4;
		}

		while(j.sw >= 0){
			j.uw <<= 1;
			sfp_out.f.exponent--;
		}
		sfp_out.f.sign = 0;
		sfp_out.f.significant = j.uw;
	}

	return sfp_out.f;
}

/////////////////////////////////////////////////////////
// Convert a signed integer to a short-float
/////////////////////////////////////////////////////////
ffp_t int2sfp(const int16_t  i)
{
	union{
		ffp_t f;
		int8_t b[4];
		uint32_t l;
	}sfp_out;

	union{
		uint8_t b[2];
		int16_t sw;
		uint16_t uw;
	}j;


	if(i==0){
		sfp_out.l = 0;
	}
	else
	{
		j.uw=i;
		// normalize
		if(j.b[1] == 0){
			j.uw <<= 8;
			sfp_out.f.exponent = 0x3f-8;
		}
		else
		{
			sfp_out.f.exponent = 0x3f;
		}

		if((j.b[1] & 0xf0) == 0){
			j.uw <<= 4;
			sfp_out.f.exponent -= 4;
		}

		while(j.sw >= 0){
			j.uw <<= 1;
			sfp_out.f.exponent--;
		}

		if(i < 0){
			sfp_out.f.sign = 1;
			sfp_out.f.significant = -j.sw;
		}
		else
		{
			sfp_out.f.sign = 0;
			sfp_out.f.significant = j.uw;
		}
	}

	return sfp_out.f ;
}

/////////////////////////////////////////////////////////
// Convert a short-float to a standard float
/////////////////////////////////////////////////////////
float sfp2fp(const ffp_t a)
{
	union u_tag{
		int32_t t;
		float f;
	} temp ;

	if (a.significant == 0)
	{
		return temp.f = 0;
	}
	//[14:0] of input go to [22:7]
	temp.t = (int32_t)(a.significant & 0x7fff)<<8 ;
	// [14:8] of input + 0x40 go to [30:23]
	temp.t |= ((int32_t)(a.exponent & 0x7f)+0x40)<<23 ;
	// [23] of input goes to [31]
	temp.t |= ((int32_t)(a.sign & 0x80)>>7) <<31 ;

	return temp.f ;
}

/*
 * Fast FP multiplication in C
 *
 */
ffp_t ffp_mul(const ffp_t f1, const ffp_t f2){
/*
 *
 */
	register union {
		uint32_t lng;
		ffp_t ffp;
	} t;

	register union {
		int8_t  b[4];
		uint16_t w[2];
		uint32_t l;
	} p;

	// Sign for result
	t.ffp.sign = f1.sign ^ f2.sign;

	t.ffp.exponent = f1.exponent + f2.exponent;
	if(t.ffp.exponent <= FFP_EXPONENT_BIAS){
		// check for underflow
		// return 0 on underflow
		t.lng = 0;
		return t.ffp;
	}

	t.ffp.exponent -= FFP_EXPONENT_BIAS; // subtract offset

	// build mantissa
	p.l = f1.significant * f2.significant;
	// ignore lower word
	// MultiU16X16toH16(t.significant, f1.significant, f2.significant);
	// left shift result until result is normalized
	//while(!(*((uint8_t*)&t.significant) & 0x80)){
	if(p.b[3] >= 0){
		p.l<<=1;
		t.ffp.exponent--;
	}

	t.ffp.significant = p.w[1];
	return t.ffp;
}


ffp_t ffp_square(const ffp_t f){
/*
 * Squaring:
 *  Exponent is doubled
 *
 *  xx = x * x
 *     = (n * 2^b) * (n * 2^b)
 *     = n * n * 2^b * 2^b
 *     = n * n * 2^(b+b)
 *     = n * n * 2^(2*b)
 *
 *     n -> MSB always set
 *     range: (2^(m+1))-1 .. 2^m
 *     		  (2^p)-1
 *     squared range:
 *     	(2^p-1)^2 .. 2^(2*m)
 *      (a  -b)^2 = a^2 - 2*a*b + b^2
 *      2^(p*2) - 2^(p+1) + 1
 *      2^(2*m+2) - 2^(m+2) + 1
 *      2^(2*15+2) - 2^17 + 1
 *      2^32 - 2^17 + 1 .. 2^30
 *      -> check if MSB is set, if not, shift left an decrease exp
 *
 *
 */
	register union {
		uint32_t lng;
		ffp_t ffp;
	} t;

	register union {
		int8_t  b[4];
		uint16_t w[2];
		uint32_t l;
	} p;

	if(f.exponent <= (FFP_EXPONENT_BIAS/2)){
		// check for underflow
		// return 0 on underflow
		t.lng = 0;
		return t.ffp;
	}

	// Sign for result
	t.ffp.sign = 0;

	t.ffp.exponent = f.exponent<<1;

	t.ffp.exponent -= FFP_EXPONENT_BIAS; // subtract offset

	// build mantissa
	p.l = f.significant * f.significant;
	// normalize result (ensure MSB = 1)
	if (p.b[FFP_SIGNIFICANT_HIGHBYTE] >= 0){
		p.l <<= 1;
		t.ffp.exponent--;
	}

	t.ffp.significant = p.w[1];
	return t.ffp;
}


inline ffp_t ffp_neg(register ffp_t n){
	n.sign = -n.sign;
	return n;
}


inline ffp_t ffp_sub(ffp_t minuend, ffp_t subtrahend){
	return ffp_add(minuend, ffp_neg(subtrahend));
}


ffp_t ffp_add(ffp_t s1, ffp_t s2){
	register union {
		ffp_t ffp;
		uint32_t l;
	} t;

	if(s1.significant == 0)
		return s2;

	if(s2.significant == 0)
		return s1;

	// reorder so s2 > s1
	if(s1.exponent != s2.exponent){
		if(s1.exponent > s2.exponent){
			t.ffp = s1;
			s1 = s2;
			s2 = t.ffp;
		}
	}
	else{
		if(s1.significant > s2.significant){
			t.ffp = s1;
			s1 = s2;
			s2 = t.ffp;
		}
	}

	// shift smaller to right
	// if difference in exponents>15 just return bigger
	t.ffp.exponent = s2.exponent - s1.exponent;
	if(t.ffp.exponent > 15){
		return s2;
	}

	if(t.ffp.exponent){
		// try to speed things up
		if(t.ffp.exponent>=12){
			s1.significant>>=12;
			t.ffp.exponent-=12;
		}
		else
		if(t.ffp.exponent>=8){
			s1.significant>>=8;
			t.ffp.exponent-=8;
		}

		s1.significant >>= t.ffp.exponent;
	}

	if (s1.sign == s2.sign) {
		// add
		t.ffp.significant = s1.significant + s2.significant;
		if (t.ffp.significant < s1.significant) {
			t.ffp.significant >>= 1;
			t.ffp.significant |= 0x8000;
			s2.exponent++;
		}
	}
	else {
		t.ffp.significant = s2.significant - s1.significant;
		// normalize 0 <= difference <= 0.5
		// test for highbyte == 0 (need to shift left by 8)
		t.ffp.exponent = 15;
		if ( t.ffp.significant_u8[1] == 0) {
			t.ffp.significant <<= 8;
			s2.exponent -= 8;
			t.ffp.exponent = 7;
		}

		if ( t.ffp.significant_u8[1] < 16) {
			t.ffp.significant <<= 4;
			s2.exponent -= 4;
			t.ffp.exponent = 3;
		}
		// shift left until MSB is set, but at max t.exponent shifts
		while ( t.ffp.significant_i8[1] >= 0 ) {
			t.ffp.significant <<= 1;
			s2.exponent--;
			if (!t.ffp.exponent--) {
				// underflow - return 0;
				t.l = 0;
			}
		}
	}
	t.ffp.exponent = s2.exponent;
	t.ffp.sign = s2.sign;

	return t.ffp;
}

int8_t ffp_magnitude(ffp_t f){
	return (int8_t) f.exponent - FFP_EXPONENT_BIAS;
}


// Table contains:
// logb_tab[k] = log_2 (1 + 0.5^k) for k=1..8
// as 0:8 fixed point
const uint8_t logb_tab[] = {
		150, 82, 44, 22,
		11, 6, 3, 1, 1
};

// Table contains:
// 1 + 2^(-k) for k=1..8 in 8:8 fixed point notation
const uint16_t logb_muladd_tab[] = {
		0x180, 0x140, 0x120, 0x110,
		0x108, 0x104, 0x102, 0x101
};

// Log of Base 2 using BKM algorithm
//
// 1 <= Argument <= 4.768462058
// Result: Log_2 in 8:8 fixed point
//
// first iteration is omitted
// since ffp type guarantees:
// 0.5 <= significant < 1
//
//
int16_t ffp_logb(const ffp_t f){
	uint16_t x = 0x8000;
	uint16_t y = 0;
	uint8_t  k;

	// get integer part of result directly from exponent
	y += (int16_t) f.exponent << 8;
	// correct for excess bias and implicit doubling of mantissa
	y -= 1+FFP_EXPONENT_BIAS;

	// calculate fractional part of result
	for ( k = 0; k < 8; k++ )
	{
		register union{
			struct{
				uint8_t  lowbyte;
				uint16_t sw;
				uint8_t  overflow;
			} __attribute__((packed));
			uint32_t l;
		}z;

		z.l = x * logb_muladd_tab[k];

		if (!z.overflow && z.sw <=  f.significant)
		{
			x  = z.sw;
			y += logb_tab[k];
		}
	}

	return y;
}
