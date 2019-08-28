/*
 * CodePart_2.h
 *
 * Created: 06.01.2018 11:55:03
 *  Author: isairon
 */ 


#ifndef CODEPART_2_H_
#define CODEPART_2_H_

//PowerOutModusFunction


uint8_t I_shutdown_normal_OCR2;   //����������� ��������, ����� ������� � ����� ��������� �� ����������
uint8_t I_shutdown_inrush_OCR2;	 //������������ ��� ���� �� ������
uint8_t Cold_filament_timer;


void Ir_3313_Start (void)
{
	// �������, ���� ���� ���. ��������� ���� �����������.
	// ���������, ���� �� ������� (���� ���� ��������� - �������, � ��� �� ������ �������� ��������� �������.
	// if( OCR2 < 24 ); � ��� �� IR_Filament_setted
	if (!(IR_ctrl_Byte&IR_outActive))		//���� ��� �� ������� (����������).
	{

			if(IR_ctrl_Byte&IR_Filament_setted)
			{//������������ ��������
				// ���������� ����������� ����������� ���������� ��������
				IR_ctrl_Byte|= IR_Filament_Cold_State; // ���� ��������
				
				OCR2 = I_shutdown_inrush_OCR2;												// �������� ����������� ���������!
				Cold_filament_timer = 150; // ��� ������ ���� ������ ���� ���.
		
				IR_ON	// ��������� ������ ����� ��������� �������

				Debug_green_ON
				Debug_yellow_OFF

				RestartDebounce();								// ����� �������� ������� ������������ �� ���������� IR (������ ��� ��� plain-������ ���� ����, ��� ��� ������).
				// ����� ������������ ���������� �� ��� ������� ����������
			}
			else
			{
			 OCR2 = I_shutdown_normal_OCR2 ;
			 IR_ON // ��� ��������
			
			// uart_puts("IR_Start ");	uart_new_line_Macro
			 
			 }

	}

}
uint8_t Cold_filament_timer;
// ���������� �������� ������� � I_shutdown_inrush_OCR2 �� I_shutdown_normal_OCR2
void Inrush_current_handler (void)
{
	if(IR_ctrl_Byte&IR_Filament_Cold_State)
	{
		// ���� ��������
		Cold_filament_timer--;				// ��������� �������.
		
		// ������ �������.
		if(Cold_filament_timer< 5)
		{ OCR2 = I_shutdown_normal_OCR2;
			IR_ctrl_Byte &= ~(IR_Filament_Cold_State);
			Debug_green_OFF
			Debug_yellow_ON
		}
		else if(Cold_filament_timer == 30 ) OCR2 -= 5;
		else if(Cold_filament_timer == 60) OCR2 -= 5;
		else if(Cold_filament_timer == 90) OCR2 -= 5;
		else if(Cold_filament_timer == 120) OCR2 -= 4;
		// �������� � 250!
		
		//������ ���������� ��� ������� ������ �� ������������� ��������� �����, ����� �� ��������� �� ���������
		
		

	}
	
}



static void IR3313_plain_switch()							//plain
{
	if(BtnTp)
	{
		IR_OFF;												// ��������� � �������, �� ������ ��� �� ��������.
		Double_LED_out = 0x00;								//LED ��� ����������� ������ ���� ��� IR_ON_bit_PWM ����������. ������ ������ ����.

		G_ADC_byte &= ~(1<<Current_sense_ON);				// ��������� ��������� ����
		IR_ctrl_Byte&=~IR_OVERCURRENT;//  ������� ���� ������
		
		IR_ctrl_Byte &= ~IR_ON_bit_plain;		// ���� �� ������� PLAIN ?	//������� ��������� ������������. 	

	}
	else													// �������� �� ������ ����.
	
	{
		Start_current_measuring();												// �������� ����� ���������
		Blincked_Digits = 0;
		
		Ir_3313_Start();
		IR_ctrl_Byte |= IR_ON_bit_plain;
		RestartDebounce();								// ����� �������� ������� ������������ �� ���������� IR
	}

}




