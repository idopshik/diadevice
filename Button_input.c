/*
 * Button_input.c
 *
 * Created: 05.06.2016 10:09:07
 *  Author: isairon
 Далее эффективный, эффектный и надёжный код :)
 */
#include "Button_input.h"
#include <avr/io.h>


// Это вставить в обработчик прерывания таймера
void Within_ISR_button_Long_press_monitor (void)
{
    //Пока здесь, но на будущее как то нужно будет может быть отвязаться от лед. Допустим, завязать на другом таймере.
    Button_Timer_Counter++;
    // Если переполнился и БЫЛ установлен хотя бы один из таймеров
    if((Button_Timer_Counter>ButtonTimerOverFlow)&&(Button_Timer_Flag&ButtonTimerSet_0))    Button_Timer_Flag|=ButtonLongReady_0;
    if((Button_Timer_Counter>ButtonTimerOverFlow)&&(Button_Timer_Flag&ButtonTimerSet_1))    Button_Timer_Flag|=ButtonLongReady_1;
    if((Button_Timer_Counter>ButtonTimerOverFlow)&&(Button_Timer_Flag&ButtonTimerSet_2))    Button_Timer_Flag|=ButtonLongReady_2;
}


//******************************************Интегратор***********************************************************

unsigned int integrator[NumberOfButtons];
unsigned char output[NumberOfButtons] = {1,1,1};                                // Не знаю как по другому(Не теряя два слова при определении). Иначе при запуске регистрируется нажатие.

void SetButtonTimer(uint8_t T)
{
    Button_Timer_Counter = 0;                                                   // Начинаем с нуля
    switch(T)
    {
        case 0:
        Button_Timer_Flag |=ButtonTimerSet_0;                                   // Ставим нужный флаг
        break;
        case 1:
        Button_Timer_Flag |=ButtonTimerSet_1;
        break;
        case 2:
        Button_Timer_Flag |=ButtonTimerSet_2;
        break;
        case 11:
        Button_Timer_Flag |=ButtonTimerSet_0|ButtonTimerSet_1;                  //две первых кнопки
        break;
    }
}

char f_integrator (unsigned char input, unsigned char button_num)
{
    if (input == 0)
    {
        if (integrator[button_num] > 0)integrator[button_num]--;
    }
    else if (integrator[button_num] < MAXIMUM) integrator[button_num]++;
    if (integrator[button_num] == 0) output[button_num] = 0;
    else if (integrator[button_num] >= MAXIMUM)
    {
        output[button_num] = 1;
        integrator[button_num] = MAXIMUM;                                   // defensive code if integrator got corrupted
    }
    return output[button_num];
}


// Возвращает коды кнопок

