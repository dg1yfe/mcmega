/*
 * io.h
 *
 *  Created on: 26.05.2012
 *      Author: eligs
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

#define CONTROL_HEAD3 0
#define CONTROL_HEAD2 1

#define PORT_SEL_DAC PORTC
#define MASK_SEL_DAC  (0x0f)

#define PORT_PL_DAC PORTA
#define MASK_PL_DAC (0x07)

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