static void IR3313_PWM_switch()							//pwm
{
	if(BtnTp)
	{
		IR_ctrl_Byte &= ~IR_ON_bit_PWM;			
		IR_OFF;												// ��������� � �������, �� ������ ��� �� ��������.
		Double_LED_out = 0x00;								//LED ��� ����������� ������ ���� ��� IR_ON_bit_PWM ����������. ������ ������ ����.
	// ������� ������ � ����������� ���������
		
		
		G_ADC_byte &= ~(1<<Current_sense_ON);				// ��������� ��������� ����
		IR_ctrl_Byte&=~IR_OVERCURRENT;//  ������� ���� ������. ��� ����� ���. 

	}
	else													// �������� �� ������ ����.
		
	{
		Start_current_measuring();												// �������� ����� ���������
		Blincked_Digits = 0;
		Ir_3313_Start();								// ��� ����, ����� ��������� ������ ������ ����� ����� ������.								
		IR_ctrl_Byte |= IR_ON_bit_PWM;								// ��������������� ��������� � ����������� ������� 1.
		RestartDebounce();								// ����� �������� ������� ������������ �� ���������� IR

	}

}



static void Show_OverCurrent_limit_on_LED_disp(void)
{		

	G_ADC_byte &= ~(1<<0);			   // ������� ���� - ���� ����������� � main	

											
			uint16_t CurLimShow;
			uint8_t tmp ;

			// ����������� ��������� � ������� ����������� �������.
			for (tmp=0;tmp<16;tmp++)			// ������� �� ���������� ������ �������
			{
				if(I_shutdown_normal_OCR2 > Current_Limits_PWM_OCR_values[tmp])continue;		// ���������� ��������� �� �������
				else if (I_shutdown_normal_OCR2 == Current_Limits_PWM_OCR_values[tmp])break;   //������, �� ��� �����
				else
				{
					tmp --;
					break;
				}   // �����������.
				
			}

		
I_shutdown_inrush_OCR2 = (Inrush_currents_mass[tmp]);				// ��������� ������ ��� �����
		

// ���������� �� ������� ������ ����������

CurLimShow = (Current_Limits_Actual_Currents[tmp]); // ����������� �� ������, �������� ������.uart_put_int(CurLimShow); 

		Common_LED_4_out(CurLimShow);				// ���������� ��������
		DotPrintByte&=0xF0;							// ��� �����.
		LED_string[0] = 10;							//�� ����� ���� ������
		LED_string[1] = 10;
		


//....................// �������
// � UART ���������� ������
 #if defined( DebugCommon )
uart_put_int(CurLimShow); uart_puts(" A - static resistance"); uart_new_line_Macro			// debug only
 #endif


// ����� � UART ���������� ������ ��� �����. ������� ���
for (tmp=0;tmp<16;tmp++)			// ��������� 256 � �������� ������ �� �������� ������
{
	if(I_shutdown_inrush_OCR2 > Current_Limits_PWM_OCR_values[tmp])continue;		// ���������� ��������� �� �������
	else if (I_shutdown_inrush_OCR2 == Current_Limits_PWM_OCR_values[tmp])break;   //������, �� ��� �����
	else
	{
		tmp --;
		break;
	}   // �����������.
	
}

CurLimShow = (Current_Limits_Actual_Currents[tmp]); // ����������� �� ������, �������� ������.
 #if defined( DebugCommon )
uart_put_int(CurLimShow); uart_puts(" A - inrush for filament lamps"); uart_new_line_Macro			// debug only
 #endif

//....................






}	


static void Set_OverCurrent_limit(void)						// 2 - ��������� ������ ����������� ���� ����������
{
	G_ADC_byte&= ~(1<<Current_sense_ON);					// ��������� ���������� ���, ����� ������ ����� ����.
	if(!BtnTp)
	{if(I_shutdown_normal_OCR2<30)I_shutdown_normal_OCR2++;}	
	else
	{if(I_shutdown_normal_OCR2>12)I_shutdown_normal_OCR2--;} 

	eeprom_update_byte(&EE_OverCurrent_limit_OCR2_value, I_shutdown_normal_OCR2);					// EEPROM!

	
 #if defined( DebugCommon )
uart_put_int(I_shutdown_normal_OCR2); uart_new_line_Macro					// debug only
 #endif
	Show_OverCurrent_limit_on_LED_disp();					// ���������� � ������ � ������ normal � inrush ������� �����

}

