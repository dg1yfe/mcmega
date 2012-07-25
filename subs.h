/*
 * subs.h
 *
 *  Created on: 26.05.2012
 *      Author: F. Erckenbrecht
 */

#ifndef SUBS_H_
#define SUBS_H_
#include <stdint.h>

void pwr_sw_chk( char cSaveSettings);

void receive(void);
void transmit(void);

char ptt_get_status(void);
//void vco_switch(char vco);
unsigned int crc16(unsigned int bytecount, void * data, unsigned int init);

char read_eeep_ch(uint16_t slot, long * freq);
char read_ieep_ch(uint16_t slot, long * freq);
char store_eeep_ch(uint16_t slot);
char store_ieep_ch(uint16_t slot);
char store_current(void);
char read_current(unsigned long * freq,long * txshift, long * offset);

void squelch(void);

void wd_reset(void);

void audio_pa(uint8_t enable, uint8_t withrxaudio);

void rfpwr_set(uint8_t enable_hi_power);
void rfpwr_print(void);

#endif /* SUBS_H_ */
