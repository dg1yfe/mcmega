/*
 * io.c
 *
 *  Created on: 26.05.2012
 *      Author: Felix Erckenbrecht
 */
#include <avr/io.h>

#include "firmware.h"
#include "regmem.h"

/*
 * ;****************
; I O   I N I T
;****************
;
; Initialisiert I/O :
;                     - Port2 DDR ( SCI TX, PLL T/R Shift )
;                     - Port6 DDR ( PTT Syn Latch )
;                     - Port5 DDR ( EXT Alarm )
;                     - Port5 Data ( EXT Alarm off (1) )
;                     - RP5CR (HALT disabled)
;                     - I2C (Clock output, Clock=0)
;                     - Shift Reg (S/R Latch output, S/R Latch = 0, S/R init)
;                     - SCI TX = 1 (Pullup)
io_init
                aim  #%11100111,RP5CR     ; nicht auf "Memory Ready" warten, das statische RAM ist schnell genug
                                          ; HALT Input auch deaktivieren, Port53 wird später als Ausgang benötigt

                ldab #%10110100
                stab Port2_DDR_buf             ; Clock (I2C),
                stab Port2_DDR                 ; SCI TX, T/R Shift (PLL), Shift Reg Latch auf Ausgang

; I2C Init
                aim  #%11111011,Port2_Data     ; I2C Clock = 0
;ShiftReg Init
                aim  #%01111111,Port2_Data     ; Shift Reg Latch = 0
;SCI TX
                oim  #%10000, Port2_Data       ; SCI TX=1

                clr  SR_data_buf               ; Shuft Reg Puffer auf 0 setzen

                ldaa #~(SR_RFPA)               ; disable PA
                ldab #(SR_nTXPWR + SR_nCLKSHIFT + SR_9V6)
                                               ; disable Power control, disable clock shift, enable 9,6 V
                jsr  send2shift_reg

; Port 5
                ldab #%00001000
                stab SQEXTDDR                  ; EXT Alarm auf Ausgang, Alles andere auf Input
                stab SQEXTDDRbuf
                oim  #%00001000, SQEXTPORT     ; EXT Alarm off (Hi)

; Port 6
                ldab #%00001100
                stab Port6_DDR_buf
                stab Port6_DDR                 ; A16 (Bank Switch), PTT Syn Latch auf Ausgang

                aim  #%10011011, Port6_Data    ; Bank 0 wählen

                clr  led_buf
                clr  arrow_buf
                clr  arrow_buf+1

                clr  sql_ctr
                clr  ui_ptt_req             ;
                rts


 *
 */

void io_init()
{
	/*
	 *  Port A:
	 *  Bit 0-2 : PL Encode
	 *  Bit 3	: F1 / #F2  (Input from Control Panel, parallel to SCI RX)
	 *  Bit 4-5 : n.c.
	 *  Bit 6	: HUB
	 *  Bit 7	: Test
	 */

	DDRA  = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
	PORTA = (1 << 2);

	/*
	 * Port B:
	 * Bit 0	: CSQ/UNSQ (Input from Control Panel)
	 * Bit 1-3  : SPI (SCK,MOSI,MISO)
	 * Bit 4	: Alert Tone (OC0)
	 * Bit 5	: Data Inhibit (clamp Signal Decode Input to GND)
	 * Bit 6	: Tx/Busy (Control Panel Reset)
	 * Bit 7	: Syn_Latch
	 */

	DDRB  = (1 << 4) | ( 1 << 5 ) | ( 1 << 6) | ( 1 << 7 );
	PORTB = (1 << 4);

	/*
	 * Port C:
	 * Bit 0 - 3 : SEL_ENC (4-Bit DAC / Sel Call)
	 * Bit 4 - 7 : n.c.
	 */

	DDRC  = 0x0f;
	PORTC = 3;

	/*
	 * Port D:
	 * Bit 0	: SWB+
	 * Bit 1	: PTT (input)
	 * Bit 2-3	: UART1
	 * Bit 4	: Signalling Decode (IC1)
	 * Bit 5	: Clock
	 * Bit 6	: Data out
	 * Bit 7	: DPTT
	 */

	DDRD  = ( 1 << 5) | ( 1 << 6 ) | ( 1 << 7 );
	PORTD = 0;

	/*
	 * Port E:
	 * Bit 0-1	: UART0 (SCI to Control Head)
	 * Bit 2	: Data In
	 * Bit 3	: Call LED
	 * Bit 4	: #NMI  (INT4)
	 * Bit 5	: #STBY (INT5)
	 * Bit 6	: Lock Detect (INT6)    ( 1 = Lock )
	 * Bit 7	: Squelch Detect (INT7) ( 1 = Signal)
	 */

	DDRE  = ( 1 << 3 );
	PORTE = 0;

	/*
	 * Port F:
	 * Bit 0-2	: n.c.
	 * Bit 3	: Signalling Decode (ADC3)
	 * Bit 4-7	: JTAG (TCK, TMS, TDO, TDI)
	 */

	DDRF  = 0;
	PORTF = 0;

	/*
	 * Port G:
	 * Bit 0	: Call SW
	 * Bit 1	: Emergency
	 * Bit 2	: n.c.
	 * Bit 3	: SR Latch
	 * Bit 4	: #Mem enable
	 */

	PORTG = (1 << 4);
	DDRG  = (1 << 3) | (1 << 4);

}



