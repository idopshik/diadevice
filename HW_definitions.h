/*
 * HW_definitions.h
 *
 * Created: 05.06.2017 10:30:05
 *  Author: isairon
 */ 


#ifndef HW_DEFINITIONS_H_
#define HW_DEFINITIONS_H_


#include <avr/io.h>

 /*
 ///////////////////////////// Карта физического подключения_2.0 //////////////////////////////////


 PD4 (OC1B) - база npn VIBRO
 PB3 (OC0) - база npn Buzzer

Линия к 595
PB0 DS
PB4 EN
PB1	Latch
PB2	Schift

Кнопки, на землю, уже с конденсаторами
PA0 - вехняя 
PA1 - средняя
PA2 - нижняя 

IR3313
PD5 - gate n-chanel mosphet
PD7 - (OC2) - контур RC цепи потания базы npn биполярника, подстраивающего ток отсечки
PA4 - feedback

Stepper TLE4729G
Phase1 - PC4
Phase2 - PC5
Error1 - PC6
Error2 - PC7

Schmidt trigger
Coil1 - PС1
Coil2 - PС2

CKP emulator
PС3			.. А так же выведены и PC1 , PC2  

CMP emulator
PD2
PD3

Voltage sense - PA5
IR_out_active - PB5 - MOSI.

relay for IR - PB7 - база NPN транзистора

 */

#define SA1  PA0
#define SA2  PA1
#define SA3  PA2
#define AntyReverce_relay_desengage PORTB &= ~(1<<PB7);							//реле выключено - подаём нуль.
#define AntyReverce_relay_ON PORTB |= (1<<PB7);	

void Hardware_init (void)
{
DDRA = DDRB = DDRC = DDRD = 0x00;			//Defencive initialising
PORTA = PORTB = PORTC = PORTD = 0x00;

DDRA &=~( (1<<SA1)|(1<<SA2)|(1<<SA3) );		// Кнопки
PORTA|= (1<<SA1)|(1<<SA2)|(1<<SA3);			


DDRB |= (1<<3);								// Зуммер      
PORTB&=~(1<<3);

DDRD|= (1<<PD4);							// вибро						
DDRC |= (1<<PC1)|(1<<PC2)|(1<<PC3);			// Выводы эмулятора и на катушки (параллельно). Здесь для моргунчика

DDRB|=(1<<PB4);								// dynamic LED OE



DDRB &= ~(1<<PB5);							//IR_out_active - PB5 - MOSI.
PORTB |=(1<<PB5);							//Pull-up - это датчик - если 0, то IR включен.

DDRB |= (1<<PB7);							//IR_ antireverce Relay - (npn транзистор).
AntyReverce_relay_desengage 						//реле выключено - подаём нуль.
}


#endif /* HW_DEFINITIONS_H_ */