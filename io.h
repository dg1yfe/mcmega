/*
 * io.h
 *
 *  Created on: 26.05.2012
 *      Author: eligs
 */

#ifndef IO_H_
#define IO_H_

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



extern char key_convert[];





#endif /* IO_H_ */
