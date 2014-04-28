/*
 * io.h
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

#ifndef IO_H_
#define IO_H_

#include "FreeRTOS.h"
#include "semphr.h"

	/*
	 * Port D:
	 * Bit 0	: SWB+
	 * Bit 1	: PTT (input)
	 * Bit 2-3	: UART1
	 * Bit 4	: Signalling Decode (IC1)
	 * Bit 5	: Clock
	 * Bit 6	: Data out
	 * Bit 7	: #DPTT
	 */
#define PORT_DPTT PORTD
#define BIT_DPTT (1<<PD7)

#define PIN_SQL PINE
#define BIT_SQL (1<<PE7)

#define PORT_EEP_DISABLE PORTG
#define BIT_EEP_DISABLE  (1 << 4);

#define PORT_SEL_DAC PORTC
#define MASK_SEL_DAC  (0x0f)

#define PORT_PL_DAC PORTA
#define MASK_PL_DAC (0x07)

	/*
	 * Port B:
	 * Bit 0	: CSQ/UNSQ (Input from Control Panel)
	 * Bit 1-3  : SPI (SCK,MOSI,MISO)
	 * Bit 4	: Alert Tone (OC0)
	 * Bit 5	: Data Inhibit (clamp Signal Decode Input to GND)
	 * Bit 6	: Tx/Busy (Control Panel Reset)
	 * Bit 7	: Syn_Latch
	 */
#define PORT_CPRESET PORTB
#define BIT_CPRESET (1 << 6)

#define PORT_SIGIN PORTD
#define PIN_SIGIN PIND
#define BIT_SIGIN (1 << 4)

// initialize IO ports, needs to be called before any IO operation takes place
void init_io( void );

void SetShiftReg(uint8_t or_value, uint8_t and_value);

void init_sci( void );
void SetPLL(const char RegSelect, char divA, int divNR);
void i2c_start();
void i2c_stop();
void i2c_ack();
char i2c_tstack();
void i2c_tx(char data);
char i2c_rx();

void sci_rx_handler();
void sci_tx_handler();

char sci_rx(char * data);
void sci_read(char * data);
char sci_rx_m(char * data);
char sci_read_m( char * data);
char sci_tx_buf(char data);
void sci_tx(char data);
void sci_tx_w( char data);
char check_inbuf();
char check_outbuf();
char sci_ack(const char data);
void sci_trans_cmd();
void men_buf_write( char data );
void pputchar(char mode, char data, char * extdata);
void uintdecd(char data);

#define SIGINVBEFORERETURN 0x10
#define NEGSIGN 0x20
#define PRINTSIGN 0x40
#define FILLWITHSPACE 0x80
void decout(uint8_t modif, uint8_t truncate, char * data);



extern char key_convert[2][21];
extern xSemaphoreHandle SerialBusMutex;
extern xQueueHandle xRxKeyQ, xRxQ;

extern uint8_t ch_reset_detected;

#endif /* IO_H_ */
