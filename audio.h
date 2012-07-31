/*
 * audio.h
 *
 *  Created on: 25.07.2012
 *      Author: F. Erckenbrecht
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include <avr/pgmspace.h>
#include <stdint.h>

#define FS UINT32_C(8000)

#define CTCSS_TABMAX 54

extern uint8_t sin_tab[] PROGMEM;
extern uint16_t ctcss_tab[] PROGMEM;
extern volatile uint32_t samp_buf;
extern volatile uint8_t samp_count;

void tone_start_pl(unsigned int frequency);

void tone_stop_pl();

void tone_start_sel(unsigned int frequency);

void tone_stop_sel();
void dtone_start(unsigned int freq1, unsigned int freq2);

uint8_t tone_decode();
void tone_decode_stop();
void goertzel_init(uint8_t ctcss_index);

#endif /* AUDIO_H_ */