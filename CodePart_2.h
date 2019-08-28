/*
 * CodePart_2.h
 *
 * Created: 06.01.2018 11:55:03
 *  Author: isairon
 */ 


#ifndef CODEPART_2_H_
#define CODEPART_2_H_

//PowerOutModusFunction


uint8_t I_shutdown_normal_OCR2;   //сохнаранить настроку, чтобы занести её после изменения на повышенную
uint8_t I_shutdown_inrush_OCR2;	 //Подстраивать под удар по лампам
uint8_t Cold_filament_timer;


void Ir_3313_Start (void)
{
	// Неважно, даже если ШИМ. Запускаем этот постановщик.
	// Проверяем, надо ли ставить (если есть настройка - ставить, а так же низкое значение настройки предела.
	// if( OCR2 < 24 ); а так же IR_Filament_setted
	if (!(IR_ctrl_Byte&IR_outActive))		//Если ещё не влючена (антиповтор).
	{

			if(IR_ctrl_Byte&IR_Filament_setted)
			{//ОТрабатываем алгоритм
				// Подгружаем увеличенную динамически изменяемую настроку
				IR_ctrl_Byte|= IR_Filament_Cold_State; // Пока холодная
				
				OCR2 = I_shutdown_inrush_OCR2;												// Добавить динмическое изменение!
				Cold_filament_timer = 150; // Для начала пока втулим сюда это.
		
				IR_ON	// Включение только после включения таймера

				Debug_green_ON
				Debug_yellow_OFF

				RestartDebounce();								// Чтобы избегать ложного срабатывания по отключению IR (Вообще это для plain-режима надо было, тут для мебели).
				// Убери многократный перезапуск из под прямого управления
			}
			else
			{
			 OCR2 = I_shutdown_normal_OCR2 ;
			 IR_ON // Без геморроя
			
			// uart_puts("IR_Start ");	uart_new_line_Macro
			 
			 }

	}

}
uint8_t Cold_filament_timer;
// Подфункция понижает уровень с I_shutdown_inrush_OCR2 до I_shutdown_normal_OCR2
void Inrush_current_handler (void)
{
	if(IR_ctrl_Byte&IR_Filament_Cold_State)
	{
		// Если холодная
		Cold_filament_timer--;				// служебный счётчик.
		
		// Четыре момента.
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
		// Начинаем с 250!
		
		//Крайне желательно все ресурсы кидать на своевременную обработку этого, чтобы не тормозило об индикацию
		
		

	}
	
}



static void IR3313_plain_switch()							//plain
{
	if(BtnTp)
	{
		IR_OFF;												// Включение в таймере, но таймер сам не выключит.
		Double_LED_out = 0x00;								//LED сам выключается только если бит IR_ON_bit_PWM установлен. Смотри строку выше.

		G_ADC_byte &= ~(1<<Current_sense_ON);				// Выключаем измерение тока
		IR_ctrl_Byte&=~IR_OVERCURRENT;//  убираем флаг аварии
		
		IR_ctrl_Byte &= ~IR_ON_bit_plain;		// Надо ли убирать PLAIN ?	//Убираем состояние включенности. 	

	}
	else													// действие по кнопке нуль.
	
	{
		Start_current_measuring();												// Включаем режим измерения
		Blincked_Digits = 0;
		
		Ir_3313_Start();
		IR_ctrl_Byte |= IR_ON_bit_plain;
		RestartDebounce();								// Чтобы избегать ложного срабатывания по отключению IR
	}

}




