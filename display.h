/*
 * display.h
 *
 *  Created on: 28.05.2012
 *      Author: Felix Erckenbrecht
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

// Control Head Error Counter
// After this amount of reset char is received
// the control head is being reset
#define LCD_CH_RESET_MAX 50

#define LCD_NO_CLEAR_LED 0
#define LCD_CLEAR_LED 1

void lcd_h_reset(void);
char lcd_s_reset(void);
void lcd_clr(char clear_leds);
void lcd_backspace(void);
void save_dbuf(void);
void restore_dbuf(void);
extern void lcd_timer_reset(void);
void lcd_cpos(unsigned char pos);
void lcd_fill(void);
void led_set(char led, char status);
void led_update(void);
void arrow_set(char pos, char state);
void lcd_chr_mode(uint8_t position, char mode);

#endif /* DISPLAY_H_ */
