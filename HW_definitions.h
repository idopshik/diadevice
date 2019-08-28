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
 ///////////////////////////// ����� ����������� �����������_2.0 //////////////////////////////////


 PD4 (OC1B) - ���� npn VIBRO
 PB3 (OC0) - ���� npn Buzzer

����� � 595
PB0 DS
PB4 EN
PB1	Latch
PB2	Schift

������, �� �����, ��� � ��������������
PA0 - ������ 
PA1 - �������
PA2 - ������ 

IR3313
PD5 - gate n-chanel mosphet
PD7 - (OC2) - ������ RC ���� ������� ���� npn �����������, ��������������� ��� �������
PA4 - feedback

Stepper TLE4729G
Phase1 - PC4
Phase2 - PC5
Error1 - PC6
Error2 - PC7

Schmidt trigger
Coil1 - P�1
Coil2 - P�2

CKP emulator
P�3			.. � ��� �� �������� � PC1 , PC2  

CMP emulator
PD2
PD3

Voltage sense - PA5
IR_out_active - PB5 - MOSI.

relay for IR - PB7 - ���� NPN �����������

 */

#define SA1  PA0
#define SA2  PA1
#define SA3  PA2
#define AntyReverce_relay_desengage PORTB &= ~(1<<PB7);							//���� ��������� - ����� ����.
#define AntyReverce_relay_ON PORTB |= (1<<PB7);	

void Hardware_init (void)
{
DDRA = DDRB = DDRC = DDRD = 0x00;			//Defencive initialising
PORTA = PORTB = PORTC = PORTD = 0x00;

DDRA &=~( (1<<SA1)|(1<<SA2)|(1<<SA3) );		// ������
PORTA|= (1<<SA1)|(1<<SA2)|(1<<SA3);			


DDRB |= (1<<3);								// ������      
PORTB&=~(1<<3);

DDRD|= (1<<PD4);							// �����						
DDRC |= (1<<PC1)|(1<<PC2)|(1<<PC3);			// ������ ��������� � �� ������� (�����������). ����� ��� ����������

DDRB|=(1<<PB4);								// dynamic LED OE



DDRB &= ~(1<<PB5);							//IR_out_active - PB5 - MOSI.
PORTB |=(1<<PB5);							//Pull-up - ��� ������ - ���� 0, �� IR �������.

DDRB |= (1<<PB7);							//IR_ antireverce Relay - (npn ����������).
AntyReverce_relay_desengage 						//���� ��������� - ����� ����.
}


#endif /* HW_DEFINITIONS_H_ */