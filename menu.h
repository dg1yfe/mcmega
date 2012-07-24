/*
 * menu.h
 *
 *  Created on: 29.05.2012
 *      Author: eligs
 */

#ifndef MENU_H_
#define MENU_H_
#include <avr/pgmspace.h>

extern char m_ok_str[] PROGMEM;
extern char m_no_lock_str[] PROGMEM;
extern char m_out_str[] PROGMEM;
extern char m_range_str[] PROGMEM;
extern char m_writing[] PROGMEM;
extern char m_stored_str[] PROGMEM;
extern char m_failed_str[] PROGMEM;
extern char m_delete[] PROGMEM;
extern char m_offset[] PROGMEM;
extern char m_sq_on_str[] PROGMEM;
extern char m_sq_off_str[] PROGMEM;
extern char m_menu_str[] PROGMEM;


#define IDLE  	     0
#define F_IN 	     1
#define MEM_SELECT   2
#define MEM_STORE    3
#define MEM_RECALL_LOAD 4
#define TXSHIFT_SW   5
#define MENU_SELECT  6
#define TXSHIFT_DIGIT  7
#define FREQ_DIGIT 8
#define POWER_SELECT 9
#define DEFCH_SELECT 10
#define MEM_SELECT_RECALL 11
#define MEM_SELECT_STORE 12
#define SHOW_VERSION 13
//#define MEM_SEL_DIGIT 5


void menu_init();
void menu();
void m_none(char key);
extern void m_reset_timer();
extern void m_norestore();


#endif /* MENU_H_ */
