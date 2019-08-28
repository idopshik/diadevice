/*

 *  Author: isairon
// Здесь не все на одном порту. Поэтому нюанс в сишнике. Смотри сишник.
 */ 

#ifndef LED_SHIFT_H_
#include <avr/io.h>
#define LED_SHIFT_H_

#define  ShiftDDR		 DDRB
#define  ShiftPort		 PORTB
#define  DS				 PB0        // Serial data in (about 595)
#define  ST_CP			 PB1		// Storage clock (latch)
#define  SH_CP			 PB2		// Shift clock (clock)
#define  EN				 PB4		

void Hardware_set_for_shift (void);

void shift( unsigned int data,unsigned char dataRGB_LED);
void Data_Out (unsigned char d);
void PutOneDigit(unsigned char Num,unsigned char Digit,unsigned char DOT,unsigned char dataRGB_LED);
void PREshift(unsigned char dataRGB_LED);							 // Преднаполнение регистра номер 1 (будет сдвинут на третий).

void PutOneSymbol(unsigned char Num_of_Code,unsigned char Digit,unsigned char DOT,unsigned char dataRGB_LED);

#endif /* LED_SHIFT_H_ */
