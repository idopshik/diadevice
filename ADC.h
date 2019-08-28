/*
 * ADC.h
 *
 * Created: 01.06.2016 14:44:41
 *  Author: isairon
 */ 


#ifndef ADC_H_
#define ADC_H_
	
	//*************Регистр ADCSRA************************//
	// ЭТО ТОЛЬКО ДЛЯ MEGA16 !

	// SC - start conversion
	// IE - interrupt enable
	// ADATE - автоход. Здесь он выключен.
	//           7    6     5     4     3     2     1     0
	//ADCSRA = ADEN ADSC  ADATE  ADIF  ADIE  ADPS2 ADPS1 ADPS0
	//************************************************************//
	

void adc_for_IR3313_current_SetUp(void)
{

	DDRA &= ~((1<<4)|(1<<5));											 // входы АЦП

	ADMUX = 0x00;														// Обнуляем до настройки, убираем мусор
	ADMUX|= (1<<REFS0)|(1<<REFS1);										// internal 2.56
	ADMUX |=(1<<MUX2);													// PA4 - feedback
	ADMUX&= ~(1<<ADLAR);												// равняем по правому, 10 bit
	ADCSRA= 0x00;
	ADCSRA&= ~(1<<ADATE);												//убираем автоход
	ADCSRA|= (1<<ADEN)| (1<<ADSC)|(1<<ADIE)|(1<<ADPS1)|(1<<ADPS2);

	ADCSRA|=(1<ADSC);													//ADSC  Начало преобразования
}

void adc_ext_voltage_SetUp(void)
{
	
	DDRA &= ~((1<<5));												// входы внешнего напряжения
	ADMUX = 0x00;													// Обнуляем до настройки, убираем мусор
	ADMUX|= (1<<REFS0)|(1<<REFS1);									// internal 2.56
	ADMUX |=(1<<MUX0)|(1<<MUX2);									// PA5 - voltage sense
	ADMUX&= ~(1<<ADLAR);											// равняем по правому, 10 bit
	ADCSRA= 0x00;
	ADCSRA&= ~(1<<ADATE);											//убираем автоход
	ADCSRA|= (1<<ADEN)| (1<<ADSC)|(1<<ADIE)|(1<<ADPS1)|(1<<ADPS2);

	ADCSRA|=(1<ADSC);												//ADSC  Начало преобразования
}


#endif /* ADC_H_ */
