/*
 * defs.h
 *
 * Created: 10.11.2017 0:51:02
 *  Author: isairon
 */ 
 // собираю все умолчания здесь
#ifndef DEFS_H_
#define DEFS_H_

//Макросы

#define DynLedState (PORTB&(1<<4))										// Для проверки состояния индикации

#define uart_new_line_Macro {uart_putc(0x0A); uart_putc(0x0D);}
#define Debug_toggle_green  PORTC ^=(1<<PC1);
#define Debug_toggle_yellow PORTC^=(1<<PC3);
#define Debug_green_OFF  PORTC |=(1<<PC1);
#define Debug_green_ON   PORTC &= ~(1<<PC1);
#define Debug_yellow_ON  PORTC|=(1<<PC3);
#define Debug_yellow_OFF PORTC &= ~(1<<PC3);

#define IGBT_HIGH_LEVEL		PORTC&=~(1<<PC1); PORTC&=~(1<<PC2);						/// БАГ БАГ БАГ БАГ
#define IGBT_LOW_LEVEL		PORTC|=(1<<PC1); PORTC|=(1<<PC2);
#define Led_Red		0x42
#define Vibro_short_zzz PORTD|= (1<<PD4); Vibro_back_response = 500;
#define Vibro_Long_zzz PORTD|= (1<<PD4); Vibro_back_response = 2500;

#define IR_ON  if(!(IR_ctrl_Byte&IR_OVERCURRENT)){IR_ctrl_Byte|=IR_outActive; PORTD|=(1<<5);}
#define IR_OFF IR_ctrl_Byte&=~IR_outActive; PORTD&= ~(1<<5);

//------------- Умолчания----------------//
#define Max_IR_duty  10											//Разрешение ШИМ для IR. Важно, потому что IR не быстрее 200 Гц.


#define BAUD 9600
#define Led_Green   0x18
/*
 * defs.h
 *
 * Created: 10.11.2017 0:51:02
 *  Author: isairon
 */ 
 // собираю все умолчания здесь
#ifndef DEFS_H_
#define DEFS_H_

//Макросы

#define DynLedState (PORTB&(1<<4))										// Для проверки состояния индикации

#define uart_new_line_Macro {uart_putc(0x0A); uart_putc(0x0D);}
#define Debug_toggle_green  PORTC ^=(1<<PC1);
#define Debug_toggle_yellow PORTC^=(1<<PC3);
#define Debug_green_OFF  PORTC |=(1<<PC1);
#define Debug_green_ON   PORTC &= ~(1<<PC1);
#define Debug_yellow_ON  PORTC|=(1<<PC3);
#define Debug_yellow_OFF PORTC &= ~(1<<PC3);

#define IGBT_HIGH_LEVEL		PORTC&=~(1<<PC1); PORTC&=~(1<<PC2);						/// БАГ БАГ БАГ БАГ
#define IGBT_LOW_LEVEL		PORTC|=(1<<PC1); PORTC|=(1<<PC2);
#define Led_Red		0x42
#define Vibro_short_zzz PORTD|= (1<<PD4); Vibro_back_response = 500;
#define Vibro_Long_zzz PORTD|= (1<<PD4); Vibro_back_response = 2500;

#define IR_ON  if(!(IR_ctrl_Byte&IR_OVERCURRENT)){IR_ctrl_Byte|=IR_outActive; PORTD|=(1<<5);}
#define IR_OFF IR_ctrl_Byte&=~IR_outActive; PORTD&= ~(1<<5);

//------------- Умолчания----------------//
#define Max_IR_duty  10											//Разрешение ШИМ для IR. Важно, потому что IR не быстрее 200 Гц.


#define BAUD 9600
#define Led_Green   0x18
#define FreshStartedDevice 0xAA		// Просто случайное число. Типа пароля.

#define SA1  PA0
#define SA2  PA1
#define SA3  PA2

// Moduses
#define DefaultMode							0b00000000
#define ShortShoot							0b00000100
#define Speed_emulator						0b00001000
#define CKP_emulator						0b00010000
#define PowerOut							0b00100000
#define Stepper								0b01000000
#define Coils								0b10000000



//***************************************************************************************************************
//										Оперативные настройки


// DynamicLed_ctrl_byte
//-----7----------6---------5---------4-----------3------------2------------1------------
#define Led_run 0b00000001


// IR_ctrl_Byte
//-----7----------6---------5---------4-----------3------------2------------1------------0
//-IR_outActive--------OC---------5---------4-----------3------------2----------1--------0
#define IR_outActive						0b10000000
#define IR_OVERCURRENT						0b01000000
#define IR_ON_bit_straight					0b00100000								//Состояния. Не режимы. Режимы задаются выбранным пунктом Микроменю
#define IR_ON_bit_PWM						0b00010000
#define IR_ON_bit_plain						0b00001000
#define IR_Filament_setted					0b00000010
#define IR_Filament_Cold_State				0b00000001


