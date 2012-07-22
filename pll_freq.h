/*
 * pll_freq.h
 *
 *  Created on: 27.05.2012
 *      Author: F. Erckenbrecht
 */

#ifndef PLL_FREQ_H_
#define PLL_FREQ_H_

#include <stdint.h>

uint8_t freq_init(void);
char init_pll(unsigned int spacing);
void pll_led(char force);
char pll_lock_chk(void);
void pll_set_channel(unsigned long channel);
void set_freq(unsigned long * freq);
void set_tx_freq(unsigned long * freq);
void set_rx_freq(unsigned long * freq);
unsigned long frq_cv_freq_ch(unsigned long * freq);
unsigned long frq_cv_ch_freq(unsigned long ch);
unsigned long frq_get_freq(void);
unsigned long frq_calc_freq(char * str);
void frq_update(unsigned long *freq);
void freq_print(unsigned long * freq);
void freq_offset_print(void);
void frq_check(void);




#endif /* PLL_FREQ_H_ */