void SetShiftReg(char or_value, char and_value)
{
	static char SR_data_buf;
	char i;

    // 0 - Audio PA enable (1=enable)      (PIN 4 ) *
    // 1 - STBY&9,6V                       (PIN 5 )
    // 2 - T/R Shift                       (PIN 6 ) *
    // 3 - Hi/Lo Power (1=Lo Power)        (PIN 7 ) *
    // 4 - Ext. Alarm                      (PIN 14) *
    // 5 - Sel.5 ATT   (1=Attenuated Tones)(PIN 13) *
    // 6 - Mic enable  (1=enable)          (PIN 12) *
    // 7 - Rx Audio enable (1=enable)      (PIN 11)

	// get exclusive Bus access
	xSemaphoreTake(SerialBusMutex, portMAX_DELAY);

	//
	SR_data_buf &= and_value;
	SR_data_buf |= or_value;

	taskENTER_CRITICAL();
	for (i=0;i<8;i++)
	{

	}
	taskEXIT_CRITICAL();

}

/*
 * ;****************************
; S E N D 2 S h i f t _ R e g
;****************************
;
; AND before OR !
;
; Parameter : B - OR-Value
;             A - AND-Value
;
; changed Regs: A,B,X
;
send2shift_reg
                inc  bus_busy                ; disable IRQ Watchdog Reset
                inc  tasksw_en

                anda SR_data_buf
                staa SR_data_buf
                orab SR_data_buf
                stab SR_data_buf

;                jsr  i2c_tx

                ldaa #8                 ; 8 Bit senden
s2sr_loop
                psha                    ; Bitcounter sichern
                lslb                    ; MSB in Carryflag schieben
                bcs  s2sr_bitset        ; Sprung, wenn Bit gesetzt
                I2C_DL                  ; Bit gelöscht, also Datenleitung 0
                I2C_CH
                I2C_CL                  ; Clock Hi/Lo toggle
                bra  s2sr_dec
s2sr_bitset
                I2C_DH                  ; Data Hi
                I2C_CH
                I2C_CL                  ; Clock Hi/Lo toggle
s2sr_dec
                pula
                deca                    ; A--
                bne  s2sr_loop
                I2C_DI                  ; Data auf Input & Hi


                oim  #$80, Port2_Data
                aim  #$7F, Port2_Data            ;Shift Reg Latch toggeln - Daten übernehmen

                dec  bus_busy                ; disable IRQ Watchdog Reset
                dec  tasksw_en
                rts
 *
 */



void sci_init()
{
	/* Set baud rate to 1200 7e1 */
	/* UBRR = Sysclk / (16 * Bitrate) -1 */
	/* UBRR = 4924800 Hz / (16 * 1200 1/s) - 1 = 255,5 */
	UBRR0H = 0;
	UBRR0L = 255;	// ca 1202 Bit / s

	/* Enable receiver and transmitter
	 * Disable RX interrupt */
	UCSR0B = (0 << RXCIE0) | (1<<RXEN0)|(1<<TXEN0);

	/* Set frame format: 7data, 1stop bit */
	UCSR0C = (1 << UPM01) |
			 (0 << UPM00) |
			 (0 << USBS0) |
			 (1 << UCSZ01)|
			 (1 << UCSZ00);

}
