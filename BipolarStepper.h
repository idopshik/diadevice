/*
 * BipolarStepper.h
 *
 * Created: 08.11.2017 18:45:24
 *  Author: isairon
 Stepper TLE4729G
 */ 
#define F_CPU 16000000UL  // 8 MHz, ���������� RC-�������
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

//�������
#define Phase1_p    SteppePORT|=(1<<Phase1)
#define Phase2_p	SteppePORT|=(1<<Phase2)
#define Phase1_n	SteppePORT&=~(1<<Phase1)
#define Phase2_n	SteppePORT&=~(1<<Phase2)

// ���������� 
extern uint8_t GlobalAllowStepperMove;
uint8_t reducer =0;
// ���������
void Stepper_Move (uint8_t direction);

void InitPins_for_Stepper(void)
{
StepperDDR|=(1<<Phase1)|(1<<Phase2);			//����
StepperDDR&= ~((1<<Error1)|(1<<Error2));
SteppePORT|=(1<<Error1)|(1<<Error2);			//������ �� �������� ������
}


inline void ResetupTimer0FotThis(void)
{
	TCCR0 = 0;	
	TCCR0 |= (1<<CS02)|(1<<CS00);	//  1024 - 32,7ms �������� ��������
	TCCR0|= (1<<TOIE0);				// ���������� �� ������������ �������.
	sei();
}
//Functions

void StepperCheckPosition (signed char external_VAR);

// Variables
volatile static uint8_t current_Step_position;						//������� ���
volatile uint8_t target_Step;						//� ���� ���������
static volatile char StepperBusyFlag;

void Stepper_Move (uint8_t direction)     //�� �������� ��������� ������ ���.
{
	
	if (current_Step_position)current_Step_position=0;		// �������� �� ����� �� ���������	
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