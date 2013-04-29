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
