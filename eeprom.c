/*
 * eeprom.c
 *
 *  Created on: 12.07.2012
 *      Author: F. Erckenbrecht
****************************************************************************
;
;    MC 70    v1.6   - Firmware for Motorola mc micro trunking radio
;                      for use as an Amateur-Radio transceiver
;
;    Copyright (C) 2004 - 2012  Felix Erckenbrecht, DG1YFE
;
;
****************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <util/crc16.h>
#include <alloca.h>

#include "regmem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "io.h"
#include "display.h"

#include "macros.h"
#include "regmem.h"
#include "firmware.h"


char eep_read(char flags, char * data);

/*
;***************************
; E E P   R A N D   R E A D
;***************************
;
; Liest ein beliebiges Byte aus dem gew�hlten EEPROM
;
;
; Parameter: B - Datenadresse im EEPROM (Byte Adresse)
;            A - NoSTOP-Flag(MSB), Device&Page Adresse (3 Bit, LSB)
;                Wenn das MSB gesetzt ist, wird KEINE STOP Condition
;                generiert.
;
; Ergebnis : B - gelesenes Byte
;            A - Status: 0 - OK
;                        1 - Kein ACK nach Device/Pageadresse (kein EEPROM mit der Adresse vorhanden?)
;                        2 - Kein ACK nach Byteadresse
;                        3 - Kein ACK nach Device/Pageadresse in "eep_current_read"
;
;
; changed Regs : A,B
;
 *
 */


char eep_rand_read(unsigned int address, char * data)
{
	char buf;
	char err;

	xSemaphoreTakeRecursive(SerialBusMutex, portMAX_DELAY);
	tasksw_en++;

	i2c_start();

	buf = (address >> 7) & 0xe;
	buf |= 0xa0;

	i2c_tx(buf);
	if((err=i2c_tstack()))
		return 1;

	i2c_tx( (char) address);
	if((err=i2c_tstack()))
		return 2;

	buf = (address >> 8);
	buf |= 0x80;		// No stop flag
	err=eep_read(buf, data);

	if(err)
		return err;

	if(!(address && 0x8000))
		i2c_stop();

	err = 0;
	tasksw_en--;
	xSemaphoreGiveRecursive(SerialBusMutex);

	return 0;
}


/*
;*****************
; E E P   R E A D
;*****************
;
; Liest das Byte an der aktuellen Adresse im gew�hlten EEPROM mittels "current address read"
;
;
; Parameter: A - NoSTOP-Flag(MSB), Device&Page Adresse (3 Bit, LSB)
;                Wenn das MSB gesetzt ist, wird KEINE STOP Condition
;                generiert.

;
; Ergebnis : B - gelesenes Byte
;            A - Status: 0 - OK
;                        3 - Kein ACK nach Device/Pageadresse
;
 */
char eep_read(char flags, char * data)
{
	char buf;

	xSemaphoreTakeRecursive(SerialBusMutex, portMAX_DELAY);
	tasksw_en++;

	// Start I2C comm
	i2c_start();

	// shift device/page address, include read bit and 0101 nibble
	buf = flags<<1;
	buf &= 0xe;
	buf |= 0xa1;

	i2c_tx(buf);
	if(i2c_tstack())
		return 3;

	// read byte from I2C
	*data = i2c_rx();

	if(!(flags & 0x80))
		i2c_stop();		// Send I2C stop, if flag was not set

	xSemaphoreGiveRecursive(SerialBusMutex);
	tasksw_en--;
	return 0;
}

/*
 *
;******************************
; E E P   S E Q   R E A D
;******************************
;
; Transferiert einen Block vorgegebener Gr��e aus dem/den EEPROM(s) ins RAM,
; dabei wird innerhalb einer Page mittels "sequential read" gelesen, was einen
; Geschwindigkeitsvorteil gegen�ber Random Read bringt, da Device- ,Page- und Startadresse
; nur einmal pro Page transferiert werden m�ssen.
;
; EEP SEQ READ funktioniert device�bergreifend: Mehrere EEPROMs werden als ein Speicherblock
; betrachtet, solange ihre Deviceadressen entsprechend gesetzt sind.
;
;
; Parameter: A - Device&Page Adresse (3 Bit)
;            B - Startadresse
;            X - Bytecount
;            STACK - Zieladresse (im RAM)
;
; Ergebnis : A - Status: 0 - OK
;                        1 - Kein ACK nach Device/Pageadresse
;                           (kein EEPROM mit der Adresse vorhanden?)
;                        2 - Kein ACK nach Byteadresse
;                        3 - Kein ACK nach Device/Pageadresse in "eep_current_read"
;
;            X - Zahl der tats�chlich gelesenen Bytes
;
 *
 */
