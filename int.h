/*
 * int.h
 *
 *  Created on: 26.05.2012
 *      Author: eligs
 */

#ifndef INT_H_
#define INT_H_

void init_SIO(void);
void init_OCI(void);

void init_Timer2();
void start_Timer2();
void stop_Timer2();


extern volatile uint8_t int_lcd_timer_dec;
extern volatile uint8_t int_wd_reset;
extern volatile uint16_t PL_phase_delta;
extern volatile uint16_t SEL_phase_delta;
extern volatile uint16_t SEL_phase_delta2;
extern volatile unsigned int PL_phase;
extern volatile unsigned int SEL_phase;
extern volatile unsigned int SEL_phase2;


#endif /* INT_H_ */
