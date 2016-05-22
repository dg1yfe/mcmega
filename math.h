/*
 * math.h
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

#ifndef MATH_H_
#define MATH_H_

uint8_t raise(uint8_t power);

extern long exp10_9;
extern long exp10tab[];


struct S_fastfp{
	uint8_t  exponent;
	int8_t   sign;
	uint16_t significant;
}__attribute__((packed));

typedef struct S_fastfp ffp_t;
/*
 * FP definition from
 * https://people.ece.cornell.edu/land/courses/ece4760/Math/Floating_point/index.html
 *
 * "The floating format with 16 bits of mantissa,
 *  7 bits of exponent, and a sign bit,
 *  is stored in the space of a 32-bit long integer.
 *  This format gives a factor of 2.5-3 speed up in multiplication (over IEEE)
 *  and a speed up of about a factor of 1.3-4.0 for addition.
 *  The speed for the multiply is about 35 cycles.
 *  The speed for the add is 35-106 cycles.
 *  My short float operations do not support overflow, denorm, or infinity
 *   detection (but underflow is detected and the value set to zero).
 *
 *  This section will concentrate on numbers stored as 32-bit long ints.
 *  The lower 16 bits are the mantissa (more properly, significand).
 *  The mantissa value is considered a binary fraction with values
 *  0.5<=mantissa<1.0. The top 8 bits are the exponent, but the top bit is used
 *  for overflow during the calculation, so the exponent range is 0x00 to 0x7f,
 *  or about 10^-18 to 10^18.
 *  The sign bit is stored in the 23rd bit (high bit, 3rd byte).
 *  The high order bit of the significand is always one (unless the actual value
 *   is zero), because there are no denorms allowed.
 *   Typical numbers are shown below.
 *   Examples:
 *	Decimal Value	Short float Representation
 *	0.0				0x0000_0000
 *	1.0				0x3f00_8000
 * 	1.5				0x3f00_c000
 * 10000			0x4c00_9c40
 * 1.0001			0x3f00_8003
 * -1.0				0x3f80_8000
 * -1.5				0x3f80_c000
 * 1e-18			0x0300_9392
 * -1e-18			0x0380_9392
 *
 */

// intRes = intIn1 + intIn2
#define SaturatedAdd16(intIn1, intIn2) \
asm volatile ( \
"add %A0, %A1 \n\t" \
"adc %B0, %B1 \n\t" \
"brvc 0f \n\t" \
"ldi %B0, 0x7f \n\t" \
"ldi %A0, 0xff \n\t" \
"sbrc %B1, 7 \n\t" \
"adiw %0, 1 \n\t" \
"0: \n\t" \
: \
"=&w" (intIn1) \
: \
"a" (intIn1), \
"a" (intIn2) \
)

/*
asm volatile ( \
"add %A0, %A1 \n\t" \
"adc %B0, %B1 \n\t" \
"brvc 0f \n\t" \
"ldi %B0, 0x7f \n\t" \
"ldi %A0, 0xff \n\t" \
"sbrc %B1, 7 \n\t" \
"adiw %0, 1 \n\t" \
"0: \n\t" \
: \
"=&w" (intIn1) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
)
*/


// signed16 * signed16
// 21 cycles
#define SquareS16to32(longRes, intIn) \
asm volatile ( \
"clr r25 \n\t" \
"clr r26 \n\t" \
"mul %A1, %A1 \n\t" \
"movw %A0, r0 \n\t" \
"muls %B1, %B1 \n\t" \
"movw %C0, r0 \n\t" \
"mulsu %B1, %A1 \n\t" \
"rol r25 \n\t" \
"lsl r0 \n\t" \
"rol r1 \n\t" \
"sbc r26, r25 \n\t" \
"sub %D0, r26 \n\t" \
"add %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (longRes) \
: \
"a" (intIn) \
: \
"r25", \
"r26" \
)



// Public Domain Multiplication macros
// from https://github.com/rekka/avrmultiplication/

// longRes = intIn1 * intIn2
#define MultiU16X16to32(longRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"movw %A0, r0 \n\t" \
"mul %B1, %B2 \n\t" \
"movw %C0, r0 \n\t" \
"mul %B2, %A1 \n\t" \
"add %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
"mul %B1, %A2 \n\t" \
"add %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (longRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26" \
)

// intRes = intIn1 * intIn2 >> 16
// uses:
// r26 to store 0
// r27 to store the byte 1 of the 32bit result
#define MultiU16X16toH16(intRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
"mul %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
"mul %B2, %A1 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"mul %B1, %A2 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (intRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26" , "r27" \
)

// intRes = intIn1 * intIn2 >> 16 + round
// uses:
// r26 to store 0
// r27 to store the byte 1 of the 32bit result
// 21 cycles
#define MultiU16X16toH16Round(intRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
"mul %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
"mul %B2, %A1 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"mul %B1, %A2 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"lsl r27 \n\t" \
"adc %A0, r26 \n\t" \
"adc %B0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (intRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26" , "r27" \
)


// signed16 * signed16
// 22 cycles
#define MultiS16X16to32(longRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"movw %A0, r0 \n\t" \
"muls %B1, %B2 \n\t" \
"movw %C0, r0 \n\t" \
"mulsu %B2, %A1 \n\t" \
"sbc %D0, r26 \n\t" \
"add %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
"mulsu %B1, %A2 \n\t" \
"sbc %D0, r26 \n\t" \
"add %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (longRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26" \
)