char eep_seq_read(unsigned int address, unsigned int bytecount,
					void * dest, unsigned int  * bytesread)
{
	char buf;
	char err;

	xSemaphoreTakeRecursive(SerialBusMutex, portMAX_DELAY);
	tasksw_en++;

	if(bytesread)
		*bytesread = 0;

	err = 0;
	while(!err && bytecount)
	{
		// begin with random read to set address
		if((err=eep_rand_read(address | 0x8000, (char *) dest)))
			return err;
		// increase destination address, and decrease bytecount
		dest++;
		bytecount--;
		if(bytesread)
			(*bytesread)++;
		// while there are bytes to copy
		while(bytecount)
		{
			// check if address reached a page or device boundary
			if(!(++address && 0xff))
			{	// send I2C stop on page boundary, continue with random read
				i2c_stop();
				break;
			}
			else
			{	// send ack
				i2c_ack();
				// read next byte using sequential read
				buf = i2c_rx();
				*((char *)dest++) = buf;

				bytecount--;
				if(bytesread)
					(*bytesread)++;
			}
		}
		i2c_stop();
	}
	xSemaphoreGiveRecursive(SerialBusMutex);
	tasksw_en--;
	return 0;
}


/*
;*****************************
; E E P   W R I T E
;*****************************
;
; Parameter: B - zu schreibendes Byte
;
;            STACK (1 / 6) - Datenadresse im EEPROM (Byte Adresse)
;            STACK (0 / 5) - Device&Page Adresse (3 Bit, LSB)
;
; Ergebnis : A - Status: 0 - OK
;                        1 - Kein ACK nach Device/Pageadresse
;                           (kein EEPROM mit der Adresse vorhanden?)
;                        2 - Kein ACK nach Byteadresse
;                        3 - Kein ACK nach Datenbyte
;                        4 - Timeout beim ACK Polling nach Schreibvorgang
 */
char eep_write(unsigned int address, char * data)
{
	char buf;
	char err;

	xSemaphoreTakeRecursive(SerialBusMutex, portMAX_DELAY);
	tasksw_en++;

	i2c_start();

	buf = ((address >> 7) & 0xae) | 0xa0;
	i2c_tx(buf);

	if((err=i2c_tstack()))
		return 1;

	buf = (char) address;
	i2c_tx(buf);

	if((err=i2c_tstack()))
		return 2;

	i2c_tx(*data);

	if((err=i2c_tstack()))
		return 3;

	i2c_stop();

	// wait at max 11 ms for write to finish
	ui_timer=11;
	err=1;
	while(ui_timer && err)
	{
		i2c_start();
		buf = ((address >> 7) & 0xae) | 0xa0;
		i2c_tx(buf);
		// check for ACK (ACK = write finished)
		err=i2c_tstack();
	}
	if(err)
		err = 4;

	i2c_stop();
	tasksw_en--;
	xSemaphoreGiveRecursive(SerialBusMutex);

	return err;
}


/*
 *
;***************************
; E E P   W R I T E   S E Q
;***************************
;
; Mehrere Bytes hintereinander schreiben (maximal 256)
;
; Parameter: STACK(4)(9) - Datenadresse im Speicher
;            STACK(3)(8) - Datenadresse im EEPROM (Byte Adresse)
;            STACK(2)(7) - Device&Page Adresse (3 Bit, LSB)
;            STACK(0)(5) - Bytecount
;
; Ergebnis : A - Status: 0 - OK
;                        1 - Kein ACK nach Device/Pageadresse
;                           (kein EEPROM mit der Adresse vorhanden?)
;                        2 - Kein ACK nach Byteadresse
;                        3 - Kein ACK nach Datenbyte
;                        4 - Timeout beim ACK Polling nach Schreibvorgang
;
;
 *
 */
char eep_write_seq(unsigned int address, char bytecount, void * data)
{
	char err;

	xSemaphoreTakeRecursive(SerialBusMutex, portMAX_DELAY);
	tasksw_en++;

	err=0;

	while(bytecount-- & !(err))
	{
		err = eep_write(address++, data++);
	}

	tasksw_en--;
	xSemaphoreGiveRecursive(SerialBusMutex);

	return err;

}

unsigned int eep_get_size()
{
	unsigned int addr;
	char dummy;

	addr = 0;

	do
	{
		if(eep_rand_read(addr, &dummy))
			break;
		addr += 0x80;
	}while(addr < 0x0800);

	return addr;
}


char eep_chk_crc()
{
	return 0;
}

char eep_write_crc()
{
	return 0;
}

char eep_rd_ch_freq(uint8_t slot, unsigned long * f)
{
	void * buf;
	char err;
	long fbuf;

	buf = alloca((size_t)10);

	if(slot > 24)
		return -1;

	slot = slot * 10;
	slot += 0x100;
	err = eep_seq_read(10, slot, buf, NULL);

	if(err)
		return(err);

	// get channel
	fbuf = *((uint16_t *) buf);
	fbuf >>=3;		// 13 significant Bits
	fbuf *= 1250;	// multiply by 1250 to obtain frequency
	fbuf += FBASE; // add Base frequency
	*f = fbuf;

	return 0;

}

