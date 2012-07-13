/*
 * subs.h
 *
 *  Created on: 26.05.2012
 *      Author: F. Erckenbrecht
 */

#ifndef SUBS_H_
#define SUBS_H_

void pwr_sw_chk( char cSaveSettings);


inline void vco_switch(char vco);
unsigned int crc16(unsigned int bytecount, void * data, unsigned int init);
char read_eep_ch(uint16_t slot, long * freq);
char store_eep_ch(uint16_t slot);
void tone_start(void);
void tone_stop(void);

#endif /* SUBS_H_ */