// signed16 * signed 16 >> 16
#define MultiS16X16toH16(intRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
"muls %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
"mulsu %B2, %A1 \n\t" \
"sbc %B0, r26 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"mulsu %B1, %A2 \n\t" \
"sbc %B0, r26 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (intRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26", "r27" \
)

// multiplies a signed and unsigned 16 bit ints with a 32 bit result
#define MultiSU16X16to32(longRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"movw %A0, r0 \n\t" \
"mulsu %B1, %B2 \n\t" \
"movw %C0, r0 \n\t" \
"mul %B2, %A1 \n\t" \
"add %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
"mulsu %B1, %A2 \n\t" \
"sbc %D0, r26 \n\t" \
"add %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (longRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26" \
)

// multiplies signed x unsigned int and returns the highest 16 bits of the result
#define MultiSU16X16toH16(intRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
"mulsu %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
"mul %B2, %A1 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"mulsu %B1, %A2 \n\t" \
"sbc %B0, r26 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (intRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26", "r27" \
)

// multiplies signed x unsigned int and returns the highest 16 bits of the result
// rounds the result based on the MSB of the lower 16 bits
// 22 cycles
#define MultiSU16X16toH16Round(intRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
"mulsu %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
"mul %A1, %B2 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"mulsu %B1, %A2 \n\t" \
"sbc %B0, r26 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"lsl r27 \n\t" \
"adc %A0, r26 \n\t" \
"adc %B0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (intRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26", "r27" \
)

// multiplies a signed long X unsigned int
// stores the high 4 bytes of the result
// rounds the number up if the MSB of the 2 low bytes is set
// 47 cycles
#define MultiSU32X16toH32Round(longRes, longIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
\
\
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
\
"mul %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
\
"mulsu %D1, %B2 \n\t" \
"movw %C0, r0 \n\t" \
\
"mulsu %D1, %A2 \n\t" \
"sbc %D0, r26 \n\t" \
"add %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
\
\
"mul %B1, %A2 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"adc %C0, r26 \n\t" \
"adc %D0, r26 \n\t" \
\
"mul %A1, %B2 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"adc %C0, r26 \n\t" \
"adc %D0, r26 \n\t" \
\
"mul %C1, %A2 \n\t" \
"adc %A0, r0 \n\t" \
"adc %B0, r1 \n\t" \
"adc %C0, r26 \n\t" \
"adc %D0, r26 \n\t" \
\
"mul %C1, %B2 \n\t" \
"adc %B0, r0 \n\t" \
"adc %C0, r1 \n\t" \
"adc %D0, r26 \n\t" \
\
\
"lsl r27 \n\t" \
"adc %A0, r26 \n\t" \
"adc %B0, r26 \n\t" \
"adc %C0, r26 \n\t" \
"adc %D0, r26 \n\t" \
\
\
"clr r1 \n\t" \
: \
"=&r" (longRes) \
: \
"a" (longIn1), \
"a" (intIn2) \
: \
"r26","r27" \
)


// multiplies 16 bit X 8 bit
// stores lower 16 bits
#define MultiSU16X8toL16(intRes, int16In, int8In) \
asm volatile ( \
"mul %A1, %2 \n\t"\
"movw %A0, r0 \n\t"\
"mulsu %B1, %2 \n\t"\
"add %B0, r0 \n\t"\
"clr r1"\
: \
"=&r" (intRes) \
: \
"a" (int16In), \
"a" (int8In) \
)

// multiplies 16 bit number X 8 bit constant
// saves lower 16 bit
// 8 cycles
#define MultiSU16XConst8toL16(intRes, int16In, int8In) \
asm volatile ( \
"ldi r22, %2 \n\t"\
"mul %A1, r22 \n\t"\
"movw %A0, r0 \n\t"\
"mulsu %B1, r22 \n\t"\
"add %B0, r0 \n\t"\
"clr r1 \n\t"\
: \
"=&r" (intRes) \
: \
"a" (int16In), \
"M" (int8In) \
:\
"r22"\
)

// multiplies 16 bit number X 8 bit and stores 2 high bytes
#define MultiSU16X8toH16(intRes, int16In, int8In) \
asm volatile ( \
"clr r26 \n\t"\
"mulsu %B1, %A2 \n\t"\
"movw %A0, r0 \n\t"\
"mul %A1, %A2 \n\t"\
"add %A0, r1 \n\t"\
"adc %B0, r26 \n\t"\
"clr r1 \n\t"\
: \
"=&r" (intRes) \
: \
"a" (int16In), \
"a" (int8In) \
:\
"r26"\
)

// multiplies 16 bit signed number X 8 bit and stores 2 high bytes
// rounds the number based on the MSB of the lowest byte
#define MultiSU16X8toH16Round(intRes, int16In, int8In) \
asm volatile ( \
"clr r26 \n\t"\
"mulsu %B1, %A2 \n\t"\
"movw %A0, r0 \n\t"\
"mul %A1, %A2 \n\t"\
"add %A0, r1 \n\t"\
"adc %B0, r26 \n\t"\
"lsl r0 \n\t"\
"adc %A0, r26 \n\t"\
"adc %B0, r26 \n\t"\
"clr r1 \n\t"\
: \
"=&r" (intRes) \
: \
"a" (int16In), \
"a" (int8In) \
:\
"r26"\
)




#endif /* MATH_H_ */
