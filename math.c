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
#include <stdint.h>
#include "math.h"
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


/*
 * Fast FP multiplication in C
 *
 */
ffp_t ffp_mult(ffp_t f1, ffp_t f2){
/*
 *
 */
	register ffp_t t;
	register uint32_t p;

	// Sign for result
	t.sign = f1.sign ^ f2.sign;

	t.exponent = f1.exponent + f2.exponent;
	if(t.exponent >= 63){
		// check for undeflow
		// return 0 on underflow
		*((uint32_t *) &t) = 0;
		return t;
	}

	t.exponent -= 62; // subtract offset

	// build mantissa
	p = f1.significant * f2.significant;
	// ignore lower word
	// MultiU16X16toH16(t.significant, f1.significant, f2.significant);
	// left shift result until result is normalized
	//while(!(*((uint8_t*)&t.significant) & 0x80)){
	while( ! (*((uint8_t *)&p+3) & 0x80)){
		p<<=1;
		t.exponent--;
	}
	t.significant = p>>16;
	return t;
}


ffp_t ffp_add(ffp_t s1, ffp_t s2){
	register ffp_t t;

	if(s1.significant == 0)
		return s2;

	if(s2.significant == 0)
		return s1;

	// reorder so s2 > s1
	if(s1.exponent != s2.exponent){
		if(s1.exponent > s2.exponent){
			t = s1;
			s1 = s2;
			s2 = t;
		}
	}
	else{
		if(s1.significant > s2.significant){
			t = s1;
			s1 = s2;
			s2 = t;
		}
	}

	// shift smaller to right
	// if difference in exponents>15 just return bigger
	t.exponent = s2.exponent - s1.exponent;
	if(t.exponent > 15){
		return s2;
	}

	// try to speed things up
	if(t.exponent>=12){
		s1.significant>>=12;
		t.exponent-=12;
	}
	else
	if(t.exponent>=8){
		s1.significant>>=8;
		t.exponent-=8;
	}

	s1.significant >>= t.exponent;

	if(s1.sign == s2.sign){
		// add
		t.significant = s1.significant + s2.significant;
		if(t.significant<s1.significant){
			t.significant>>=1;
			t.significant|=0x8000;
			s2.exponent++;
		}
	}
	else{
		t.significant = s2.significant - s1.significant;
		// normalize 0 <= difference <= 0.5
		// test for highbyte == 0 (need to shift left by 8)
		t.exponent=15;
		if( *((uint8_t *) &t.significant +1)){
			t.significant <<= 8;
			s2.exponent-=8;
			t.exponent=7;
		}
		if( *((uint8_t *) &t.significant +1)>=16){
			t.significant <<= 4;
			s2.exponent-=4;
			t.exponent=3;
		}
		// shift left until MSB is set, but at max t.exponent shifts
		while(!(*((uint8_t *) &t.significant +1) & 0x80)){
			t.significant <<= 1;
			s2.exponent--;
			if(!t.exponent--){
				// underflow - return 0;
				*((uint32_t *) &t) = 0;
			}
		}
	}
	t.exponent = s2.exponent;
	t.sign = s2.sign;

	return t;
}


