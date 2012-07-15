/*
 * eeprom.h
 *
 *  Created on: 13.07.2012
 *      Author: F. Erckenbrecht
 */

#ifndef EEPROM_H_
#define EEPROM_H_


char eep_rand_read(unsigned int address, char * data);
char eep_read(char flags, char * data);
char eep_seq_read(unsigned int address, unsigned int bytecount,
					void * dest, unsigned int  * bytesread);
char eep_write(unsigned int address, char * data);
char eep_write_seq(unsigned int address, char bytecount, void * data);
unsigned int eep_get_size();

char eep_chk_crc();
char eep_write_crc();
char eep_rd_ch_freq(uint8_t slot, unsigned long * f);

#endif /* EEPROM_H_ */
