/*
 * subs.h
 *
 *  Created on: 26.05.2012
 *      Author: F. Erckenbrecht
 */

#ifndef SUBS_H_
#define SUBS_H_

void pwr_sw_chk( char cSaveSettings);

void receive(void);
void transmit(void);

char ptt_get_status(void);
void vco_switch(char vco);
unsigned int crc16(unsigned int bytecount, void * data, unsigned int init);
char read_eep_ch(uint16_t slot, long * freq);
char store_eep_ch(uint16_t slot);
void tone_start(void);
void tone_stop(void);
void squelch(void);
void wd_reset(void);
char store_current(void);
char read_current(unsigned long * freq,long * txshift, long * offset);


#endif /* SUBS_H_ */
