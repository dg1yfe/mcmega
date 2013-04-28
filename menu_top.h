/*
 * menu_top.h
 *
 *  Created on: 13.07.2012
 *      Author: eligs
 */

#ifndef MENU_TOP_H_
#define MENU_TOP_H_

extern void m_frq_up(void);
extern void m_frq_down(void);
void m_sql_switch(void);
void m_tone(void);
extern void m_txshift(char key);
extern void mts_print(void);
extern void mts_switch(char key);
void mts_digit(char key);
void m_set_shift(void);
void m_prnt_rc(void);
void m_test(char key);
void m_tone_stop(void);
void m_prnt_tc(void);
void m_submenu(char key);
char m_freq_digit(char key);
void m_top(uint8_t key);
void m_debug(char key);


#endif /* MENU_TOP_H_ */
