/*
 * io.c
 *
 *  Created on: 26.05.2012
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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"


#include "macros.h"
#include "regmem.h"
#include "firmware.h"
#include "display.h"


// serial bus maximum frequency in kHz
// the maximum rate is limited by RC low-pass-filters
// towards PLL: R = 12k7, C = (68 pF + 10 pF) -> Tau = 0.99 us
// -> rise time & fall time must be >= 5*Tau
// -> clock period = 5*0.99 -> 10 us -> 100 kHz
#ifdef DEBUG_BUILD
// make it faster for debug builds due to missing optimization
#define SBUS_PLL_CLOCK 500
#else
#define SBUS_PLL_CLOCK 100
#endif
#define SBUS_PLL_DELAY  (1000/SBUS_PLL_CLOCK)
#define SBUS_PLL_DELAY2 (SBUS_PLL_DELAY/2)


void SetShiftReg(uint8_t or_value, uint8_t and_value);
void men_buf_write( char data );

void sci_read_cmd(void);

void pputchar(char mode, char data, char * extdata);

static char pc_cache(char data);
static void sendnibble( char data);
static void store_dbuf(char data);
void uinthex( const char data);
void uintdec(char data);
void ulongout(char modif, char * data);
void decout(uint8_t modif, uint8_t truncate, char * data);

int putchar_wrapper (char c, FILE *stream);

xSemaphoreHandle TxDone;
xSemaphoreHandle SerialBusMutex;
xQueueHandle xRxQ, xRxKeyQ, xTxQ;

uint8_t tx_stall = 0;
uint8_t ch_reset_detected;

struct S_TxChar{
	char data;
	uint8_t delay;
};

static FILE mystdout = FDEV_SETUP_STREAM(putchar_wrapper, NULL,
										 _FDEV_SETUP_WRITE);

const unsigned int char_convert[] PROGMEM = {
		0x004C, 0x4E60, 0x4E21, 0x4D45, 0x4D35, 0x4D28, 0x4D47, 0x4E20,
		0x4D71, 0x4D11, 0x4F29, 0x4F28, 0x4D08, 0x4D04, 0x4D01, 0x4D08,
		0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
		0x0048, 0x0049, 0x4D11, 0x4D18, 0x4E14, 0x4D05, 0x4D0A, 0x4F27,
		0x4D75, 0x4F0D, 0x4F0E, 0x4F0F,	0x4F10, 0x4F11, 0x4F12, 0x4F13,
		0x4F14, 0x4F15,	0x4F16, 0x4F17, 0x4F18, 0x4F19, 0x4F1A, 0x4F1B,
		0x4F1C, 0x4F1D, 0x4F1E, 0x4F1F, 0x4F20, 0x4F21,	0x4F22, 0x4F23,
		0x4F24, 0x4F25, 0x4F26, 0x4D71,	0x4D02, 0x4D11, 0x4E05, 0x004B,
		0x4D02, 0x004A, 0x4F0E, 0x4F0F, 0x4F10, 0x4F11, 0x4F12,	0x4F13,
		0x4F14, 0x4F15, 0x4F16, 0x4F17, 0x4F18,	0x4F19, 0x4F1A, 0x4F1B,
		0x4F1C, 0x4F1D, 0x4F1E,	0x4F1F, 0x4F20, 0x4F21, 0x4F22, 0x4F23,
		0x4F24,	0x4F25, 0x4F26, 0x4D71, 0x4E60, 0x4D11, 0x4D22, 0x004C
};

const char e_char_convert[] PROGMEM = {
		0x00,  0x00,  0x00,  0x0A,  0x6A,  0x06,  0x50,  0x00,
		0x00,  0x03,  0x00,  0x00,  0x00,  0x08,  0x00,  0x04,
		0x40,  0x41,  0x42,  0x43,  0x44,  0x45,  0x46,  0x47,
		0x48,  0x49,  0x00,  0x00,  0x00,  0x08,  0x00,  0x00,
		0x09,  0x0D,  0x0E,  0x0F,  0x10,  0x11,  0x12,  0x13,
		0x14,  0x15,  0x16,  0x17,  0x18,  0x19,  0x1A,  0x1B,
		0x1C,  0x1D,  0x1E,  0x1F,  0x20,  0x21,  0x22,  0x23,
		0x24,  0x25,  0x26,  0x00,  0x10,  0x03,  0x00,  0x00,
		0x00,  0x0D,  0x0E,  0x0F,  0x10,  0x11,  0x12,  0x13,
		0x14,  0x15,  0x16,  0x17,  0x18,  0x19,  0x1A,  0x1B,
		0x1C,  0x1D,  0x1E,  0x1F,  0x20,  0x21,  0x22,  0x23,
		0x24,  0x25,  0x26,  0x00,  0x00,  0x03,  0x03,  0x04
};

const char key_convert[2][21] PROGMEM = {
	// Control Head #3
	//     ---------------------------  1   2   3
	//     !                         !
	// D1  !                         !  4   5   6
	//     !                         !
	// D2  !                         !  7   8   9
	//     ---------------------------
	//     D3  (D4)  D5  (D6)  D7  (D8) *   0   #
	//
	// --,   D1,   D2,   D3,   D4,   D5,   D6,   D7,   D8,
	{0x00, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	//  3, 6, 9,    #   , 2, 5,  8, 0, 1, 4, 7,    *
		3, 6, 9,KC_ENTER, 2, 5,  8, 0, 1, 4, 7, KC_EXIT},

	// Control Head #2
	//     ---------------------------
	//     !                         !
	// D1  !                         !    5
	//     !                         !
	// D2  !                         !    8
	//     ---------------------------
	//     D3   D4   D5   D6   D7   D8
	//
	// --,   D1,   D2,   D3,   D4,   D5,   D6,   D7,   D8,
	{0x00,KC_D1,KC_D2,KC_D3,KC_D4,KC_D5,KC_D6,KC_D7,KC_D8,
	//  3, 6, 9,    #,    2,    5   ,     8   , 0, 1, 4, 7,    *
	    3, 6, 9,KC_ENTER, 2, KC_EXIT, KC_ENTER, 0, 1, 4, 7, KC_EXIT},
};


/*
;****************
; I O   I N I T
;****************
;
; Initialisiert I/O :
;                     - Port2 DDR ( SCI TX, PLL T/R Shift )
;                     - Port6 DDR ( PTT Syn Latch )
;                     - Port5 DDR ( EXT Alarm )
;                     - Port5 Data ( EXT Alarm off (1) )
;                     - RP5CR (HALT disabled)
;                     - I2C (Clock output, Clock=0)
;                     - Shift Reg (S/R Latch output, S/R Latch = 0, S/R init)
;                     - SCI TX = 1 (Pullup)
io_init
                aim  #%11100111,RP5CR     ; nicht auf "Memory Ready" warten, das statische RAM ist schnell genug
                                          ; HALT Input auch deaktivieren, Port53 wird später als Ausgang benötigt

                ldab #%10110100
                stab Port2_DDR_buf             ; Clock (I2C),
                stab Port2_DDR                 ; SCI TX, T/R Shift (PLL), Shift Reg Latch auf Ausgang

; I2C Init
                aim  #%11111011,Port2_Data     ; I2C Clock = 0
;ShiftReg Init
                aim  #%01111111,Port2_Data     ; Shift Reg Latch = 0
;SCI TX
                oim  #%10000, Port2_Data       ; SCI TX=1

                clr  SR_data_buf               ; Shuft Reg Puffer auf 0 setzen

                ldaa #~(SR_RFPA)               ; disable PA
                ldab #(SR_nTXPWR + SR_nCLKSHIFT + SR_9V6)
                                               ; disable Power control, disable clock shift, enable 9,6 V
                jsr  send2shift_reg

; Port 5
                ldab #%00001000
                stab SQEXTDDR                  ; EXT Alarm auf Ausgang, Alles andere auf Input
                stab SQEXTDDRbuf
                oim  #%00001000, SQEXTPORT     ; EXT Alarm off (Hi)

; Port 6
                ldab #%00001100
                stab Port6_DDR_buf
                stab Port6_DDR                 ; A16 (Bank Switch), PTT Syn Latch auf Ausgang

                aim  #%10011011, Port6_Data    ; Bank 0 wählen

                clr  led_buf
                clr  arrow_buf
                clr  arrow_buf+1

                clr  sql_ctr
                clr  ui_ptt_req             ;
                rts


 *
 */

