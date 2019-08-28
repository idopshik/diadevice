/*
 * BipolarStepper.h
 *
 * Created: 08.11.2017 18:45:24
 *  Author: isairon
 Stepper TLE4729G
 */ 
#define F_CPU 16000000UL  // 8 MHz, внутренняя RC-цепочка
#include <avr/io.h>

#ifndef BIPOLARSTEPPER_H_
#define BIPOLARSTEPPER_H_
#define Phase1 PC4
#define Phase2 PC5
#define StepperDDR DDRC
#define SteppePORT PORTC
#define SteppePin PINC
#define StepperPIN PINC 
#define Error1 PC6
#define Error2 PC7

//Макросы
#define Phase1_p    SteppePORT|=(1<<Phase1)
#define Phase2_p	SteppePORT|=(1<<Phase2)
#define Phase1_n	SteppePORT&=~(1<<Phase1)
#define Phase2_n	SteppePORT&=~(1<<Phase2)

// Переменные 
extern uint8_t GlobalAllowStepperMove;
uint8_t reducer =0;
// Прототипы
void Stepper_Move (uint8_t direction);

void InitPins_for_Stepper(void)
{
StepperDDR|=(1<<Phase1)|(1<<Phase2);			//Фазы
StepperDDR&= ~((1<<Error1)|(1<<Error2));
SteppePORT|=(1<<Error1)|(1<<Error2);			//Подтяг на датчиках ошибок
}


inline void ResetupTimer0FotThis(void)
{
	TCCR0 = 0;	
	TCCR0 |= (1<<CS02)|(1<<CS00);	//  1024 - 32,7ms довольно медленно
	TCCR0|= (1<<TOIE0);				// Прерывания по переполнению таймера.
	sei();
}
//Functions

void StepperCheckPosition (signed char external_VAR);

// Variables
volatile static uint8_t current_Step_position;						//Текущий шаг
volatile uint8_t target_Step;						//К чему стремимся
static volatile char StepperBusyFlag;

void Stepper_Move (uint8_t direction)     //по внешеней переменно делаем шаг.
{
	
	if (current_Step_position)current_Step_position=0;		// проверка на выход из диапазона	
	else current_Step_position=1;

	if(direction)
	{
	Phase2_n;
	if(current_Step_position)Phase1_p;
	else Phase1_n;
	}
	else
	{
	Phase1_n;
	if(current_Step_position)Phase2_p;
	else Phase2_n;

	}
	
	
}


#endif /* BIPOLARSTEPPER_H_ */