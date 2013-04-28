/*
 * menu.h
 *
 *  Created on: 29.05.2012
 *      Author: eligs
 */

#ifndef MENU_H_
#define MENU_H_
#include <avr/pgmspace.h>

extern const char m_ok_str[] PROGMEM;
extern const  char m_no_lock_str[] PROGMEM;
extern const  char m_out_str[] PROGMEM;
extern const  char m_range_str[] PROGMEM;
extern const  char m_writing[] PROGMEM;
extern const  char m_stored_str[] PROGMEM;
extern const  char m_failed_str[] PROGMEM;
extern const  char m_delete[] PROGMEM;
extern const  char m_offset[] PROGMEM;
extern const  char m_sq_on_str[] PROGMEM;
extern const  char m_sq_off_str[] PROGMEM;
extern const  char m_menu_str[] PROGMEM;

enum {IDLE, F_IN, MEM_SELECT, MEM_STORE, MEM_RECALL_LOAD,
	  TXSHIFT_SW, MENU_SELECT, TXSHIFT_DIGIT,
	  FREQ_DIGIT, POWER_SELECT, DEFCH_SELECT, MEM_SELECT_RECALL,
	  MEM_SELECT_STORE, SHOW_VERSION, CTCSS_SEL_RX, CTCSS_SEL_TX,
	  CAL, TEST, DEBUG};
//#define MEM_SEL_DIGIT 5


void menu_init();
void menu();
void m_none(char key);
extern void m_reset_timer();
extern void m_norestore();


#endif /* MENU_H_ */
