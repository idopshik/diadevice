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
	// Просто заготовка. 
	// кроме этого вписываешь SelCar возможность выбора второй машины и в прерывании TIMER2_COMP чтобы case условие было.
}
void vaz2108(void)
{
	
	VersatileDevider++;
	if(VersatileDevider==18)PORTC &= ~((1<<PC1)|(1<<PC2));		//Ставим на девятом фронте СKP положительный фронт CMP
	if(VersatileDevider==116)PORTC |= (1<<PC1)|(1<<PC2);		// Спад CMP
	if(VersatileDevider>120)Emul_temp_dummy_var=1;				// всё время выход в нуле - пропуск зубьев (четыре раза)
	if(VersatileDevider==125){VersatileDevider=1; Emul_temp_dummy_var = 0;}		//Первый зуб нового оборота.

	if(Emul_temp_dummy_var)
	{
		PORTC|=(1<<3);
		Emul_temp_dummy_var = 0;
		
	}
	else									//ULN инвертирует. Поэтом начинаем со снятия бита.
	{
		Emul_temp_dummy_var = 1;
		PORTC &= ~(1<<3);		//Убираем бит эмулятора
	}
}




static void SelCar(void)
{
	if(BtnTp)Car_to_emulate--;								// Нажата кнопка 1 (счёт от нуля)
	else Car_to_emulate++;									// действие по кнопке нуль.

	if(!Car_to_emulate) Car_to_emulate = 1;					// Если спустились на нуль, ставим максимальное число в базе.
	if(Car_to_emulate>1)Car_to_emulate = 1;					// Если больше количества машин в базе (пока одна :-
	LED_string[0]=LED_string[1]=LED_string[2] =10;		//Убираем лишний мусор
	LED_string[3] = Car_to_emulate;
}



// Функции вывода и обработки комманд (Вынести вниз и снабдить прототипами).
static void Set_RPM_for_CKP(void)

{
	// 124 щелчка таймера это один оборот для ваза.
	//Таймер без делителя.
	if(BtnTp)
	{OCR2++;
	}
	else // действие по кнопке нуль.
	{
		OCR2--;
	}
	
	// Покажем частоту для ваза

	// Режим таймера CTC и без предделителя
	uint16_t rpm;
	rpm =  (F_CPU/(2*62*4))/OCR2; //	Здесь 2 потому что на зуб два фронта и 62 зуба на маховике. Почему ещё 3 не понял
	
	
	
	Common_LED_4_out(rpm);
	DotPrintByte&=0xF0;				// Без точек.

	 
	 #if defined( DebugCommon )
	 uart_put_int(OCR2);
	 uart_new_line_Macro
	 uart_put_int(rpm);
	 uart_new_line_Macro
	 #endif
}

static void Enter_CKP_emulator_Modus(void)
{
	// Шестьдесят зубьев, холостой ход. 1000 x 60 x 2фронта. -2000 Гц.
	// 10 тыс это уже 20000 Гц, или 0.02 МHz 

	// Логика перевключения
	TIMSK &= ~(1<<OCIE2);								 // Больше не генерируем
	Double_LED_out = 0x00;								 //Показываем, что выключились


	// Переконфигурируем таймер 2
	TCCR2 = 0;											 // Первоначально очищаем
	TCCR2|= (1<<WGM21);									 // Сброс по совпадению (CTC).
	TCCR2 |= (1<<CS21)|(1<<CS22);						 // 256. 244Гц. // переполнение при 256 щелчках. 16MHz

	OCR2 = 30;
	// Железо

	DDRC|=(1<<3); // СКP эмулятор
	DDRC|=(1<<1); // CMP
	DDRC|=(1<<2); // СMP

	
	 #if defined( DebugCommon )
	  uart_puts("CKP/CMP_emul_Modus_activated "); uart_new_line_Macro
	 #endif
	
	
	SelCar(); // Чтобы сразу нарисовать номер тачки.
}




#endif /* CODEPART_3_H_ */