uint8_t ButtonCheck (void)
{
    uint8_t var0,var1,var2,Button_output;
    Button_output = 0;                                                      // Каждый раз с нулём
                                                                            //Антидребезг
    var0 = f_integrator(PinButton &(1<<Button_0),0);                        // Чистые состояния без bounce
    var1 = f_integrator(PinButton &(1<<Button_1),1);
    var2 = f_integrator(PinButton &(1<<Button_2),2);

/////////////////////////////////////____Обслуживание кнопки 0
                                                                            // Если кнопка нажата
     if(!var0)
    {
                                                                            // Предохранитель на повторное действие
        if(!(Button_Timer_Flag&ButtonRepeatPrevention_0))
        {
                                                                            //Первичная установка таймера и флага нажатия.
            if (!(Button_Timer_Flag&ButtonTimerSet_0))
            {
                                                                            //Устанавливаем таймер,Как и маску о его установке
                SetButtonTimer(0);
                Button_state |= ButtonPressed_0_MASK;                       // Заявка на короткое нажатие
            }
                                                                            //Вышел ли таймер Длинного нажатия
            if(Button_Timer_Flag&ButtonLongReady_0)
            {
                                                                            // А нету ли у нас запрета на это действие, потому что сделано другое?
                if (!(Button_state&ButtonPressed_SHORT_Double_MASK))
                {
                                                                            //ДЕЙСТВИЕ ПО ДЛИТЕЛЬНОМУ НАЖАТИЮ
                    Button_output|=ButtonPressed_0_LONG_MASK;
                }
                                                                            //вышел, ставим предохранитель
                Button_Timer_Flag |= ButtonRepeatPrevention_0;
                                                                            // Отмена заявки на короткое нажатие
                Button_state &= ~ButtonPressed_0_MASK;
            }
        }
    }

    // Если кнопка отпущена
    if(var0)
    {
                                                                            //Проверяем состояние, было ли короткое нажатие
        if (Button_state&ButtonPressed_0_MASK)
        {
                                                                            //Может быть в это время нажата и не прошла LONG вторая кнопка, а так же пока нет регистрации SHORT_Double
             if ((Button_state&ButtonPressed_1_MASK)&&(!(Button_state&ButtonPressed_SHORT_Double_MASK)))
            {
                Button_state|= ButtonPressed_SHORT_Double_MASK;             //ставим флаг двойного нажатия (для второй кнопки и уходим
                                                                            // БЫЛО КОРОТКОЕ ДВОЙНОЕ НАЖАТИЕ
                Button_output|=ButtonPressed_SHORT_Double_MASK;
                                                                            // Вторая кнопка должна по выходу убрать этот флаг.
                                                                            // Отсечь возможное длинное второй кнопки
            }
                                                                           //Нет не нажата, и у нас нет флага игнорирования
            else if(!(Button_state&ButtonPressed_SHORT_Double_MASK))
            {
                                                                            //Было, КОРОТКОЕ НАЖАТИЕ ДЕЙСТВИЕ,
                Button_output|=ButtonPressed_0_MASK;
            }
                                                                            // видимо всё таки есть
            else
            {
                Button_state=0;                                             // Убирает это событие для исключения повтора, уходим
            }
                                                                            // Убирает это событие для исключения повтора так или иначе
                Button_state&= ~ButtonPressed_0_MASK;
        }
                                                                            // Гарантированно очищаем таймерные флаги этой кнопки
        Button_Timer_Flag &= ~ (ButtonLongReady_0 | ButtonTimerSet_0 | ButtonRepeatPrevention_0); // Их нужно всегда очищать! Да, потеря времени на каждой итерации. Но если каждый раз проверять перед очисткой флаг, едва ли будет меньше кода
    }



/////////////////////////////////////____Обслуживание кнопки 1

                                                                            // Если кнопка нажата
    if(!var1)
    {
                                                                            // Предохранитель на повторное действие
        if(!(Button_Timer_Flag&ButtonRepeatPrevention_1))
        {
                                                                            //Первичная установка таймера и флага нажатия.
            if (!(Button_Timer_Flag&ButtonTimerSet_1))
            {
                                                                            //Устанавливаем таймер,Как и маску о его установке
                SetButtonTimer(1);
                Button_state |= ButtonPressed_1_MASK;                       // Заявка на короткое нажатие
            }
                                                                            //Вышел ли таймер Длинного нажатия
            if(Button_Timer_Flag&ButtonLongReady_1)
            {
                                                                            // А нету ли у нас запрета на это действие, потому что сделано другое?
                if (!(Button_state&ButtonPressed_SHORT_Double_MASK))
                {
                                                                            //ДЕЙСТВИЕ ПО ДЛИТЕЛЬНОМУ НАЖАТИЮ
                    Button_output|=ButtonPressed_1_LONG_MASK;
                }
                                                                            //вышел, ставим предохранитель
                Button_Timer_Flag |= ButtonRepeatPrevention_1;
                                                                            // Отмена заявки на короткое нажатие
                Button_state &= ~ButtonPressed_1_MASK;
            }
        }
    }

                                                                            // Если кнопка отпущена
    if(var1)
    {
                                                                            //Проверяем состояние, было ли короткое нажатие
        if (Button_state&ButtonPressed_1_MASK)
        {
                                                                            //Может быть в это время нажата и не прошла LONG вторая кнопка, а так же пока нет регистрации SHORT_Double
            if ((Button_state&ButtonPressed_0_MASK)&&(!(Button_state&ButtonPressed_SHORT_Double_MASK)))
            {
                Button_state|= ButtonPressed_SHORT_Double_MASK;             //ставим флаг двойного нажатия (для второй кнопки и уходим
                                                                            // БЫЛО КОРОТКОЕ ДВОЙНОЕ НАЖАТИЕ
                Button_output|=ButtonPressed_SHORT_Double_MASK;
                                                                            // Вторая кнопка должна по выходу убрать этот флаг.
                                                                            // Отсечь возможное длинное второй кнопки
            }
                                                                            //Нет не нажата, и у нас нет флага игнорирования
            else if(!(Button_state&ButtonPressed_SHORT_Double_MASK))
            {
                                                                            //Было, КОРОТКОЕ НАЖАТИЕ ДЕЙСТВИЕ,
                Button_output|=ButtonPressed_1_MASK;
            }
                                                                            // видимо всё таки есть
            else
            {
                Button_state=0;                                             // Убирает это событие для исключения повтора, уходим
            }
                                                                            // Убирает это событие для исключения повтора так или иначе
            Button_state&= ~ButtonPressed_1_MASK;
        }
                                                                            // Гарантированно очищаем таймерные флаги этой кнопки
        Button_Timer_Flag &= ~ (ButtonLongReady_1 | ButtonTimerSet_1 | ButtonRepeatPrevention_1); // Их нужно всегда очищать! Да, потеря времени на каждой итерации. Но если каждый раз проверять перед очисткой флаг, едва ли будет меньше кода
    }



/////////////////////////////////////____Обслуживание кнопки 2
                                                                            // Если кнопка нажата
    if(!var2)
    {
                                                                            // Предохранитель на повторное действие
        if(!(Button_Timer_AUXiliary_Flag&ButtonRepeatPrevention_2_2nd_flag))
        {
                                                                            // Может быть он (таймер)даже не установлен?
            if (!(Button_Timer_Flag&ButtonTimerSet_2))
            {
                                                                            //Устанавливаем таймер
                SetButtonTimer(2);
                Button_state |= ButtonPressed_2_MASK;                       // Заявка на короткое нажатие
            }

                                                                            //Вышел ли таймер Длинного нажатия
            if(Button_Timer_Flag&ButtonLongReady_2)
            {
                                                                            //вышел, ставим предохранитель
                Button_Timer_AUXiliary_Flag |= ButtonRepeatPrevention_2_2nd_flag;
                                                                            // Отмена заявки на короткое нажатие
                Button_state &= ~ButtonPressed_2_MASK;

                                                                            /// ВАЖНО. Здесь нет логики которкого двойного нажатия. Кнопка опирается только на саму себя
                                                                            //ДЕЙСТВИЕ ПО ДЛИТЕЛЬНОМУ НАЖАТИЮ
                Button_output|=ButtonPressed_2_LONG_MASK;
            }
        }
    }

                                                                            // Если кнопка отпущена
    if(var2)
    {
                                                                            //Проверяем состояние, было ли короткое нажатие
        if (Button_state&ButtonPressed_2_MASK)
        {
                                                                            /// ВАЖНО. Здесь нет логики которкого двойного нажатия. Кнопка опирается только на саму себя
                                                                            //Было, КОРОТКОЕ НАЖАТИЕ ДЕЙСТВИЕ,
            Button_output|=ButtonPressed_2_MASK;
                                                                            // Убирает это событие для исключения повтора
            Button_state&= ~ButtonPressed_2_MASK;
        }
                                                                            // Гарантированно очищаем таймерные флаги этой кнопки (Потому что нажатие могло быть и длинным. Придётся на каждой итерации
        Button_Timer_Flag &= ~  (ButtonLongReady_2 | ButtonTimerSet_2 );
        Button_Timer_AUXiliary_Flag &= ~ButtonRepeatPrevention_2_2nd_flag;
    }

    return Button_output;
}
