/*
 * menu.h
 *
 *  Created on: 29.05.2012
 *      Author: eligs
 */

#ifndef MENU_H_
#define MENU_H_


extern char m_ok_str[];
extern char m_no_lock_str[];
extern char m_out_str[];
extern char m_range_str[];
extern char m_writing[];
extern char m_stored[];
extern char m_failed[];
extern char m_delete[];
extern char m_offset[];
extern char m_sq_on_str[];
extern char m_sq_off_str[];


#define IDLE  	     0
#define F_IN 	     1
#define MEM_SELECT   2
#define MEM_STORE    3
#define MEM_RECALL_LOAD 4
#define TXSHIFT_SW   5
#define MENU_SELECT  6
#define TXSHIFT_DIGIT  7
//#define MEM_SEL_DIGIT 5


void menu_init();
void menu();


#endif /* MENU_H_ */
