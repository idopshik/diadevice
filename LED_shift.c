#include "LED_shift.h"

#include <avr/pgmspace.h>

 // ������ PORTB ������ ���� ShiftPort


//SH_CP   ---- CLOCK
//ST_CP  ---- LATCH
//DS  ----  Data

 /*
 10 -NO_0x00;
 11  E - 
 12  C - 
 13  � - 
 14  H - 
 15  F - 0xA0C0
 16  ������ 0xA008
 17	�����  � ������ �������
 18 - ���� � ������ �������
 19 - ����� � ������ �������
 20 - 
 21 "-"
 22 "_"
 23 TEST 0xFFFF
 24 L
 25 �
 */
 
 // �������� ���� ��� ����������� ����� ����������� ���� 595 � ��������� ����� LED ����������� ��������

const unsigned int PROGMEM NumCodes[] =
{
	0x38C8,0x808,0xB048,0x9848,0x8888,0x98C0,0xB8C0,0x848,0xB8C8,0x98C8  , 0x0000,0xB0C0, 0x3C,0x20C0,0x2298,0xA0C0,0xA008,0x8008,0x48,0x8800,0x1800,0x8000,0x1000,0xFFFF,0x3080,0x28C8// ������� ���� ������ - ��������!
};

const unsigned int PROGMEM SymbCodes[] =
{
	0x0040,0x0008,0x0800,0x1000,0x2000,0x0080,0x8000,0x848,0xB8C8,0x98C8  , 0x0000,0x23C, 0x3C,0x1C,0x2298,0xFFFF// ������� ���� ������ - ��������!
};
void PutOneDigit(unsigned char Num,unsigned char Digit,unsigned char DOT,unsigned char dataRGB_LED)
{
	uint16_t localvar;

	switch(Digit)
	{
		case 0:localvar = 0x430;							//  0000 0100 0011 0000
		break;
		case 1:localvar = 0x610;							//  0000 0110 0001 0000
		break;
		case 2:localvar = 0x620;							//  0000 0110 0010 0000
		break;
		case 3:localvar = 0x230;							//  0000 0010 0011 0000
		break;
		default: localvar = 0;
	}
	
	if(DOT)localvar|=0x4000; // ������ �����


 shift( pgm_read_word(NumCodes+Num)|localvar,dataRGB_LED) ;
}
//some comment
// ��������� ������� ��� ������ ��������� ������
void PutOneSymbol(unsigned char Num_of_Code,unsigned char Digit,unsigned char DOT,unsigned char dataRGB_LED)
{
	uint16_t localvar;
	
	switch(Digit)
	{
		case 0:localvar = 0x430;						  //  0000 0100 0011 0000
		break;
		case 1:localvar = 0x610;						  //  0000 0110 0001 0000
		break;
		case 2:localvar = 0x620;						  //  0000 0110 0010 0000
		break;
		case 3:localvar = 0x230;						  //  0000 0010 0011 0000
		break;
		default: localvar = 0;
	}
	
	if(DOT)localvar|=0x4000; // ������ �����

	shift( pgm_read_word(SymbCodes+Num_of_Code)|localvar,dataRGB_LED) ;
}
void PREshift(unsigned char dataRGB_LED)					// �������������� �������� ����� 1 (����� ������� �� ������). 
{
		for (signed char i = 7; i >= 0; i--){				// Now we are entering the loop to shift out 8+ bits

			ShiftPort &= ~(1 << SH_CP); 					// Set the serial-clock pin low

			PORTB |= (((dataRGB_LED&(0x01<<i))>>i) << DS ); 	// Go through each bit of data and output it

			ShiftPort |= (1 << SH_CP); 						// Set the serial-clock pin high

			PORTB &= ~(1 << DS );							// Set the datapin low again
		}
}

void shift( unsigned int data,unsigned char dataRGB_LED){					// ��������� ������� �����.
	uint8_t localvar;
	uint8_t j = 2;											//����������� ����� ��� ����������, ��� ��������� ���� ������!
	
	ShiftPort &= ~(1 << ST_CP); 							// Set the register-clock pin low


	/////////////////////////////////////////////////////////////
	if (dataRGB_LED){ PREshift(dataRGB_LED);data|=0x1;}		//���� �������� ���������. 0x1- ������� ��� ������� - Q0 �� 595 �����1 � ����� �� LED. 
	//////////////////////////////////////////////

	localvar = (data>>8);									// ������ ������� ����, ������� �������, ��� ���� "�������" �� ������ 595
	do
	{
		for (signed char i = 7; i >= 0; i--){				// Now we are entering the loop to shift out 8+ bits

			ShiftPort &= ~(1 << SH_CP); 					// Set the serial-clock pin low

			PORTB |= (((localvar&(0x01<<i))>>i) << DS ); 	// Go through each bit of data and output it

			ShiftPort |= (1 << SH_CP); 						// Set the serial-clock pin high

			PORTB &= ~(1 << DS );							// Set the datapin low again 
		}
		localvar = data;									// ��� ������ ��� ���� �������, � ������ ��� ���� ��������!
		j--;												// ������ �������� 1, ������ - 0, � ������� ������� �� �����.
		
	} while (j);
	
	ShiftPort |= (1 << ST_CP);								// Set the register-clock pin high to update the output of the shift-register
	}
	
	void Hardware_set_for_shift (void)
	{
		ShiftDDR |=(1<<ST_CP)|(1<<SH_CP);
		ShiftPort &=~((1<<ST_CP)|(1<<SH_CP));
		
		DDRB |= (1<<0)|(1<<4); //DS � OE
		PORTB  &=~((1<<0)|(1<<4));
		// ����� OE ������ ������ ���� �� "�����". ������ �������.
	}

