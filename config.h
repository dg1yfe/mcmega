/*
 * config.h
 *
 *  Created on: 01.08.2012
 *      Author: F. Erckenbrecht
 */

#ifndef CONFIG_H_
#define CONFIG_H_

typedef struct {
	unsigned channel : 13;		// 1250 Hz steps, relative to F_BASE
	unsigned shift	 : 9;		// 25 kHz steps
	unsigned shift_pos: 1;		// Sign of shift (1 = positive shift)
	unsigned shift_act: 1;		// Shift active Bit
}T_CHANNEL;


typedef struct {
	unsigned ctcss_rx_index;	// active index CTCSS rx
	unsigned ctcss_tx_index;	// active index CTCSS tx
	unsigned head : 2;			// active control head
	unsigned power_mode : 1;	// power mode (hi/lo)
	unsigned defch_save : 2; 	// channel save mode (off/auto/manual)
} T_CFG;

typedef struct {
	unsigned * conf_ptr;

	unsigned dirty;
};


#endif /* CONFIG_H_ */