/*

;***********************
; E E P   C H K   C R C *BROKEN*
;***********************
;
; Bildet CRC �ber den Config Bereich
;
; Parameter: keine
;
; Ergebnis : A - Read Status :  0 = OK
;                               1 = CRC Error
;                              >8 = Lesefehler:
;
;
;
;
;            X - CRC
;
; changed Regs : A, X
;
eep_chk_crc
                pshb
                tsx
                xgdx
;                subd #EP_CONF_MEM  ; Platz f�r Config auf dem Stack schaffen
                xgdx
                txs
                ldx  #0            ;
                pshx               ; CRC auf 0 initialisieren

                xgdx               ; Start im EEPROM bei Adresse 0 (EEPROM Adresse in A:B)

                tsx
                xgdx
 ;               subd #EP_CONF_MEM
                xgdx
                txs                ; 52 Byte auf dem Stack reservieren
                pshx               ; Adresse auf Stack
;                ldx  #EP_CONF_MEM  ; 52 Byte lesen (nur den Config Bereich + CRC)
                jsr  eep_seq_read  ; Block lesen
                ins
                ins                ; Adresse vom Stack l�schen
;                cpx  #EP_CONF_MEM  ; komplette Config gelesen ?
                bne  ecc_read_err  ; Nein? Dann Fehler
                xgdx               ; Bytecount nach D
                tsx                ; Startadresse im RAM
                pshb               ;
                ldab #2            ; f�r CRC Calc
                abx                ; berechnen (Daten liegen auf Stack)
                pulb               ; B wiederholen (Bytecount/LoByte)
                                   ; Auf Stack liegt CRC Init Wert bzw.
                jsr  crc16         ; CRC �ber den Block berechnen
                xgdx               ; CRC nach X
                cpx  #0            ; CRC =0?
                bne  ecc_crc_err   ; Nein? Dann CRC Error
                clra               ; kein Lese Fehler aufgetreten
ecc_end
                tsx
                xgdx
;                addd #EP_CONF_MEM+2; Stack bereinigen
                xgdx
                txs
                pulb               ; B wiederherstellen
                rts
ecc_read_err
                oraa #8            ; Lesefehler
                bra  ecc_end
ecc_crc_err
                ldaa #1
                bra  ecc_end

;***************************
; E E P   W R I T E   C R C *BROKEN*
;***************************
;
; CRC16 �ber Config Block bilden und schreiben
;
; Parameter: keine
;
; Ergebnis : X - CRC
;          : A - Read Status :   0 = OK
;                             >= 8 = Read-Error
;                             >=16 = Write-Error
;
eep_write_crc
                pshb
                psha
                pshx
                tsx
                xgdx
                subd #50           ; 50 Byte Platz auf dem Stack schaffen
                xgdx
                txs
                ldx  #0            ;
                pshx               ; CRC auf 0 initialisieren
                xgdx               ; Start im EEPROM bei Adresse 0 (EEPROM Adresse in A:B)
ewc_loop
                pshb               ; Daten aus EEPROM sollen auf
                tsx                ;
                xgdx
                subd
                ldab #4            ; den Stack gelegt werden,
                abx                ; dazu die Adresse berechnen
                pulb
                pshx               ; Bytecount auf Stack
                ldx  #50           ; 50 Byte lesen (nur den Config Bereich)
                jsr  eep_seq_read  ; Block lesen
                ins
                ins                ; Bytecount vom Stack l�schen
                cpx  #50           ; 50 Bytes gelesen ?
                bne  ewc_read_err  ; Nein? Dann Fehler
                xgdx               ; Bytecount nach D
                tsx                ; Startadresse im RAM
                pshb               ;
                ldab #2            ; f�r CRC Calc
                abx                ; berechnen (Daten liegen auf Stack)
                pulb               ; B wiederholen (Bytecount/LoByte)
                                   ; Auf Stack liegt CRC Init Wert
                jsr  crc16         ; CRC �ber den Block berechnen
                                   ; CRC liegt auf Stack, zuerst LoByte ins EEPROM schreiben
                pulb
                ldx  #50
                pshx
                jsr  eep_write     ; Byte schreiben
                pulx
                tsta
                bne  ewc_write_err ; Schreibfehler
                pulb
                ldx #51
                pshx
                jsr  eep_write     ; Byte schreiben
                pulx
                tsta
                bne  ewc_write_err ; Schreibfehler
ewc_end
                tsx
                xgdx
                addd #51           ; Stack bereinigen
                xgdx
                txs
                pula
                pulb
                pulx
                rts
ewc_read_err
                oraa #$8
                bra  ewc_end
ewc_write_err
                oraa #$10
                bra  ewc_end


*/

