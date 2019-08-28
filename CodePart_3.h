/*
 * CodePart_3.h
 *
 * Created: 06.01.2018 11:57:12
 *  Author: isairon
 */ 

// CKP_EMUL_ModusFunction


#ifndef CODEPART_3_H_
#define CODEPART_3_H_



void car2(void)
{
	// ������ ���������. 
	// ����� ����� ���������� SelCar ����������� ������ ������ ������ � � ���������� TIMER2_COMP ����� case ������� ����.
}
void vaz2108(void)
{
	
	VersatileDevider++;
	if(VersatileDevider==18)PORTC &= ~((1<<PC1)|(1<<PC2));		//������ �� ������� ������ �KP ������������� ����� CMP
	if(VersatileDevider==116)PORTC |= (1<<PC1)|(1<<PC2);		// ���� CMP
	if(VersatileDevider>120)Emul_temp_dummy_var=1;				// �� ����� ����� � ���� - ������� ������ (������ ����)
	if(VersatileDevider==125){VersatileDevider=1; Emul_temp_dummy_var = 0;}		//������ ��� ������ �������.

	if(Emul_temp_dummy_var)
	{
		PORTC|=(1<<3);
		Emul_temp_dummy_var = 0;
		
	}
	else									//ULN �����������. ������ �������� �� ������ ����.
	{
		Emul_temp_dummy_var = 1;
		PORTC &= ~(1<<3);		//������� ��� ���������
	}
}




static void SelCar(void)
{
	if(BtnTp)Car_to_emulate--;								// ������ ������ 1 (���� �� ����)
	else Car_to_emulate++;									// �������� �� ������ ����.

	if(!Car_to_emulate) Car_to_emulate = 1;					// ���� ���������� �� ����, ������ ������������ ����� � ����.
	if(Car_to_emulate>1)Car_to_emulate = 1;					// ���� ������ ���������� ����� � ���� (���� ���� :-
	LED_string[0]=LED_string[1]=LED_string[2] =10;		//������� ������ �����
	LED_string[3] = Car_to_emulate;
}



// ������� ������ � ��������� ������� (������� ���� � �������� �����������).
static void Set_RPM_for_CKP(void)

{
	// 124 ������ ������� ��� ���� ������ ��� ����.
	//������ ��� ��������.
	if(BtnTp)
	{OCR2++;
	}
	else // �������� �� ������ ����.
	{
		OCR2--;
	}
	
	// ������� ������� ��� ����

	// ����� ������� CTC � ��� ������������
	uint16_t rpm;
	rpm =  (F_CPU/(2*62*4))/OCR2; //	����� 2 ������ ��� �� ��� ��� ������ � 62 ���� �� ��������. ������ ��� 3 �� �����
	
	
	
	Common_LED_4_out(rpm);
	DotPrintByte&=0xF0;				// ��� �����.

	 
	 #if defined( DebugCommon )
	 uart_put_int(OCR2);
	 uart_new_line_Macro
	 uart_put_int(rpm);
	 uart_new_line_Macro
	 #endif
}

static void Enter_CKP_emulator_Modus(void)
{
	// ���������� ������, �������� ���. 1000 x 60 x 2������. -2000 ��.
	// 10 ��� ��� ��� 20000 ��, ��� 0.02 �Hz 

	// ������ �������������
	TIMSK &= ~(1<<OCIE2);								 // ������ �� ����������
	Double_LED_out = 0x00;								 //����������, ��� �����������


	// ����������������� ������ 2
	TCCR2 = 0;											 // ������������� �������
	TCCR2|= (1<<WGM21);									 // ����� �� ���������� (CTC).
	TCCR2 |= (1<<CS21)|(1<<CS22);						 // 256. 244��. // ������������ ��� 256 �������. 16MHz

	OCR2 = 30;
	// ������

	DDRC|=(1<<3); // ��P ��������
	DDRC|=(1<<1); // CMP
	DDRC|=(1<<2); // �MP

	
	 #if defined( DebugCommon )
	  uart_puts("CKP/CMP_emul_Modus_activated "); uart_new_line_Macro
	 #endif
	
	
	SelCar(); // ����� ����� ���������� ����� �����.
}




#endif /* CODEPART_3_H_ */