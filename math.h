/*
 * math.h
 *
 *  Created on: 10.07.2012
 *      Author: F. Erckenbrecht
 */

#ifndef MATH_H_
#define MATH_H_

uint8_t raise(uint8_t power);

extern long exp10_9;
extern long exp10tab[];


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
