/*
 * math.c
 *
 *  Created on: 10.07.2012
 *      Author: F. Erckenbrecht
 */
#include <stdint.h>

//****************************************************************************
//
//    MC 70    v1.0.1 - Firmware for Motorola mc micro trunking radio
//                      for use as an Amateur-Radio transceiver
//
//    Copyright (C) 2004 - 2007  Felix Erckenbrecht, DG1YFE
//
//    This program is free software// you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation// either version 2 of the License, or
//    any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY// without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program// if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//
//****************************************************************************
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
