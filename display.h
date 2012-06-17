/*
 * display.h
 *
 *  Created on: 28.05.2012
 *      Author: Felix Erckenbrecht
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_


void lcd_h_reset();
char lcd_s_reset();
void lcd_clr(char clear_leds);
void lcd_backspace();
void save_dbuf();
void restore_dbuf();
inline void lcd_timer_reset();
void lcd_cpos(unsigned char pos);
void lcd_fill();
void led_set(char led, char status);
void led_update();
void arrow_set(char pos, char state);
void lcd_chr_mode(uint8_t position, char mode);

#endif /* DISPLAY_H_ */
