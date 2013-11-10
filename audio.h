/*
 * audio.h
 *
 *  Created on: 25.07.2012
 *****************************************************************************
 *	MCmega - Firmware for the Motorola MC micro radio
 *           to use it as an Amateur-Radio transceiver
 *
 * Copyright (C) 2013 Felix Erckenbrecht, DG1YFE
 *
 * ( AVR port of "MC70"
 *   Copyright (C) 2004 - 2013  Felix Erckenbrecht, DG1YFE)
 *
 * This file is part of MCmega.
 *
 * MCmega is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MCmega is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MCmega.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include <avr/pgmspace.h>
#include <stdint.h>

#define FS UINT32_C(8000)

#define SAMP_BUF_LEN UINT8_C(32)
#define CTCSS_TABMAX 54

extern const uint8_t sin_tab[] PROGMEM;
extern const uint16_t ctcss_tab[] PROGMEM;
extern volatile uint8_t samp_buf[SAMP_BUF_LEN];
extern volatile uint8_t samp_buf_count;
extern volatile float ge;
extern volatile float g_coeff;
extern volatile uint8_t tone_detect;
extern volatile uint8_t tone_detect_updated;

/*
	Initialize ADC in free-running mode
*/
void adc_init();

/*
	re-init Goertzel registers
*/
void tone_decode_reset();

void tone_start_pl(unsigned int frequency);

void tone_stop_pl();

void tone_start_sel(unsigned int frequency);

void tone_stop_sel();
void dtone_start(unsigned int freq1, unsigned int freq2);

uint8_t tone_decode();
void tone_decode_stop();
void goertzel_init(uint16_t ctcss_freq);




#endif /* AUDIO_H_ */
