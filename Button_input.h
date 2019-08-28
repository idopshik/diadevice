/*
 * Button_input.h
 *
 * Created: 05.06.2016 10:03:22
 *  Author: isairon
 */ 


#ifndef BUTTON_INPUT_H_
#define BUTTON_INPUT_H_


/*
Button_Timer_Flag
------------7------------6------------5------------4------------3------------2------------1------------0------------
		
	
Button_Timer_AUXiliary_Flag
------------7------------6------------5------------4------------3------------2------------1------------0------------
																									Rep_prev_2
	
ButtonState
------------7------------6------------5------------4------------3------------2------------1------------0-----------	
															             Pressed_2   Pressed_1	  Pressed_0		
*/
//////////////////////////////////////________Прототипы


void Within_ISR_button_Long_press_monitor (void);
unsigned char ButtonCheck (void);

//////////////////////////////////////_________Переменные
volatile unsigned char Button_state;
volatile unsigned char Button_Timer_Flag;
volatile unsigned int Button_Timer_Counter;
volatile unsigned char Button_Timer_AUXiliary_Flag;			// К сожалению, для третьей кнопки не хватило одного флаг-бита



#define DEBOUNCE_TIME       0.1     
#define SAMPLE_FREQUENCY    2000
#define MAXIMUM         (DEBOUNCE_TIME * SAMPLE_FREQUENCY)
#define NumberOfButtons      3

char f_integrator (unsigned char input, unsigned char button_num);


 #define DDRButton DDRA
 #define PortButton PORTA
 #define PinButton PINA
 #define Button_0 PA0
 #define Button_1 PA1
 #define Button_2 PA2
 ///#define Button_3 PA2			К сожалению кнопка на другом порту. Пиши туда ручками, какая

// Маски ButtonState
#define ButtonPressed_0_MASK			  0b00000001
#define ButtonPressed_1_MASK	   		  0b00000010
#define ButtonPressed_2_MASK			  0b00000100

#define ButtonPressed_0_LONG_MASK		  0b00001000
#define ButtonPressed_1_LONG_MASK	   	  0b00010000
#define ButtonPressed_2_LONG_MASK	   	  0b00100000
#define ButtonPressed_SHORT_Double_MASK   0b10000000			// В этом проекте не используется

// Маски Button_Timer_Flag
#define ButtonTimerSet_0				  0b00100000
#define ButtonTimerSet_1				  0b01000000
#define ButtonTimerSet_2				  0b10000000


#define ButtonLongReady_0				  0b00000001
#define ButtonLongReady_1	   			  0b00000010
#define ButtonLongReady_2			  	  0b00000100

#define ButtonRepeatPrevention_0		  0b00001000
#define ButtonRepeatPrevention_1		  0b00010000
#define ButtonRepeatPrevention_2_2nd_flag	     0b00000001

// Маски Button_Timer_AUXiliary_Flag
#define ButtonTimerOverFlow 200  



#endif /* BUTTON_INPUT_H_ */