// initialize IO ports, needs to be called before any IO operation takes place
void init_io()
{
	/*
	 *  Port A:
	 *  Bit 0-2 : PL Encode
	 *  Bit 3	: F1 / #F2  (Input from Control Panel, parallel to SCI RX)
	 *  Bit 4	: n.c.
	 *  Bit 5	: CALL_LED
	 *  Bit 6	: HUB
	 *  Bit 7	: Test
	 */

	DDRA  = (0 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
	PORTA = (1 << 2);

	/*
	 * Port B:
	 * Bit 0	: CSQ/UNSQ (Input from Control Panel)
	 * Bit 1-3  : SPI (SCK,MOSI,MISO)
	 * Bit 4	: Alert Tone (OC0)
	 * Bit 5	: Data Inhibit (clamp Signal Decode Input to GND)
	 * Bit 6	: Tx/Busy (Control Head Reset)
	 * Bit 7	: Syn_Latch
	 */

	DDRB  = (1 << 4) | ( 1 << 5 ) | ( 0 << 6) | ( 1 << 7 );
	PORTB = (1 << 4);

	/*
	 * Port C:
	 * Bit 0 - 3 : SEL_ENC (4-Bit DAC / Sel Call)
	 * Bit 4 - 7 : n.c.
	 */

	DDRC  = 0x0f;
	PORTC = 3;

	/*
	 * Port D:
	 * Bit 0	: SWB+
	 * Bit 1	: PTT (input)
	 * Bit 2-3	: UART1
	 * Bit 4	: Signalling Decode (IC1)
	 * Bit 5	: Clock
	 * Bit 6	: Data out
	 * Bit 7	: DPTT
	 */

	// leave Bit 6 low & input
	// only use PORTE2 as data i/o
	DDRD  = ( 1 << 5) | ( 0 << 6 ) | ( 1 << 7 );
	PORTD = ( 0 << 5) | ( 0 << 6);

	/*
	 * Port E:
	 * Bit 0-1	: UART0 (SCI to Control Head)
	 * Bit 2	: Data In
	 * Bit 3	: Brown-Out detection (AIN1)
	 * Bit 4	: #NMI  (INT4)
	 * Bit 5	: #STBY (INT5)
	 * Bit 6	: Lock Detect (INT6)    ( 1 = Lock )
	 * Bit 7	: Squelch Detect (INT7) ( 1 = Signal)
	 */

	DDRE  = ( 1 << 3 );
	PORTE = 1;

	/*
	 * Port F:
	 * Bit 0-2	: n.c.
	 * Bit 3	: Signalling Decode (ADC3)
	 * Bit 4-7	: JTAG (TCK, TMS, TDO, TDI)
	 */

	DDRF  = 0;
	PORTF = 0;

	/*
	 * Port G:
	 * Bit 0	: Call SW
	 * Bit 1	: Emergency
	 * Bit 2	: n.c.
	 * Bit 3	: SR Latch
	 * Bit 4	: #Mem enable
	 */

	PORTG = (1 << 4);
	DDRG  = (1 << 3) | (1 << 4);

	SR_data_buf = 0;
	led_buf = 0;
	arrow_buf = 0;
	sql_flag = 0;
	ui_ptt_req = 0;

	// create Mutex for HW access
	SerialBusMutex = xSemaphoreCreateRecursiveMutex();

    // Select RX VCO
    // Enable 9,6V regulator, set EXTALARM to high (open)
    SetShiftReg(SR_EXTALARM | SR_9V6, 0);

    stdout = &mystdout;

}



void init_sci()
{
	/* Set baud rate to 1200 7e1 */
	/* UBRR = Sysclk / (16 * Bitrate) -1 */
	/* UBRR = 4924800 Hz / (16 * 1200 1/s) - 1 = 255,5 */
#define SCI_BAUD 1200L

#define SCI_DIV ((F_CPU / (16 * SCI_BAUD) -1))
#define SCI_HIDIV ( SCI_DIV >> 8)
#define SCI_LODIV ( SCI_DIV & 0xff)

	UBRR0H = SCI_HIDIV;
	UBRR0L = SCI_LODIV;	// ca 1202 Bit / s

	/* Set frame format: 7data, odd parity, 1stop bit */
	UCSR0C = (1 << UPM01) |
			 (1 << UPM00) |
			 (0 << USBS0) |
			 (1 << UCSZ01)|
			 (0 << UCSZ00);

	/* Enable receiver and transmitter
	 * Disable RX interrupt */
	UCSR0B = (0 << RXCIE0) | (1<<RXEN0)|(1<<TXEN0);

	// create queues required for sci communication
	xRxQ = xQueueCreate( 1, sizeof( char ) );
	xRxKeyQ = xQueueCreate( 1, sizeof( char ) );
	xTxQ = xQueueCreate( 1, sizeof( struct S_TxChar ) );

	vSemaphoreCreateBinary( TxDone );
	xSemaphoreTake( TxDone, 0 );
	ch_reset_detected = LCD_CH_RESET_MAX;
}


/*
 *  Control Task exclusive
 */
void SetShiftReg(uint8_t or_value, uint8_t and_value)
{
	char i, d;

// SR_AUDIOPA   - 0 - Audio PA enable (1=enable)      (PIN 4 ) *
// SR_9V6       - 1 - STBY&9,6V                       (PIN 5 )
// SR_RXVCOSEL  - 2 - T/R Shift (1=TX)                (PIN 6 ) *
// SR_TXPWRLO   - 3 - Hi/Lo Power (1=Lo Power)        (PIN 7 ) *
// SR_nCLKSHIFT - 3
// SR_EXTALARM  - 4 - Ext. Alarm                      (PIN 14) *
// SR_SELATT    - 5 - Sel.5 ATT   (1=Attenuated Tones)(PIN 13) *
// SR_MICEN     - 6 - Mic enable  (1=enable)          (PIN 12) *
// SR_RXAUDIOEN - 7 - Rx Audio enable (1=enable)      (PIN 11)

	// UI task must not access blocking IO directly
	if(xTaskGetCurrentTaskHandle()== xUiTaskHandle)
		return;
	// get exclusive Bus access
	if(xTaskGetSchedulerState()==taskSCHEDULER_RUNNING){
		if(xSemaphoreTakeRecursive(SerialBusMutex, 0) == pdFAIL)
			return;
	}

	//
	SR_data_buf &= and_value;
	SR_data_buf |= or_value;
	d = SR_data_buf;

	taskENTER_CRITICAL();
	DDR_SBUS_DATA |= BIT_SBUS_DATA;

	for (i=0;i<8;i++)
	{
		if(d & 0x80)
		{
			//PORT_SBUS_DATA |= BIT_SBUS_DATA;
			PORT_SBUS_DATA |= BIT_SBUS_DATA;
			DDR_SBUS_DATA &= ~BIT_SBUS_DATA;
		}
		else
		{
			PORT_SBUS_DATA &= ~BIT_SBUS_DATA;
			DDR_SBUS_DATA |= BIT_SBUS_DATA;
		}

		d <<= 1;

		// set Port Bit to 0
		PORT_SBUS_CLK &= ~BIT_SBUS_CLK;
		_delay_loop_1(3);
		// set Port Bit to 1
		PORT_SBUS_CLK |= BIT_SBUS_CLK;
	}
	// Latch hi
	PORT_SR_LATCH |= BIT_SR_LATCH;

	// set Data to input (ext. Pull up)
	DDR_SBUS_DATA &= ~BIT_SBUS_DATA;
	PORT_SBUS_DATA |= BIT_SBUS_DATA;

	// Latch low
	PORT_SR_LATCH &= ~BIT_SR_LATCH;

	// Bus access finished
	if(xTaskGetSchedulerState()==taskSCHEDULER_RUNNING)
		xSemaphoreGiveRecursive(SerialBusMutex);
	// exit critical section
	taskEXIT_CRITICAL();
}



void SetPLL(const char RegSelect, char divA, int divNR)
{
/* 
 valid divider values for

 N: 3-1023
 A: 0-127
 R: 3-16383

 */
	char i, bits;
	uint32_t data;

	if(RegSelect)
	{
		// set R (14 Bit) + Control (1 Bit, 1)
		data = ((uint32_t)divNR << 2) | 2;	// include Control Bit
		// shift first data bit to MSB position
		data <<= 16;
		bits = 15;
	}
	else
	{
		// set N (10 Bit) & A (7 Bit) & Control ( 1 Bit, 0)
		data = ( (uint32_t) divNR << 8 ) | ((uint32_t)divA << 1);
		// shift first data bit to MSB position
		data <<= 14;
		bits = 18;
	}

	xSemaphoreTakeRecursive(SerialBusMutex, portMAX_DELAY);

	taskENTER_CRITICAL();

	DDR_SBUS_DATA |= BIT_SBUS_DATA;

	for(i=bits;i>0;i--)
	{
		if( (data >> 24) & 0x80)
		{
			PORT_SBUS_DATA |= BIT_SBUS_DATA;
		}
		else
		{
			PORT_SBUS_DATA &= ~BIT_SBUS_DATA;
		}
		// set Port Bit to 0
		PORT_SBUS_CLK &= ~BIT_SBUS_CLK;

		data <<= 1;
		// we need to wait some microseconds for the signal to settle
		_delay_us(SBUS_PLL_DELAY2);
		// set Port Bit to 1
		PORT_SBUS_CLK |= BIT_SBUS_CLK;
		_delay_us(SBUS_PLL_DELAY2);
	}
	// toggle PLL latch
	PORT_PLL_LATCH |= BIT_PLL_LATCH;

	// set Data to input (ext. Pull up)
	DDR_SBUS_DATA &= ~BIT_SBUS_DATA;

	// Bus access finished
	xSemaphoreGiveRecursive(SerialBusMutex);
	taskEXIT_CRITICAL();
	
	// ensure proper pulse width for latch signal
	_delay_us(SBUS_PLL_DELAY2);
	// reset latch
	PORT_PLL_LATCH &= ~BIT_PLL_LATCH;
}


//
//**********************************************
// I 2 C
//**********************************************
//
//*******************
// I 2 C   S T A R T
//*******************
//
// I2C "START" Condition senden
//
// changed Regs: NONE
//
void i2c_start()
{
	// Data Hi & Input
	DDR_SBUS_DATA  &=  ~BIT_SBUS_DATA;
	PORT_SBUS_DATA  |=  BIT_SBUS_DATA;
	_delay_loop_1(7);

	// I2C Clock Hi
	PORT_SBUS_CLK |= (BIT_SBUS_CLK);
	_delay_loop_1(7);

	// Data low
	PORT_SBUS_DATA &= ~(BIT_SBUS_DATA);
	DDR_SBUS_DATA  |=  (BIT_SBUS_DATA);
	_delay_loop_1(7);

	// I2C Clock Lo
	PORT_SBUS_CLK &= ~(BIT_SBUS_CLK);
	_delay_loop_1(14);

	// Data Hi / Input
	DDR_SBUS_DATA  &=  ~(BIT_SBUS_DATA);
	PORT_SBUS_DATA |=  (BIT_SBUS_DATA);
}
//******************
// I 2 C   S T O P
//******************
//
// I2C "STOP" Condition senden
//
// changed Regs: NONE
//
void i2c_stop()
{
	// Data low
	PORT_SBUS_DATA &= ~(BIT_SBUS_DATA);
	DDR_SBUS_DATA  |=  (BIT_SBUS_DATA);

	// I2C Clock Hi
	PORT_SBUS_CLK |= (BIT_SBUS_CLK);
	_delay_loop_1(7);

	// Data Leitung auf Hi / Eingang
	DDR_SBUS_DATA &=  ~BIT_SBUS_DATA;
	PORT_SBUS_DATA |=  BIT_SBUS_DATA;
	_delay_loop_1(7);

	// I2C Clock Lo
	PORT_SBUS_CLK &= ~(BIT_SBUS_CLK);

	// Data Leitung auf Eingang
	DDR_SBUS_DATA &= ~(BIT_SBUS_DATA);
	_delay_loop_1(14);
}

//***************
// I 2 C   A C K
//***************
//
// I2C "ACK" senden
// Bestätigung des Adress und Datenworts -> 0 im 9. Clock Zyklus senden
//
// changed Regs: NONE
//
void i2c_ack()
{
	// Data low
	PORT_SBUS_DATA &= ~(BIT_SBUS_DATA);
	DDR_SBUS_DATA  |=  (BIT_SBUS_DATA);

	// I2C Clock Hi
	PORT_SBUS_CLK |= (BIT_SBUS_CLK);
	_delay_loop_1(14);	

	// I2C Clock Lo
	PORT_SBUS_CLK &= ~(BIT_SBUS_CLK);
	_delay_loop_1(14);

	// Data wieder auf Eingang
	DDR_SBUS_DATA &= ~(BIT_SBUS_DATA);
	PORT_SBUS_DATA  |=  (BIT_SBUS_DATA);
}

//***********************
// I 2 C   T S T A C K
//***********************
//
// I2C "ACK" prüfen
// Bestätigung des Adress und Datenworts -> 0 im 9. Clock Zyklus senden
//
// Ergebnis    : A - 0 : Ack
//                   1 : No Ack / Error
//
// changed Regs: A
//
char i2c_tstack()
{
	char ack;

	// Data Input
	DDR_SBUS_DATA &= ~(BIT_SBUS_DATA);
	
	// I2C Clock Hi
	PORT_SBUS_CLK |= (BIT_SBUS_CLK);
	_delay_loop_1(14);	
	
	ack = PIN_SBUS_DATA & BIT_SBUS_DATA;

	// I2C Clock Lo
	PORT_SBUS_CLK &= ~(BIT_SBUS_CLK);
	_delay_loop_1(14);

	ack = ack ? 1 : 0;

	return ack;
}
//*************
// I 2 C   T X
//*************
//
// 8 Bit auf I2C Bus senden
//
// Parameter: B - Datenwort, wird mit MSB first auf I2C Bus gelegt
//
//
//
void i2c_tx(char data)
{
	char i;
	
	PORT_SBUS_DATA |= BIT_SBUS_DATA;
	DDR_SBUS_DATA &= ~(BIT_SBUS_DATA);
	for(i=0;i<8;i++)
	{
		if(data & 0x80)
		{
			DDR_SBUS_DATA &= ~(BIT_SBUS_DATA);
			PORT_SBUS_DATA |= BIT_SBUS_DATA;
		}
		else
		{
			PORT_SBUS_DATA &= ~BIT_SBUS_DATA;
			DDR_SBUS_DATA |= BIT_SBUS_DATA;
		}
		_delay_loop_1(7);

		PORT_SBUS_CLK |= BIT_SBUS_CLK;
		_delay_loop_1(14);
		data <<= 1;

		PORT_SBUS_CLK &= ~BIT_SBUS_CLK;
		_delay_loop_1(7);
	}
	DDR_SBUS_DATA &= ~BIT_SBUS_DATA;
	PORT_SBUS_DATA |= BIT_SBUS_DATA;
}

//*************
// I 2 C   R X
//*************
//
// Byte auf I2C Bus empfangen
//
// Ergebnis : B - Empfangenes Byte
//
// changed Regs: B
//
char i2c_rx()
{
	char i,data;

	data = 0;

	DDR_SBUS_DATA &= ~BIT_SBUS_DATA;

	for(i=0;i<8;i++)
	{
		PORT_SBUS_CLK |= BIT_SBUS_CLK; 
		_delay_loop_1(14);

		data<<=1;
		if (PIN_SBUS_DATA & BIT_SBUS_DATA)
		{
			data |= 1;
		}
		PORT_SBUS_CLK &= ~BIT_SBUS_CLK; 
		_delay_loop_1(14);
	}

	return data;
}



//**********************************************
// S C I
//**********************************************
//************************
// S C I   R X
//************************
// Parameter: A - Status (0=RX ok, 3=no RX)
//            B - rxd Byte
//
// changed Regs : A, B, X
//
// required Stack Space : 2
//
char sci_rx(char * data)
{
	char rxd;

	if(xQueueReceive( xRxQ, &rxd, 0 ) == pdPASS)
	{
		// do not read data, if pointer is NULL
		if(data)
			*data = rxd;
		return 0;
	}
	else
		return 3;
}


//************************
// S C I   R E A D
//************************
//
// Zeichen vom serieller Schnittstelle lesen (blocking)
//
// Parameter : B - rxd Byte
//
// changed Regs : A, B, X
//
// required Stack Space : 2
//
void sci_read(char * data)
{
	char rxd;
	// Wenn Zeiger auf Lese und Schreibpos gleich sind keine Daten gekommen -> warten
	xQueueReceive( xRxQ, &rxd, portMAX_DELAY );
	if(data)
		*data = rxd;
}


//************************
// S C I   R X   M
//************************
//
// Taste von Bedienteil lesen (non-blocking)
//
// Parameter:  none
//
// Returns: A - raw value ( Bit 0-6)
//              Status  (Bit 7) (0=RX ok, 1=no RX)
//          B - converted Byte (Key convert Table)
//
// changed Regs : A, B
//
// required Stack Space : 2
//
char sci_rx_m(char * data)
{
	uint8_t raw;

	if(xQueueReceive( xRxKeyQ, &raw, 0) == errQUEUE_EMPTY)
	{
		*data = 0;
		raw = 0x80;
	}
	else
	{
		*data = pgm_read_byte( &key_convert[(uint8_t)config.controlHead][raw]);
	}
	return raw;
}

//************************
// S C I   R E A D   M
//************************
//
// Eingabe von Bedienteil lesen (blocking)
//
// Parameter:  none
//
// Ergebnis :  A : raw data
//             B : converted data
//
// changed Regs : A, B
//
// required Stack Space : 4
//
char sci_read_m( char * data)
{
	uint8_t raw;

	if(xQueueReceive( xRxKeyQ, &raw, m_timer) == errQUEUE_EMPTY)
	{
		return -1;
	}

	*data = key_convert[(uint8_t)config.controlHead][raw];

	return raw;
}

//************************
// S C I   T X
//************************
//
// Transfer char to/via SCI
//
// Parameter:  none
//
// Returns : A - Status (0=ok, 1=buffer full)
//           B - TX Byte
//
// changed Regs : none
//
// required Stack Space : 3
//
void sci_tx(char data)
{
	struct S_TxChar txchar;

	txchar.data = data;
	txchar.delay = 0;

	// send data & delay
	xQueueSendToBack( xTxQ, &txchar, portMAX_DELAY);
	
}

//
//************************
// S C I   T X   W
//************************
//
// Sendet Zeichen in B nach Ablauf des LCD_TIMER. Setzt abhängig von gesendeten
// Zeichen den Timer neu. Berücksichtigt unterschiedliche Timeouts für $78, $4F/$5F und Rest
//
// Parameter    : B - zu sendendes Zeichen
//
// Ergebnis     : Nichts
//
// changed Regs : None
//
// required Stack Space : 6
//
void sci_tx_w( char data)
{
	struct S_TxChar txchar;

	txchar.data = data;
	switch((uint8_t)data)
	{
		// send data related to extended chars without delay
		case 0x4D:
		case 0x5D:
		case 0x4E:
		case 0x5E:
		case 0x4F:
		case 0x5F:
			txchar.delay = 0;
			break;
		default:
			txchar.delay = LCDDELAY;
			break;
	}

	// display clear command need 4* normal delay
	if(data >= 0x78)
		txchar.delay = LCDDELAY<<2;

	// send data & delay
	xQueueSendToBack( xTxQ, &txchar, portMAX_DELAY);
}

/*
 *  SCI Handler
 *
 * inter-key-time ~158,7 ms (150 ms + 8,66 ms)
 */
#define KEYLOCK 0x74
#define KEY_UNLOCK 0x75

void sci_rx_handler()
{
	static char extended=0;

	if (UCSR0A & (1 << RXC0))
	{
		char rx;

		if(!(UCSR0A & ((1<<UPE0) | (1<<FE0))))
		{
			// reception OK

			rx = UDR0;
			if((rx == 0x7e) && ch_reset_detected)
				ch_reset_detected--;

			if(!extended && rx && ((rx < 0x20) || (rx == KEYLOCK)))
			{
				if (rx == KEYLOCK)
				{
					rx = KEY_UNLOCK;
					rx_ack_buf = rx;
				}
				else
				{
					xQueueSendToBack( xRxKeyQ, &rx, 0);
					rx_ack_buf = rx;
				}
			}
			else
			{
				if(rx != 0x7f)
				{
					xQueueSendToBack( xRxQ, &rx, 0);
				}
				else	// control head awaits ack for something, postpone own transmission for some time
					tx_stall = 5;

				// check if char just received was first byte of 2 byte combo
				
				rx |= 0x10;
				if(!extended && ((rx == 0x5d) || (rx == 0x5e) || (rx == 0x5f)))
					extended = 1;
				else
					extended = 0;
				// then next char has to go into RxQ
			}
		}
		else
		{
			rx = UDR0;
		}
	}
}


void sci_tx_handler()
{
	// check if the TX Reg is currently empty
	if(UCSR0A & (1<<TXC0))
	{
		struct S_TxChar tx;

		if(!lcd_timer)
		{
			// check if a keystroke needs to be acknowledged
			if(rx_ack_buf)
			{
				UDR0 = rx_ack_buf;
				// clear ack buf
				rx_ack_buf = 0;
				// wait some time before sending next char
				// 20 ms seems to work reasonably well
				lcd_timer = 20;
			}
			else
			{
				// check if tx is stalled
				if(tx_stall)
					tx_stall--;
				else
					// check if there is something to be sent in the buffer
					// do not block here
				if(xQueuePeek( xTxQ, &tx, 0) == pdPASS)
				{
					// do not send if there is a byte in the input buffer
					// could be a keystroke which has to be acknowledged first
					if(!(UCSR0A & (1 << RXC0)))
					{
						xQueueReceive( xTxQ, &tx, 0);
						UDR0 = tx.data;
						lcd_timer = tx.delay;
					}
				}
			}
		}
	}
}

//
//****************
// S C I   A C K
//****************
//
// Parameter : B - gesendetes Zeichen, das best�tigt werden soll
//
// Result    : A - 0 = OK
//                 1 = Error / Timeout
//
// changed Regs: X
//
// required Stack Space : 13 / 4 + putchar 'p'
//
//
char sci_ack(const char data)
{
	char ret = 1;
	char read;

	if(xQueueReceive( xRxQ, &read, LCDDELAY*4))
	{
	// check if char was received
		if(data == read)
		{
			ret = 0;
		}
	}

	return ret;
}



//
//************************************************
//
//************************
// P U T C H A R
//************************
//
// Putchar Funktion, steuert das Display an.
//
// Parameter : A - Modus
//                 'c' - ASCII Char, Char in B (ASCII $20-$7F, $A0-$FF = char + Blink)
//                 'x' - unsigned int (hex, 8 Bit)
//                 'd' - unsinged int (dezimal, 8 Bit) (0-99, value is printed with leading zero)
//                 'u' - unsinged int (dezimal, 8 Bit)
//                 'l' - longint (dezimal, 32 Bit)
//                 'p' - PLAIN, gibt Zeichen unverändert aus, speichert es NICHT im Puffer
//                       Anwendung von 'p' : Senden von Display Befehlen (Setzen des Cursors, Steuern der LEDs, etc.)
//
//
//             B - Zeichen in Modus c,x,u,p,d
//                 Anzahl nicht darzustellender Ziffern in Modus 'l' (gezählt vom Ende! - 1=Einer, 2=Zehner+Einer, etc...)
//
//             X - Pointer auf longint in Modus L
//
//             Stack - Longint in Modus l
//
//
// Ergebnis :    nothing
//
// changed Regs: A,B,X
//
// changed Mem : CPOS,
//               DBUF,
//               Stack (longint)
//
// required Stack Space : 'c' - 15
//                        'd' - 24
//                        'u' - 23
//                        'l' - 33
//                        'x' - 23
//                        'p' - 15
//                        jeweils +15 bei Keylock
//
void pputchar(char mode, char data, char * extdata)
{
	switch(mode)
	{
	case 'u':
		uintdec(data);
		break;
	case 'd':
		uintdec(data);
		break;
	case 'l':
		ulongout(data, extdata);
		break;
	case 'L':
		decout(0, 0, extdata);
		break;
	case 'x':
		uinthex(data);
		break;
	case 'c':
	case 'p':
		break;
	default:
		return;
	}

	if(mode == 'c')
	{
		if(cpos < 8)
		{
			uint8_t c;
			unsigned int out;

			// check if char is already displayed at current position
			// exit if it is
			if (pc_cache(data))
				store_dbuf(data);
			else
				return;

			// exclude blink bit
			c = data & ~CHR_BLINK;

			c-=0x20;

			out = pgm_read_word( &char_convert[c] );
			if(data & CHR_BLINK)
			{	// include Blink Bit
				if(out & 0xff00)
					out |= 0x1000;
				else
					out |= 0x10;
			}

			do
			{
				// check for 2 byte code
				if (out & 0xff00)
				{	// transmit hi byte
					// TODO: Exit with error after x unsuccessful attempts
					do
					{
						sci_tx_w(out>>8);
					}while( sci_ack(out>>8) );
				}

				// TODO: Exit with error after x unsuccessful attempts
				sci_tx_w(out & 0xff);
			}while( sci_ack(out & 0xff) );

			// check for extended character
			if(((out >> 8) | 0x10) == 0x5d)
			{
				// decide between blinking & non-blinking chars
				out = data & CHR_BLINK ? 0x5e00 : 0x4e00;
				// include char from table
				out |= pgm_read_byte( &e_char_convert[c]);
				do
				{
					sci_tx_w(out>>8);
				}while( sci_ack(out>>8) );

				do
				{
					sci_tx_w(out & 0xff);
				}while( sci_ack(out & 0xff) );
			}
		}
	}
	// plain mode
	if (mode == 'p')
	{	// write unmodified data
		// this is mainly used for control characters
		do
		{
			sci_tx_w(data);
		}while( sci_ack(data) );
	}
}


//******************
// P C   C A C H E
//
// Putchar Subroutine
//
// Funktion zur Beschleunigung der Displayausgabe
// Überspringt Zeichen, die schon auf dem Display vorhanden sind
// Falls ein Unterschied zwischen vorhandenem und zu schreibenden Zeichen
// auftritt, wird der Cursor von dieser Funktion korrekt positioniert
//
// Parameter : B - Zeichen (ASCII)
//
// Ergebnis :  A - 0 = Zeichen überspringen
//                 1 = Zeichen ausgeben
//
static char pc_cache(char data)
{
	char ret;

	if(dbuf[cpos] == data)
	{
		cpos++;
		// cpos and control head cursor position are different now
		pcc_cdiff_flag = 1;
		ret = 0;
	}
	else
	{
		// check if control head cursor is different from local cursor
		if(pcc_cdiff_flag)
		{	// set cursor to current position
			pputchar('p', cpos+0x60, 0);
			// cursor position is up to date
			pcc_cdiff_flag = 0;
		}
		ret = 1;
	}
	return ret;
}


//***************
// U I N T H E X
//
// Putchar Subroutine
//
// Formatiert einen 8 Bit Integer für Hexadezimale Darstellung um und gibt ihn aus
//
// Parameter : B - 8 Bit Integer
//
// Ergebnis : none
//
// changed Regs : A,B,X
//
// Required Stack Space : 21
//
void uinthex( const char data)
{
	sendnibble(data >> 4);
	sendnibble(data & 0x0f);
}

//**********************
// S E N D   N I B B L E
//
// Putchar Subroutine
//
// Gibt einen 4 Bit Wert als HEX Digit (0-9,A-F) aus
//
// Parameter : B - 4 Bit Wert
//
// Ergebnis : none
//
// changed Regs : A,B,X
//
//
void sendnibble( char data)
{
	// format hex nibble to character (0-9, a-f)
	data = data > 9 ? data + 0x37 : data + 0x30;
	// print character
	pputchar('c', data, 0);
}



//*****************
// U I N T D E C D
//
// Putchar Subroutine
//
// Prints 2 decimal digits
//
// Parameter : B - 8 Bit Integer
//
// Ergebnis : none
//
// changed Regs : A,B,X
//
// Required Stack Space : 21
//
// TODO: Bug in ASM source, bne, should be bcs
void uintdecd(char data)
{
	if(data >= 100)
	{
		data -= 100;
	}
	if(data < 10)
	{
		pputchar('c', '0', 0);
	}
	uintdec(data);
}


//****************
// U I N T   D E C
//
// Putchar Subroutine
//
// Formatiert einen 8 Bit Integer für Dezimale Darstellung um und gibt ihn aus
//
// Parameter : B - 8 Bit Integer
//
// Returns : nothing
//
// changed Regs : A,B,X
//
// Required Stack Space : 21
//                        36 (bei keylock)
//
void uintdec(char data)
{
	div_t divresult;

	if(data>=10)
	{
		if(data>=100)
		{
			divresult = div(data,10);
			data = divresult.rem;
			pputchar('c', divresult.quot+'0', 0);
		}

		divresult = div(data,10);
		data = divresult.rem;
		pputchar('c', divresult.quot+'0', 0);
	}
	pputchar('c', data+'0', 0);
}


//******************
// U L O N G   O U T
//
// Putchar Subroutine
//
// Formatiert einen 32 Bit Integer für Dezimale Darstellung um und gibt ihn aus
//
// Parameter : B - Anzahl der vom Ende der Zahl abzuschneidenden Ziffern (Bit 0-3)
//                 Flags (Bit 4-7, only ulongout -> see below)
//             A - Anzahl der mindestens auszugebenden Stellen (Bit 0-3)
//                 Flags (udecout)
//                 force sign inversion on return (Bit 4)
//                 force negative sign (Bit 5)
//                 Force sign print (Bit 6)
//                 Prepend space instead of zero (Bit 7)
//             Stack - 32 Bit Integer
//
// Ergebnis : none
//
// changed Regs : A,B,X
//
// Required Stack Space : 31
//
// 7 Long LoWord,LoByte
// 6 Long LoWord,HiByte
// 5 Long HiWord,LoByte
// 4 Long HiWord,HiByte
// 3 R-Adresse2 lo
// 2 R-Adresse2 hi
// 1 R-Adresse1 lo
// 0 R-Adresse1 hi
//
void ulongout(char modif, char * data)
{
	decout(modif & 0xf0, modif & 0x0f, data);
}


void decout(uint8_t modif, uint8_t truncate, char * data)
{
#define SIGINVBEFORERETURN 0x10
#define NEGSIGN 0x20
#define PRINTSIGN 0x40
#define FILLWITHSPACE 0x80

	char mindigits = modif & 0x0f;
	signed long * l;
	char i;
	// 10 chars for long int (10^9) + 1 for sign + 1 for terminator
	char buf[12];

	l = (signed long *) data;

	if (*l < 0)
	{
		modif |= PRINTSIGN | NEGSIGN;
	}

	truncate &= 7;		// truncate max. 7 digits

	if(modif & NEGSIGN)
	{
		// if sign is to be printed,
		// decrease number of digits to print by 1
		if(mindigits)
			mindigits--;
	}

	// convert longint for printing to digits in base 10
	ltoa(*l, buf, 10);

	i = strlen(buf);

	if(i < mindigits)
	{
		char j;

		if(modif & PRINTSIGN)
		{
			mindigits--;
			if (!(modif & FILLWITHSPACE))
			{
				if(modif & NEGSIGN)
					pputchar('c','-',0);
				else
					pputchar('c','+',0);

				for(j=i;j<mindigits;j++)
				{
					pputchar('c','0',0);
				}
			}
			else
			{
				for(j=i;j<mindigits;j++)
				{
					pputchar('c',' ',0);
				}
				if(!(modif & NEGSIGN))
					pputchar('c','+',0);
			}
		}
	}
	else
	{
		if(modif & PRINTSIGN)
		{
			if(!(modif & NEGSIGN))
				pputchar('c','+',0);
		}
	}

	i=0;
	while(buf[(uint8_t) i] && i< (strlen(buf)-truncate))
	{
		pputchar('c',buf[(uint8_t) i++],0);
	}
}


//**********************
// S T O R E   D B U F
//
// Putchar Subroutine
//
// stores Byte from B in Display Buffer - if Space left
//
// changed Regs: NONE
//
// required Stack Space : 6
//
static void store_dbuf(char data)
{
	if(cpos < 8)
	{
		dbuf[cpos++] = data;
	}
}


//
//************************************
// P R I N T F
//************************************
//
// Print formatted string including variables
//
// use printf from avr-libc

int putchar_wrapper (char c, FILE *stream)
{
	pputchar('c', c, 0);
	return 0;
}


