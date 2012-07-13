/*
 * menu_top.h
 *
 *  Created on: 13.07.2012
 *      Author: eligs
 */

#ifndef MENU_TOP_H_
#define MENU_TOP_H_

inline void m_frq_up(void);
inline void m_frq_down(void);
void m_sql_switch(void);
void m_tone(void);
inline void m_txshift(char key);
inline void mts_print(void);
inline void mts_switch(char key);
void mts_digit(char key);
void m_set_shift(void);
void m_prnt_rc(void);
void m_test(void);
void m_tone_stop(void);
void m_prnt_tc(void);
inline void m_submenu(char key);
char m_freq_digit(char key);
void m_top(uint8_t key);



#endif /* MENU_TOP_H_ */
