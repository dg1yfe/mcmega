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