static void IR3313_PWM_switch()							//pwm
{
	if(BtnTp)
	{
		IR_ctrl_Byte &= ~IR_ON_bit_PWM;			
		IR_OFF;												// Включение в таймере, но таймер сам не выключит.
		Double_LED_out = 0x00;								//LED сам выключается только если бит IR_ON_bit_PWM установлен. Смотри строку выше.
	// Дисплей чистим в запускающих заглушках
		
		
		G_ADC_byte &= ~(1<<Current_sense_ON);				// Выключаем измерение тока
		IR_ctrl_Byte&=~IR_OVERCURRENT;//  убираем флаг аварии. Это почти баг. 

	}
	else													// действие по кнопке нуль.
		
	{
		Start_current_measuring();												// Включаем режим измерения
		Blincked_Digits = 0;
		Ir_3313_Start();								// Это надо, чтобы выполнить логику помощи пуска лампы накала.								
		IR_ctrl_Byte |= IR_ON_bit_PWM;								// Непосредственно включение в обработчике таймера 1.
		RestartDebounce();								// Чтобы избегать ложного срабатывания по отключению IR

	}

}



static void Show_OverCurrent_limit_on_LED_disp(void)
{		

	G_ADC_byte &= ~(1<<0);			   // Очищаем байт - флаг обработчика в main	

											
			uint16_t CurLimShow;
			uint8_t tmp ;

			// Прочёсываем массивчик в поисках подходящего индекса.
			for (tmp=0;tmp<16;tmp++)			// Зависит от количество членов массива
			{
				if(I_shutdown_normal_OCR2 > Current_Limits_PWM_OCR_values[tmp])continue;		// Перебираем следующие по очереди
				else if (I_shutdown_normal_OCR2 == Current_Limits_PWM_OCR_values[tmp])break;   //Попали, то что нужно
				else
				{
					tmp --;
					break;
				}   // Перескочили.
				
			}

		
I_shutdown_inrush_OCR2 = (Inrush_currents_mass[tmp]);				// Сохраняем предел для лампы
		

// Показываем на дисплее предел нормальный

CurLimShow = (Current_Limits_Actual_Currents[tmp]); // Загружаемся по адресу, котоорый узнали.uart_put_int(CurLimShow); 

		Common_LED_4_out(CurLimShow);				// Отображаем значение
		DotPrintByte&=0xF0;							// Без точек.
		LED_string[0] = 10;							//Не нужны нули лишние
		LED_string[1] = 10;
		


//....................// отладка
// В UART нормальный предел
 #if defined( DebugCommon )
uart_put_int(CurLimShow); uart_puts(" A - static resistance"); uart_new_line_Macro			// debug only
 #endif


// Далее в UART показываем предел для лампы. Находим его
for (tmp=0;tmp<16;tmp++)			// Исправить 256 и добавить логики на скорость работы
{
	if(I_shutdown_inrush_OCR2 > Current_Limits_PWM_OCR_values[tmp])continue;		// Перебираем следующие по очереди
	else if (I_shutdown_inrush_OCR2 == Current_Limits_PWM_OCR_values[tmp])break;   //Попали, то что нужно
	else
	{
		tmp --;
		break;
	}   // Перескочили.
	
}

CurLimShow = (Current_Limits_Actual_Currents[tmp]); // Загружаемся по адресу, котоорый узнали.
 #if defined( DebugCommon )
uart_put_int(CurLimShow); uart_puts(" A - inrush for filament lamps"); uart_new_line_Macro			// debug only
 #endif

//....................






}	


static void Set_OverCurrent_limit(void)						// 2 - процедура выбора предельного тока отключения
{
	G_ADC_byte&= ~(1<<Current_sense_ON);					// Запрещаем показывать ток, чтобы видеть лимит тока.
	if(!BtnTp)
	{if(I_shutdown_normal_OCR2<30)I_shutdown_normal_OCR2++;}	
	else
	{if(I_shutdown_normal_OCR2>12)I_shutdown_normal_OCR2--;} 

	eeprom_update_byte(&EE_OverCurrent_limit_OCR2_value, I_shutdown_normal_OCR2);					// EEPROM!

	
 #if defined( DebugCommon )
uart_put_int(I_shutdown_normal_OCR2); uart_new_line_Macro					// debug only
 #endif
	Show_OverCurrent_limit_on_LED_disp();					// показываем и грузим в буферы normal и inrush барьеры токов

}