static void Enter_Set_OverCurrent_limit(void)	
{
	G_ADC_byte&= ~(1<<Current_sense_ON);					// ��������� ���������� ���, ����� ������ ����� ����.
  I_shutdown_normal_OCR2 = eeprom_read_byte(&EE_OverCurrent_limit_OCR2_value);

  #if defined( DebugCommon )
 uart_puts("OCR2_readed_from_eeprom - ");uart_put_int(I_shutdown_normal_OCR2);	uart_new_line_Macro
  #endif
 Show_OverCurrent_limit_on_LED_disp();

 AntyReverce_relay_ON										// ��� ���� ����� - �� �����.
 OCR2 = I_shutdown_normal_OCR2 ;						// ����� OCR2 �����, ����� ������������ ������ �� ������ ���������� � �� ���� ������� ������������ ��� ���������

}


														// 3 - ��������� ������ ���������� �������
static void Show_Duty_for_IR_OUT(void)
{

//��������� ��������� ��������� ����
G_ADC_byte&= ~(1<<Current_sense_ON);				// ��������� ���������� ���. �� ��� � ���������.

	
  #if defined( DebugCommon )
uart_put_int(IR_Duty);  uart_new_line_Macro			// debug only
  #endif
	Common_LED_4_out(IR_Duty*10);
	LED_string[0] = 21;			// ׸������ -������
	LED_string[3] = 21;								// �������� �� ����� ��� ������ ����������. ������� �������� ����.
	DotPrintByte&=0xF0;									// ��� �����.
}

static void Set_Duty_for_IR_OUT(void)						
{
	G_ADC_byte&= ~(1<<Current_sense_ON);				// ��������� ���������� ���, ����� ������ ����������
	if(BtnTp)
	{
		if (IR_Duty>1) IR_Duty -= 1;
	}
	else
	{
		if(IR_Duty<Max_IR_duty)IR_Duty += 1;
	}
	Show_Duty_for_IR_OUT();

}


static uint8_t IR_Out_mode = 1;								// ���� ����������. ��� �������� ��� �������. �������� ������ ���. 
								
void Select_IR_Out_mode(void)								// ������, ����� ������� ����, ��� ��� ��� �� ����� �������, � ������� �������.
{
	
	if(BtnTp)IR_Out_mode--;									// ������ ������ 1 (���� �� ����)
	else IR_Out_mode++;										// �������� �� ������ ����.

	if(!IR_Out_mode) IR_Out_mode = 1;						// ���� ���������� �� ����, ������ ������������ ����� � ����.
	if(IR_Out_mode>4)IR_Out_mode = 1;						// ���� ������ ���������� ����� � ���� (���� ���� :-
	LED_string[0]=10;
	LED_string[1]=1;
	LED_string[2] =21;										// ׸������
	LED_string[3] = IR_Out_mode;
	/*
	1)������ ����������
	2)���������� ���������
	3)���
	4)������ ��� �������� ���������(�������)

	������� ������ ������ - ������� � ����� ������ ����. ����� - ������� � ����������.
	*/
}
// �������� ����� ��� ������ ���������� � EEPROM ��� �����. 

static void Enter_PowerOut_Modus(void)
{
	// ����������� ������
	TCCR2 = 0;												// ������������� �������
	TCCR2 |=(1<<WGM20)|(1<<WGM21);							// fast pwm
	TCCR2 |=(1<<COM21);										// OC2
	TCCR2 |=(1<<CS20);										//��� ������������ ������ ������� 63kHz
	TIMSK &= ~(1<<TOIE2);									// ����������� ����� �� ����, PWM

	DDRD|=(1<<7);											// ����� ��� OCR2
	DDRD|= (1<<5);											// ������ �������� ��� �������� IR3313

															// ����� ���������� � ����� �������� �� ���������
	uart_puts("Power_Out_Modus_activated ");
	uart_new_line_Macro
															// �������������.  ���� �� ����������� ��� �������, � �������� �������, �� ��� ������� �� 1 �������, ��� �� ��������.
	LED_string[0]=10;
	LED_string[1]=1;
	LED_string[2] =21;										// ׸������
	LED_string[3] = IR_Out_mode;									
}


#endif /* CODEPART_2_H_ */
