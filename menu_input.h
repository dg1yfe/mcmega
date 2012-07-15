/*
 * menu_input.h
 *
 *  Created on: 02.06.2012
 *      Author: F. Erckenbrecht
 */

#ifndef MENU_INPUT_H_
#define MENU_INPUT_H_
#include <stdint.h>

void m_start_input(char key);
void m_print(char key);
void m_f_in(char key);
void m_non_numeric(char key);
void m_backspace (void);
void m_clr_displ (void);
void m_set_freq (void);
void m_set_freq_x(void);
void m_frq_prnt(void);
char m_digit_editor(char key, char lopos, char highpos, int8_t mode);

#endif /* MENU_INPUT_H_ */