static void Enter_Set_OverCurrent_limit(void)	
{
	G_ADC_byte&= ~(1<<Current_sense_ON);					// Запрещаем показывать ток, чтобы видеть лимит тока.
  I_shutdown_normal_OCR2 = eeprom_read_byte(&EE_OverCurrent_limit_OCR2_value);

  #if defined( DebugCommon )
 uart_puts("OCR2_readed_from_eeprom - ");uart_put_int(I_shutdown_normal_OCR2);	uart_new_line_Macro
  #endif
 Show_OverCurrent_limit_on_LED_disp();

 AntyReverce_relay_ON										// Раз сюда дошли - не шутим.
 OCR2 = I_shutdown_normal_OCR2 ;						// Задаю OCR2 здесь, чтобы конденсаторы вокруг ОУ успели зарядиться и не было ложного срабатывания при включении

}


														// 3 - процедура выбора скважности сигнала
static void Show_Duty_for_IR_OUT(void)
{

//Полностью запретить показания тока
G_ADC_byte&= ~(1<<Current_sense_ON);				// Запрещаем показывать ток. Но это с задержкой.

	
  #if defined( DebugCommon )
uart_put_int(IR_Duty);  uart_new_line_Macro			// debug only
  #endif
	Common_LED_4_out(IR_Duty*10);
	LED_string[0] = 21;			// Чёрточки -дефисы
	LED_string[3] = 21;								// Наверное не стоит там ничего показывать. Стираем ненужный нуль.
	DotPrintByte&=0xF0;									// Без точек.
}

static void Set_Duty_for_IR_OUT(void)						
{
	G_ADC_byte&= ~(1<<Current_sense_ON);				// Запрещаем показывать ток, чтобы видеть скважность
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


static uint8_t IR_Out_mode = 1;								// Пока глобальная. При загрузке уже единица. Нулевого режима нет. 
								
void Select_IR_Out_mode(void)								// Сделай, чтобы понятно было, что это уже не выбор модусов, а перебор подменю.
{
	
	if(BtnTp)IR_Out_mode--;									// Нажата кнопка 1 (счёт от нуля)
	else IR_Out_mode++;										// действие по кнопке нуль.

	if(!IR_Out_mode) IR_Out_mode = 1;						// Если спустились на нуль, ставим максимальное число в базе.
	if(IR_Out_mode>4)IR_Out_mode = 1;						// Если больше количества машин в базе (пока одна :-
	LED_string[0]=10;
	LED_string[1]=1;
	LED_string[2] =21;										// Чёрточка
	LED_string[3] = IR_Out_mode;
	/*
	1)Прямое управление
	2)Постоянное включение
	3)ШИМ
	4)Работа при коротком замыкании(будущее)

	Нажатие нижней кнопки - переход в выбор лимита тока. Далее - переход к управлению.
	*/
}
// Очевидно нужны ещё четыре переменные в EEPROM для токов. 

static void Enter_PowerOut_Modus(void)
{
	// Настраиваем таймер
	TCCR2 = 0;												// Первоначально очищаем
	TCCR2 |=(1<<WGM20)|(1<<WGM21);							// fast pwm
	TCCR2 |=(1<<COM21);										// OC2
	TCCR2 |=(1<<CS20);										//без предделителя таймер щёлкает 63kHz
	TIMSK &= ~(1<<TOIE2);									// прерываение здесь не надо, PWM

	DDRD|=(1<<7);											// вывод для OCR2
	DDRD|= (1<<5);											// Затвор полевика для открытия IR3313

															// Далее накопление и паузу выставим по умолчанию
	uart_puts("Power_Out_Modus_activated ");
	uart_new_line_Macro
															// Первоначально.  Если не прописывать эти строчки, а вызвавть функцию, то она смещает на 1 единицу, что не нравится.
	LED_string[0]=10;
	LED_string[1]=1;
	LED_string[2] =21;										// Чёрточка
	LED_string[3] = IR_Out_mode;									
}


#endif /* CODEPART_2_H_ */