// Sparkbyte 
//-----7----------6---------5---------4-----------3------------2------------1------------0
//Enable_procc----6---------5---------4-----------3------------2----------Idle--------charge
#define Spark_Enable						0b10000000
#define Spark_Idle							0b00000010
#define Spark_Charge						0b00000001


// Stepper_ControlByte
//-----7----------6---------5------------4-----------3------------2------------1------------0
//----step_en-----6---------5------------4-------------------3-----------POS----------POS----------POS
#define Stepper_Enable						0b10000000
#define Stepper_move_bit					0b01000000


// G_ADC_byte
//-----7----------6---------5---------4-----------3------------2------------1------------0
//----------------6---------5---------4-----------3-------currrentADC----voltADC--------Adc_new
#define New_ADC_result						0
#define Voltage_sense_ON					1
#define Current_sense_ON					2


// Speed_CKP_cntrl_byte
//-----7----------6---------5---------4-----------3------------2------------1------------0
//--------------------------5---------4---------reverse-----------2-------------1------------0
#define CKP_reverse							0b0001000
#define Speed_LOW_freq						0b0100000
#define Speed_HIGH_freq						0b1000000


// IGBT_PWM_cntrl_byte
//---------7------------6-----------------5---------4-----------3------------2------------1------------0
//---IGBT_PWM_en------str_duty -----------5---------4---------3-----------2-------------1------------0
#define IGBT_PWM_en							0b1000000							// старший бит - разрешена работа
#define IGBT_PWM_str_duty					0b0100000


// Modus_6_ctrl_byte
//-----7----------6---------5---------4-----------3------------2------------1------------
//--------------------------5---------4---------3-----------2-------------1------------0
#define Modus_6_ctrl_byte_sound_ON			0b00000010
#define Modus_6_ctrl_byte_SHORT_PRESENT		0b00000100
#define Modus_6_ctrl_byte_SHOOT_active		0b00000001

#endif /* DEFS_H_ */
#define FreshStartedDevice 0xAA		// Просто случайное число. Типа пароля.

#define SA1  PA0
#define SA2  PA1
#define SA3  PA2

// Moduses
#define DefaultMode							0b00000000
#define ShortShoot							0b00000100
#define Speed_emulator						0b00001000
#define CKP_emulator						0b00010000
#define PowerOut							0b00100000
#define Stepper								0b01000000
#define Coils								0b10000000



//***************************************************************************************************************
//										Оперативные настройки


// DynamicLed_ctrl_byte
//-----7----------6---------5---------4-----------3------------2------------1------------
#define Led_run 0b00000001


// IR_ctrl_Byte
//-----7----------6---------5---------4-----------3------------2------------1------------0
//-IR_outActive--------OC---------5---------4-----------3------------2----------1--------0
#define IR_outActive						0b10000000
#define IR_OVERCURRENT						0b01000000
#define IR_ON_bit_straight					0b00100000								//Состояния. Не режимы. Режимы задаются выбранным пунктом Микроменю
#define IR_ON_bit_PWM						0b00010000
#define IR_ON_bit_plain						0b00001000
#define IR_Filament_setted					0b00000010
#define IR_Filament_Cold_State				0b00000001


// Sparkbyte 
//-----7----------6---------5---------4-----------3------------2------------1------------0
//Enable_procc----6---------5---------4-----------3------------2----------Idle--------charge
#define Spark_Enable						0b10000000
#define Spark_Idle							0b00000010
#define Spark_Charge						0b00000001


// Stepper_ControlByte
//-----7----------6---------5------------4-----------3------------2------------1------------0
//----step_en-----6---------5------------4-------------------3-----------POS----------POS----------POS
#define Stepper_Enable						0b10000000
#define Stepper_move_bit					0b01000000


// G_ADC_byte
//-----7----------6---------5---------4-----------3------------2------------1------------0
//----------------6---------5---------4-----------3-------currrentADC----voltADC--------Adc_new
#define New_ADC_result						0
#define Voltage_sense_ON					1
#define Current_sense_ON					2


// Speed_CKP_cntrl_byte
//-----7----------6---------5---------4-----------3------------2------------1------------0
//--------------------------5---------4---------reverse-----------2-------------1------------0
#define CKP_reverse							0b0001000
#define Speed_LOW_freq						0b0100000
#define Speed_HIGH_freq						0b1000000


// IGBT_PWM_cntrl_byte
//---------7------------6-----------------5---------4-----------3------------2------------1------------0
//---IGBT_PWM_en------str_duty -----------5---------4---------3-----------2-------------1------------0
#define IGBT_PWM_en							0b1000000							// старший бит - разрешена работа
#define IGBT_PWM_str_duty					0b0100000


// Modus_6_ctrl_byte
//-----7----------6---------5---------4-----------3------------2------------1------------
//--------------------------5---------4---------3-----------2-------------1------------0
#define Modus_6_ctrl_byte_sound_ON			0b00000010
#define Modus_6_ctrl_byte_SHORT_PRESENT		0b00000100
#define Modus_6_ctrl_byte_SHOOT_active		0b00000001

#endif /* DEFS_H_ */
