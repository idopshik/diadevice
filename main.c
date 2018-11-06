/*
 * Dia_device.c
 *
 * Created: 14.10.2017 0:01:43
 * Author : isairon
 */ 
/*
...................BUGs.................................
*/

//option компиляции (Закоментить, если не надо)
//#define Debug595LED                           // Не работает двуцветный светодиод (третья 595). x16 slower
//#define Debug_moduses_menu                    //Выводятся бинарные значения переменной "Modus_is_under_selection". И может ещё допишу чего.
//#define DebugCommon
#define Sound_ON

#define F_CPU 16000000UL                        // 16 MHz, внутренняя RC-цепочка

#include <avr/io.h>

#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "avr/wdt.h"

#include <stddef.h>                             // Описания типов и имён
#include <stdint.h>


#include "LED_shift.h"
#include "HW_definitions.h"
#include "Button_input.h"
#include "ADC.h"
#include "BipolarStepper.h"
#include "defs.h"
#include "uart.h"
#include "uart_addon.h"                          // Исключительно DEBUG
#include "MicroMenu.h"                           //MICRO-MENU V2 Dean Camera, 2012. Респект чуваку.
#include "sound.h"                               // звуковой модуль



//--------------------------------Энергонезависивая память.-----------------------------------
unsigned char EE_OverCurrent_limit_OCR2_value EEMEM;
unsigned char EE_IR_Duty EEMEM; 
unsigned char EE_StepperRotationSpeed EEMEM;  
unsigned char EE_SparkCharge EEMEM;
unsigned int  EE_SparkPause EEMEM ;
unsigned char EE_index_OCR2_mass EEMEM;

unsigned int EE_current_presc EEMEM;

//---------------Переменные глобальные настройки и состояния-------------------//

// 1 - таблица значений таймера относительно к предельным токам отключения
uint8_t const Current_Limits_PWM_OCR_values[] = {12,14,15,16,17,18,21,23,24,25,26,27,28,45,50,55};  //Последний член для логики выхода из диапазона строки.
uint8_t const Inrush_currents_mass[] =          {0 ,0 ,20,28,40,45,47,50,23,24,26,28,30,32,34,38};

uint8_t const Current_Limits_Actual_Currents[] ={0 ,0 ,0 ,1 ,5,10,15,20,25,30,35,40,45,55,60,70};  //Членов столько же. Токи /20 чтобы влезть в размерность 256

uint8_t GlobalAllowStepperMove = 0;

unsigned char OCR_values[] = {255,162,122,81,61,49,41,35,28,24,20,16,12,10,9,8,7,6,5,4,32,29,26,23,21,19,17,15,13,12,11,10,9,8,7,6,5,4};
unsigned char index_mass_speed;

volatile unsigned char G_ADC_byte ; 
volatile unsigned char Modus;               

volatile unsigned char SparkByte;

volatile unsigned char Stepper_ControlByte;
volatile unsigned char IR_ctrl_Byte;
volatile unsigned char StepperRotationSpeed;

volatile unsigned char DynamicLed_ctrl_byte ;

volatile unsigned char Car_to_emulate;
volatile unsigned char RPM_to_emulate;

volatile unsigned char Emul_temp_dummy_var;

volatile unsigned char IGBT_PWM_cntrl_byte;

volatile unsigned char Speed_CKP_cntrl_byte;

volatile uint8_t SparkCharge;                                   // 16*0.128 = 2ms
volatile uint16_t SparkPause;                                   //= 520 488*0.256 = 125ms //8Hz * 60s= 480 /// 480 *2 оборота = 1000 об/мин Здесь ошибка, другой кварц

unsigned int current_presc;         // Ещё одна глобальная переменная.

//--------------- Глобальные счётчики и другие изменяемые переменные.я-------------------//
volatile unsigned char BtnTp;                                    // Вид нажатой кнопки. (плюс или минус)
volatile unsigned char Button_way_of_interpret;                  // биты 0 1 2 как кнопки. Единицы в них - распозавание по отпусканию!
volatile static unsigned char Double_LED_out;
volatile unsigned char RGB_small_counter;
volatile unsigned char RGB_value = 3;
volatile unsigned int RGB_Large_prog_counter = 0;
volatile unsigned char RGB_COLOR0 =0;
volatile unsigned char RGB_COLOR1 =0;
volatile unsigned char Nested_shift_color_cntr =0;

volatile unsigned char Flirty_draw_cntr =0;
volatile unsigned char Digit = 0;                               //Разряд для мультиплексированного вывода.
volatile unsigned char LED_string[4];                           //Текущая строка вывода.
volatile unsigned int ADC_readed;                               // Считываемое оперативное значение
volatile unsigned char DotPrintByte;                            //Озаглавить иначе и добавить другие флаги
volatile unsigned char ADC_punch_delay;                         // Счётчик задержки пинка АЦП
volatile unsigned int Timer2_nested_Counter = 0;
volatile unsigned char Modus_is_under_selection = 0;            // Возможно совместить с переменной Модус
volatile unsigned char VersatileDevider = 0;
volatile unsigned char Ir_duty_counter = 0;                     //inside timer 1
volatile unsigned char Symb_to_draw =0;                         //temp. только для flirty effect
volatile unsigned char IR_state_of_output;
volatile int DebugInt = 0;
volatile unsigned char Blincked_Digits = 0;                         //Blincked_Digits

volatile unsigned int Vibro_back_response;
volatile static uint16_t blink_counter;
volatile unsigned int Steps_of_stepper = 200;                   // Начинаем с этого числа

uint16_t ShortShootTimer;
unsigned char IR_Duty = Max_IR_duty;                            // По умолчанию при включении всегда 100%. И индикацию прикрутить
uint8_t Timer2_nested_Counter_antibug;

//---------------Прототипы внешние-------------------//
void Acceleration_to_LED (signed char acceleration_output);
void ReadAxis(void); 
void Get_time (void);
void ONE_WIRE_DO_IT_HERE(void);
void Threshold_reducer (void);
void Change_Mode_of_Operation(uint8_t NewMode);
void Inrush_current_handler (void);

//---------------Прототипы на "местные" функции------------------//
void Frequency_to_show(void);
static void Menu_Generic_Write(const char* pointer_to_string);  //
void ChangeModus(void);
void Set_Timer_1(void);
static void IR3313_plain_switch(void);

static void Menu_Modus_SELECT(void);
static void Menu_move_UP(void);
void RGB_LED_idle_fading(void);
void FlirtyEffect_1(void);
void vaz2108(void);
void car2(void);
void StraghtContr(void);
void Set_IR_out_mode (void);
void Menu_set_Stepper_speed(void);
void set_Stepper_speed(void);
static void Show_Duty_for_IR_OUT(void);
void Start_current_measuring(void);
void Speed_emul_Set(void);

void EEPROM_mass_read(void)
{
 SparkCharge = eeprom_read_byte(&EE_SparkCharge);
 StepperRotationSpeed = eeprom_read_byte(&EE_StepperRotationSpeed);
 SparkPause  = eeprom_read_word(&EE_SparkPause);
}


void EEPROM_rewrite_from_ROM(void)                              //Восстановление битой ЕЕПРОМ
{
     eeprom_write_byte(&EE_OverCurrent_limit_OCR2_value,20);    //Таймер - 256 - пол таймера. Вроде 15 ампер но оче нь не точно.
    
     eeprom_write_byte(&EE_IR_Duty,5);                          //всего десять уровней заполнения
    
     eeprom_write_byte(&EE_StepperRotationSpeed,0x78);          //120 dec
     
     eeprom_write_byte(&EE_SparkCharge,0x0F);                   //Вписал накопление 2мс.                    
      
     eeprom_write_word(&EE_SparkPause,500);                     //Пересчитывает и показываето около 1000 (типа об/мин).

     eeprom_write_byte(&EE_index_OCR2_mass,12);                 // Чтобы не нуль.

     eeprom_write_word(&EE_current_presc, 100); 
}

//***************************************** INTERRUPT SERVICE ROUTINES **********************
ISR(TIMER1_OVF_vect)                                                        //1ms   // Обслуживаем индикацию и клавиатуру
{

    Within_ISR_button_Long_press_monitor();
    Inrush_current_handler();   
                                                                        
    ADC_punch_delay++;                                                      //Логика пинка АЦП
    if (ADC_punch_delay>125)                                                // Включен режим и отщёлкало четверть секунды)
    {
        if((G_ADC_byte&(1<<Current_sense_ON))||(G_ADC_byte&(1<<Voltage_sense_ON)))ADCSRA |=(1<<ADSC);    //   Начинаем преобразование АЦП
        ADC_punch_delay = 0;                                                 // ну и обнуляем
    }
                                                            
                                                                
    if(IR_ctrl_Byte & IR_ON_bit_PWM)                                        //IR  - T2 занят под создание напряжения на операционник                        
    {                                                                       //  IR3313 не быстрее 200 ГЦ. Поэтому  50 Гц. (надо увеличить до сотни навверное    
            Ir_duty_counter++;
            if(Ir_duty_counter>Max_IR_duty){Ir_duty_counter = 0; IR_ON;}        
            if(Ir_duty_counter>IR_Duty){IR_OFF;}                    
    }

    if (DynamicLed_ctrl_byte&Led_run)                                                           // Если нет запрета 
    {
         if (!(Modus&0xFC))RGB_LED_idle_fading();                                   // Если никакой модус не включен (в сравнении отсекаем биты настройки)
          
         Digit++;                                                               //изменение разряда для мультиплексированного вывода.
         if(Digit==4) Digit=0;
         uint8_t tmp;                                                           //Логика точки
         tmp = 0x01;
         tmp<<=Digit;

         tmp &= DotPrintByte;                                                   // проверяю бит в командром байте.
         
         if (Blincked_Digits)
         {
             blink_counter++;
             if(blink_counter>500) blink_counter = 0;                           // Оттикали, обнуляем
             if(blink_counter>350) PutOneDigit(10,Digit,tmp,Double_LED_out);        // Выводим пустоту.
             else PutOneDigit(LED_string[Digit],Digit,tmp,Double_LED_out);                                                  //нормальный вывод
         }
         else                                                                       //Не моргаем
         {
             if ((Modus&Speed_emulator)&&(Digit == 0)&&(LED_string[0] == 0))FlirtyEffect_1();       // Если не мешает
             else
             PutOneDigit(LED_string[Digit],Digit,tmp,Double_LED_out);
         }
    }

}


ISR(TIMER2_OVF_vect)                                                // Различная логика.
{
    if (IGBT_PWM_cntrl_byte&IGBT_PWM_en)
    {
        IGBT_LOW_LEVEL;
    }
    else                                                            // Всё остальное 
    {                                                               // Spark (0,256 ms переполнение)  0.128 переполнение - другой кварц!
        Timer2_nested_Counter++;
        Timer2_nested_Counter_antibug++;
        
        if(((Timer2_nested_Counter>SparkCharge)&&(SparkByte&0x01))||(!(SparkByte&0x80))) // стояла заявка на накопление и счётчик вышел
        {
            PORTC |= (1<<PC1);                                      //N-chanel Mosfet - 1 на базе и он открыт. Но у нас ИНВЕРТИРУЮЩИЙ триггер
            PORTC |= (1<<PC2);                                      //Подаём 1 на триггер, он подаёт 0 на мосфет, тот закрывается и ИСКРА!!!
            
            Timer2_nested_Counter = 0;                              // Обнуляем
            SparkByte &= 0xFE;                                      // Убираем цикл заряда1
            SparkByte |= 0x2;                                       //Ставим Ожидание1
        }

        if((Timer2_nested_Counter>SparkPause)&&(SparkByte&0x82))     // стояла заявка на паузу и счётчик вышел И есть разрешение
        {
            PORTC &= ~(1<<PC1);                                     //ИНВ триггер. 0 на него, он даёт 1, mosfet отрывается - накапливаем
            PORTC &= ~(1<<PC2);                                     //
            SparkByte &= 0xFD;                                      // Убираем цикл заряда1
            SparkByte |= 0x1;                                       //Ставим накопление1// если есть старший бит - бит работы искры.
            Timer2_nested_Counter = 0;                              // Обнуляем
        }
        
        if(Stepper_ControlByte & Stepper_Enable)                    // Тупо ставим флажок нон-стоп.
        {
            
            if (Timer2_nested_Counter_antibug >StepperRotationSpeed)
            {
                Timer2_nested_Counter_antibug = 0;                  //  Обнуляем
                Stepper_ControlByte|= Stepper_move_bit;         // Заявочку на основной цикл.
            }
        }
    }
}

ISR(TIMER2_COMP_vect)
{

                                                                            // Восьмиклапаный ваз. Двойной пропуск и после него девятый положительный фронт совпадает с положительным фроном 
                                                                            //распредвала. Третий полжительный фронт коленвала от конца совпадает с отрицательным фронтом распредвала.
                                                                            //Формула задающего диска 60+2.

    if(Modus&CKP_emulator)                                                  // Если активирован этот модус
    {   
        switch(Car_to_emulate)
        {
            case 1: vaz2108();          
            break;
            case 2: car2();
            break;
        }
    
    }       

    else if(Modus&Speed_emulator)                                           // Если активирован этот модус
    {
    //   61Гц- минимальная частота, если не использовать делители, а частота ШИМ вдвое меньше - 30.5 Гц. 

        VersatileDevider++;

        if((VersatileDevider>8)||(Speed_CKP_cntrl_byte&&Speed_HIGH_freq))  // Вроде должно круто работать.
        {
            VersatileDevider = 0;
            if(Double_LED_out)Double_LED_out = 0x00;
            else Double_LED_out = Led_Green;
            

            if(Emul_temp_dummy_var)
                {
                    Emul_temp_dummy_var = 0;
                    PORTD &= ~((1<<PD2)|(1<<PD3));                                  //Убираем бит эмулятора
        
                }
                else
                {
                    PORTD|= (1<<PD2)|(1<<PD3);
                    Emul_temp_dummy_var = 1;
                }
        }
    }   
                                                                                            // Логика работы ШИМ через IGBT транзисторы, что на плате.
    else if (IGBT_PWM_cntrl_byte&IGBT_PWM_en)IGBT_HIGH_LEVEL;               // Больше 30КГц нам и даром не надо, так -то,
                                                                            // поэтому аппаратные режими ШИМ не используем. 
}

ISR(ADC_vect)                                                               // Обработчик прерывания АЦП_завершено_преобразование
{   
                                                                            //Cчитать результат.
    uint16_t tmp;
    tmp = ADC ;                                                             // считываем 10-ти битное значение, хотя это не прваильно
                                                                            //Сравнить результат, если он новый, поствить галку.
    if(tmp!=ADC_readed)
    {ADC_readed=tmp;
    G_ADC_byte |= (1<<0);                                                   //ставим бит

    }
}

//*******************************************************************************************
//--------------- Процедуры и функции-----------------------//



void DynamicLedHalt (void)
{
    DynamicLed_ctrl_byte &= ~Led_run;
    
    uint8_t tmp;

    for (tmp=0;tmp<14;tmp++)            // Исправить 256 и добавить логики на скорость работы
    {
        PutOneDigit(10,tmp,1,Double_LED_out);                           //нормальный вывод
    }
    
}
void DynamicLedRelease (void)
{
    DynamicLed_ctrl_byte|= Led_run;                                             // Enable dynamic LED
}

void StopTimer_1(void)                                                      //Если нужна точность и подозрения на баги
{
    PutOneSymbol(6,4,0x00,0x00);                                            // 6-черточка 4-switch уйдёт на default и подсветит все разряды 0x00 без точки, 0x00 - светодиод, но здесь безразлично
    TIMSK&= ~(1<<TOIE1);                                                    //  Собственно, вырубаем прерывание
                                                                            //Button_way_of_interpret &= 0xF8; // Убираем биты 0,1 и 2 - метка на возможно будущую логику
}

void RestartTimer_1(void)                                                   // Установка переменных для логики кнопок и на будущую логику.
{                                                                       //  Button_way_of_interpret |= 0x07; // биты-флаги чтения кнопок. есть мнение, что это не надо уже.
    Set_Timer_1();
}

void FlirtyEffect_1(void)
{
    Flirty_draw_cntr++;
    if (Flirty_draw_cntr>5)
    {
        Flirty_draw_cntr=0;
        Symb_to_draw++;
        if (Symb_to_draw>5)Symb_to_draw=0;
    }

    PutOneSymbol(Symb_to_draw,0,0x8,Double_LED_out);
}


void RGB_LED_idle_fading(void)
{
RGB_Large_prog_counter++;

switch(RGB_Large_prog_counter)
{
    case 1:                                                         // Начинаем с паузы 0.3 c.
    RGB_COLOR0 = RGB_COLOR1 = 0x00;
    RGB_value=0;                                                    //Показатель цветосмешения.
    break;
    case 150:                                                       // Плавно включаем один цвет.
    RGB_COLOR0 = 0x00 ;                                             //Стартует с нуля
    RGB_COLOR1 = Led_Green;                                         //Заполняем зелёным.
    RGB_value=0;                                                    //Показатель цветосмешения.
    break;
    case  550:                                                      // Переходим из цвета в цвет.
    RGB_COLOR0 = Led_Green;
    RGB_COLOR1 = Led_Red;
    RGB_value=0;                                                    //Показатель цветосмешения.
    break;
    case  950   :                                                   // Плавно гасим красный
    RGB_COLOR0 = Led_Red;
    RGB_COLOR1 = 0x00;
    RGB_value=0;                                                    //Показатель цветосмешения.
    break;
    case 1500:                                                      // Начинаем с паузы 0.3 c.
    RGB_COLOR0 = RGB_COLOR1 = 0x00;
    RGB_value=0;                                                    //Показатель цветосмешения.
    break;
    case  1650:                                                     // Плавно включаем второй.
    RGB_COLOR0 = 0x00;
    RGB_COLOR1 = Led_Red;                                           // Врубаем плавно красный
    RGB_value=0;                                                    //Показатель цветосмешения.
    break;
    case  2050  :                                                   // Переходим из цвета в цвет.
    RGB_COLOR0 = Led_Red;
    RGB_COLOR1 = Led_Green;
    RGB_value=0;                                                    //Показатель цветосмешения.
    break;
    case  2450  :                                                   // Плавно гасим зелёный.
    RGB_COLOR0 = Led_Green;
    RGB_COLOR1 = 0x00;
    RGB_value=0;                                                    //Показатель цветосмешения.
    
    break;
    case  3000:                                                     // Весь алгоритм заново.
    RGB_COLOR0 = RGB_COLOR1 = 0x00;
    RGB_Large_prog_counter = 0;                                     // Весь алгоритм заново.
    break;

}

RGB_small_counter++;
if (RGB_small_counter>6){Double_LED_out = RGB_COLOR1;RGB_small_counter=0; }
if (RGB_small_counter>=RGB_value)Double_LED_out = RGB_COLOR0;

Nested_shift_color_cntr++;
if (Nested_shift_color_cntr>50){RGB_value++;Nested_shift_color_cntr = 0;}
}

void Set_Timer_1(void)
{
                                                                                        //Для динамической индикации
    TCCR1A = 0;
    TCCR1B = 0;  
                                                                                        //Надо понизить разрядность. Иначе даже без предделителя частота будет слишком низкой - 100ГЦ. 
    TCCR1A |= (1<<WGM12)|(1<<WGM10);                                                     // fast pwm 8 bit


        #if defined( Debug595LED )
        TCCR1B&= ~(1<<CS11); TCCR1B |=(1<<CS12);                                        // 1024!
        #else  
        TCCR1B |=(1<<CS10)|(1<<CS11);                                                   // 64       Нормальная работа.
        #endif
            
    TIMSK|=(1<<TOIE1);                                                                  // Прерываение

//  LED_string[0] = {8,8,8,8};                                                          //Почему-то так не хочет работать
    DotPrintByte = 0xFF;
}

uint16_t DIG_digit(uint16_t dig, uint16_t sub,uint8_t DIGIT) {
    char c = 0;
    while (dig >= sub) {
        dig -= sub;
        c++;
    }
    LED_string[DIGIT]=c;                // Вожделенная строчка!
    return dig;
}
void Common_LED_4_out(int16_t num) {        // Подготавливаем строку для текущего вывода
    
    num = DIG_digit(num, 1000,0); 
    num = DIG_digit(num, 100,1); 
    num = DIG_digit(num, 10,2); 
    num = DIG_digit(num, 1,3); 
}

void DIG_ChargTime(int16_t unum) {


    unum = DIG_digit(unum, 100,0); 
    unum = DIG_digit(unum, 10,1); 
    unum = DIG_digit(unum, 1,2); 
    DotPrintByte|=0x01; 
}

void DIG_num_for_ADC(uint16_t snum) {
    

    if (G_ADC_byte&(1<<Voltage_sense_ON))
    {
        uint16_t tmp = 557;

        tmp -=10;                                                       // Смещение для этого железа. Не могу настроить, не хватает регулировки
        tmp-=snum;                                                      // узнали разницу с 15 вольтами
        tmp*=27;                                                        // узнали  её в вольтах.
        tmp/=10;                                                        //!!!!!!!! разделили с потерей знака.
        snum = 1500-tmp;

                                                                        // Убрать погрешность при делении. Сделать возможным измерения более 15 вольт. Протестить

        DotPrintByte|=0x02;                                             // Командный байт точки (глобальный). Его нужно и стрирать тоже
        Common_LED_4_out(snum);     
    }

}

void Show_current(uint16_t snum) {
    
        uint8_t value;
        if (OCR2<22)value = 12;
        else if (OCR2<24)value = 26;
        else
        {
            value = 16;
        }
        if(snum<value)snum = 0; // Нет нагрузки, отсекаем помехи
        else
        {
            snum -= 10; // снимаем смещение
            if(OCR2<20)snum = (13*snum);        // При низком значении регистра сравнения
            else
            {snum = (16*snum);      // При ocr = 20 это по опыту похоже на правду.
            }
            
            DotPrintByte|=0x02;
        }
        
        Common_LED_4_out(snum);
        

        /*  Черновик-заготовка на будущее
        if (snum>123)snum -=124;   // при 30-ти считывалось 123 
        else snum = 0;              // Значит помехи. Отсекаем.
        
        if (snum<4)snum = 0;        //Чтобы помехи не показывал

        //1.86 ампера - выводило 134 в регистр
        snum*=19;           // вроде получаем число в амперах. 
        Common_LED_4_out(snum);
        if(snum<1000)LED_string[0]=10; //Лишний знак меня бесит
        DotPrintByte = 0x02; // Точка посередине. Меряем с сотыми ампера.
        */
}


#define IR_alert_debouncerValue_MAX 20          // Ищу баг, пытаюсь увеличить в пять раз.

static uint8_t  Ir_cntr_integr;

char IR_alert_debouncer (unsigned char input)                                   //Заводим логику
{
    char out;
    
    out = 0;

    if (input == 0)
    {
        if (Ir_cntr_integr > 0)Ir_cntr_integr--;                            //проверка на нуль  и баунс возле нуля.
    }
    else if (Ir_cntr_integr < IR_alert_debouncerValue_MAX) Ir_cntr_integr++;        // проверка на максимум
    
    
    if (Ir_cntr_integr == 0) out = 0;
    
    else if (Ir_cntr_integr >= IR_alert_debouncerValue_MAX)
    {
        out = 1;
        Ir_cntr_integr = IR_alert_debouncerValue_MAX;                           // defensive code if integrator got corrupted
    }
    return out;
}

void RestartDebounce(void)
{
    Ir_cntr_integr = 0;  // Обнуляем счётчик атнидребезга датчика состояния вывода IR
}


//88888888888888888888888888888888888888888888888888888888888888888888888888888888888888
// Уменьшаем размер файла выносом кода в другие файлы.
#include "CodePart_2.h"                                                 // PowerOutModusFunction
#include "CodePart_3.h"                                                 // CKP_EMUL_ModusFunction


void StartWaves(void)
{
    
    TIMSK |= (1<<OCIE2);                                                // прерываение по совпадению ! Другой обработчик прерывания.
    Set_RPM_for_CKP();

    Double_LED_out = Led_Green;                                         //Общее для всех. Зелёный - нормальная работа.
}

static void Enter_Coil_Modus(void)
{
                                                                        // Таймер
    TCCR2 = 0;                                                          // Первоначально очищаем
    TCCR2 |= (1<<CS21);                                                 // 8. 3.9k 0.256с
    TIMSK |= (1<<TOIE2);                                                // прерываение
                                                                        // Железо
    DDRC |= (1<<PC1)|(1<<PC2); 

     #if defined( DebugCommon )
     uart_puts("Coil_Modus_Active "); uart_new_line_Macro
     #endif                                                                 // Далее накопление и паузу выставим по умолчанию
                                                                //Сразу переходим на меню ниже, выбираем длительность накопления
    Menu_Navigate(MENU_NEXT);
}

static void Stepper_Start_Logic(void)
{
    StraghtContr();
    Blincked_Digits = 0;                                                // Мы использовали моргания в настройке 
    Stepper_ControlByte |= Stepper_Enable;                          //Убрать это и использовать модусы  
    Common_LED_4_out((uint16_t)Steps_of_stepper);                       // Показываем текущую условную позицию.
}
static void Enter_Stepper_Modus(void)
{
                                                                        // Таймер
    TCCR2 = 0;                                                          // Первоначально очищаем
    TCCR2 |= (1<<CS21);                                                 // 8. 3.9k 0.256с
    TIMSK |= (1<<TOIE2);                                                // прерываение
                                                                        // Железо
    InitPins_for_Stepper();

    Menu_Navigate(MENU_NEXT);
}


static void Enter_Safe_SPEED_sensor_emul_Modus(void)
{                                                                   // Переконфигурируем таймер 2
    TCCR2 = 0;                                                          // Первоначально очищаем
    TCCR2|= (1<<WGM21);                                                 // Сброс по совпадению (CTC).
    TCCR2 |= (1<<CS20)|(1<<CS21)|(1<<CS22);                             // 1024  61Гц. // переполнение при 256 щелчках. 16MHz
    TIMSK |= (1<<OCIE2);                                                // прерываение по совпадению ! Другой обработчик прерывания.
                                                                    
//Далее естественно костыльная строчка. 
    index_mass_speed = eeprom_read_byte(&EE_index_OCR2_mass);               // Загружаем из EEPROM
    OCR2 = OCR_values[index_mass_speed];
    
    if(index_mass_speed>19)Speed_CKP_cntrl_byte|= Speed_HIGH_freq;
    else Speed_CKP_cntrl_byte &= ~Speed_HIGH_freq;      // Не баг ли это?
                                                                        
    DDRD|= (1<<PD2)|(1<<PD3);                                           // Выводы CMP Эмулятора. (транзисторы npn с подтягом 5V)


    #if defined( DebugCommon )
    uart_puts("Speed_sensor_emul_Modus_activated ");    uart_new_line_Macro
    #endif

    Frequency_to_show();   
    Double_LED_out = Led_Green;                                         //Общее для всех. Зелёный - нормальная работа.
}



void Speed_emul_Set(void)   // Показывает частоту от 60 до 5000 ГЦ. Надо максимум 400 ГЦ. И от нуля.
{

    //самая маленькая частота
    //Кнопка два. Значение регистра OCR2 увеличивается, но частота то выходная - уменьшается! Поэтому кнопка 2 - МИНУС.
    if(BtnTp)
    {
        if (index_mass_speed)index_mass_speed--;
        
    }
    else
    {
        if(index_mass_speed<36)index_mass_speed++;
    }

    OCR2 = OCR_values[index_mass_speed];
    
    if(index_mass_speed>19)Speed_CKP_cntrl_byte|= Speed_HIGH_freq;
    else Speed_CKP_cntrl_byte &= ~Speed_HIGH_freq;      // Не баг ли это?


    #if defined( DebugCommon )
     uart_put_int(index_mass_speed);
     uart_putc(' ');
     uart_put_int(OCR2);
     uart_putc(' ');
     uart_putbin_byte(Speed_CKP_cntrl_byte);
     uart_new_line_Macro
    #endif


    eeprom_update_byte(&EE_index_OCR2_mass, index_mass_speed);
    Frequency_to_show();

}

void ShowSparkState(void)
{
LED_string[0] = 10;
    if(SparkByte&0x80)
    {
    LED_string[1] = 10;
    LED_string[2] = 16;
    LED_string[3] = 10;                                                 // Типа молния
    Double_LED_out = Led_Green;                                         //Общее для все отображение "всё работает".
    }
    else
    {
        LED_string[1] = 0;
        LED_string[2] = 15;
        LED_string[3] = 15;
        Double_LED_out = 0x00;
    }
}

void SparkOnOff(void)
{
 if(!BtnTp)
 { SparkByte|=0x80;                                                      // Разрешаем искрообразование
    SparkByte |=0x05;                                                    //Даём два толчка с искрами

 }
 else SparkByte&=0x7F;

 ShowSparkState();


 #if defined( DebugCommon )
 uart_puts("SparkByte "); uart_putbin_byte(SparkByte); uart_new_line_Macro
 #endif

}
void SparkChargeShow(void)
{
        
    uint16_t tmp;
    tmp = SparkCharge *13;
    DIG_ChargTime(tmp);
    LED_string[3]=10;                                                   // Убираем оттуда мусор.

}
void SparkChargeSet(void)
{
    if(!BtnTp)                                                          // Кнопка нуль
    {
    SparkCharge++;
    }
    else
    {
        if(SparkCharge>1)SparkCharge--; 
    }
    eeprom_update_byte(&EE_SparkCharge, SparkCharge);                   // EEPROM!
                                                                        // Показать значение
     SparkChargeShow();
}

void SparkRPMShow(void)
{
    uint16_t tmp;
    uart_put_int(SparkPause);
    uart_new_line_Macro
    tmp = (10000/((SparkPause/4)+2))*12;

    Common_LED_4_out(tmp);
    DotPrintByte&=0xF0;                                                 // Без точек

    #if defined( DebugCommon )
    uart_put_int(SparkPause); uart_new_line_Macro
    #endif
}

void SparkRPMSet(void)                                                  // Отображает уже в оборотах в минуту. Искра для одного цилиндра. 
{

// // 488*0.128 = 125ms //8Hz * 60s= 480 /// 480 *2 оборота = 1000 об/мин.. Здесь ошибка, другой кварц.

    if(!BtnTp)  // Кнопка нуль
    {
        if(SparkPause>50)SparkPause-=50;
    }
    else
    {
        if(SparkPause<850)SparkPause+=50;   
    }
    cli();
    eeprom_update_word(&EE_SparkPause, SparkPause);                         // EEPROM
    sei();
                                                                            // Показать значение
    SparkRPMShow();

}

void Frequency_to_show(void)
{
    uint16_t temp;
         
        if(Speed_CKP_cntrl_byte&Speed_HIGH_freq)temp = 7000/OCR2;
        else temp = 766/OCR2;

        #if defined( DebugCommon )
        uart_put_int(temp);     uart_new_line_Macro
        #endif
        

        Common_LED_4_out(temp);
        DotPrintByte|=0x01;
}



void StraghtContr(void)
{
Button_way_of_interpret &= (~(ButtonPressed_0_MASK | ButtonPressed_1_MASK | ButtonPressed_0_LONG_MASK | ButtonPressed_1_LONG_MASK));
                                                                                //Убираем эти единицы из байта настроек реакции на кнопки
}
void RestoreBtn(void)
    { Button_way_of_interpret = 0xFF; }
void Set_Def_Modus(void)
    { Modus = 0x00; }
void Restore_timer_and_go_UP(void)                                              //Другие прерывания таймеров выключаем
{
    
     TIMSK &= ~(1<<OCIE2);                                                      // В частности прерывание для эмуляции
                                                                                        //Обязательно нормальные кнопки
Button_way_of_interpret = 0xFF;                                                 // Будешь добавлять логику, словишь здесь БАГ.
                                                                                //Все конфигурации в нули
Stepper_ControlByte = 0;                                                        // Убери. Возможно БАГ здесь потому что может быть что-то надо оставить

    RestartTimer_1();                                                           // Восстановит таймер LED
    ChangeModus();                                                              // Сама выйдет на карусель и даст звук и вибро.
}

void Start_current_measuring(void)
{
    adc_for_IR3313_current_SetUp();
    G_ADC_byte|=(1<<Current_sense_ON);                                              // Включаем режим измерения
    ADCSRA |=(1<<ADSC);    //   Начинаем преобразование АЦП
}




void ShowPrescaler(void)
{
    uint16_t freq_IR_PWM = 0;
        switch(current_presc)
        {
            case 001:
            freq_IR_PWM = 62500 ;   ///как??? Разрядов слишком много.
            break;
            case 010:
            freq_IR_PWM = 7812;
            break;
            case 011:
            freq_IR_PWM = 1953;
            break;
            case 100:
            freq_IR_PWM = 975;
            break;
            case 101:
            freq_IR_PWM = 488;
            break;
            case 110:
            freq_IR_PWM = 244;
            break;
        }   
        Common_LED_4_out((uint16_t)freq_IR_PWM);
}

void ShiftPrescaler (void)                                                              // Втупую следующий!
{
    /*
        CS22 CS21 CS20 Description    и частоты таймера при этих предделителях

        001     clkT2S/(No prescaling)  62500
        010     clT2S/8 (From prescaler)    7812
        011     clkT2S/32 (From prescaler)  1953
        100     clkT2S/64 (From prescaler)  975
        101     clkT2S/128 (From prescaler) 488
        110     clkT2S/256 (From prescaler) 244
        111     clkT2S/1024 (From prescaler)    61
        */

    TCCR2 &=~((1<<CS22)|(1<<CS21)|(1<<CS20));                                           //Стираем чтобы поставить вновь
    switch(current_presc)
    {
        case 110:
        current_presc = 001;
        TCCR2|=(1<<CS20);
        break;
        case 001:
        current_presc = 010;
        TCCR2|=(1<<CS21);
        break;
        case 010:
        current_presc = 011;
        TCCR2|=(1<<CS21)|(1<<CS20);
        break;
        case 011:
        current_presc = 100;
        TCCR2|=(1<<CS22);
        break;
        case 100:
        current_presc = 101;
        TCCR2|=(1<<CS22)|(1<<CS20);
        break;
        case 101:
        current_presc = 110;
        TCCR2|=(1<<CS22)|(1<<CS21);
        break;
    }
    
    eeprom_update_word(&EE_current_presc, current_presc);                   // EEPROM!
    ShowPrescaler();
}


void IGBT_PWM_Enter(void)
{
        // Перевыключение если уже было включено 
        IGBT_PWM_cntrl_byte &= ~IGBT_PWM_en;                            // запрещаем прямую логику
        TIMSK &= ~((1<<OCIE2)|(1<<TOIE2));                              // ОБА прерывания сразу!
        RestoreBtn();                                                   // нормальные кнопки.
        Double_LED_out= 0x00;                                           // Отображаем завершение работы
        
    
    
        DDRC |= (1<<PC1)|(1<<PC2);
                                                                        //таймер 2 - два прервывания
        Blincked_Digits = 1;                                            // Моргаем и отображем частоту. 
    
                                                                        // Переконфигурируем таймер 2
        TCCR2 = 0;                                                      // Первоначально очищаем
        TCCR2|= (1<<WGM20)|(1<<WGM21);                                  // FAST PWM

        current_presc = eeprom_read_word(&EE_current_presc);            // Загружаем из EEPROM
        ShowPrescaler();    
}


void IGBT_PWM_Start(void)   
{
    
    IGBT_PWM_cntrl_byte|=IGBT_PWM_en;                                   // Разрешаем логику
    TIMSK |= (1<<OCIE2)|(1<<TOIE2);                                     // ОБА прерывания сразу!
    Double_LED_out = Led_Green;
    Blincked_Digits = 0;

    StraghtContr();
    IGBT_PWM_cntrl_byte|=IGBT_PWM_str_duty;

                                                                        // Первый раз отобразим частоту.
    OCR2 = 128;                                                         // Начинаем с 50% чтобы было удобно.
    Common_LED_4_out((39*OCR2)/100);                                    //вычисляем количество процентов по разрядности 256.
    
}

//Заглушка 1

void DummySender2(void)
{
LED_string[0] = 22;                                 //Очищаем дисплей
LED_string[1] = 22;
LED_string[2] = 22;
LED_string[3] = 2;
IR3313_plain_switch();
}
// Заглушка 2
void DummySender3(void)
{
LED_string[0] = 22;                                 //Очищаем дисплей
LED_string[1] = 22;
LED_string[2] = 22;
LED_string[3] = 3;
IR3313_plain_switch();

}
//заглушка 3
void DummySender3_2(void)
{
    if(IR_ctrl_Byte & IR_ON_bit_PWM)Start_current_measuring();          // Включаем режим измерения
    else
    {
        LED_string[0] = 22;                                 //Очищаем дисплей
        LED_string[1] = 22;
        LED_string[2] = 22;
        LED_string[3] = 3;  
    }
    
}


void ShowFilamentMode(void)
{
    if(I_shutdown_normal_OCR2>22) // Не давать выбрать "Компенсацию Токового Удара Ламп Накаливания" при больших пределах тока.
    {
        Menu_Navigate(MENU_NEXT);
        return;
    }
    if(IR_ctrl_Byte&IR_Filament_setted)
    {
        LED_string[0] = 15;                                 
        LED_string[1] = 1;
        LED_string[2] = 24;         // L
        LED_string[3] = 10;

    }
    else
    {
        LED_string[0] = 25;         //П
        LED_string[1] = 0;
        LED_string[2] = 21;         // L
        LED_string[3] = 15;

    }

}
void ToggleFilamentMode(void)
{
/* Показать и переключить OCR2 Подстройку. Выдать команду на динамичесое управление при влючении.
Динамическое управление - через настройку.
-Скорость включения - фиксированная, но задаётся #define. Для начала 0.1 секунды. 100 ms
-Автоматическая обработка в прерывании
-Выключение и выход на заданную настройку.
*/

IR_ctrl_Byte ^= IR_Filament_setted;
ShowFilamentMode();
}

uint8_t Modus_6_ctrl_byte;

 
void StartShortShoot(void)
{
//железо
    // Настраиваем таймер
    TCCR2 = 0;                                              // Первоначально очищаем
    TCCR2 |=(1<<WGM20)|(1<<WGM21);                          // fast pwm
    TCCR2 |=(1<<COM21);                                     // OC2
    TCCR2 |=(1<<CS20);                                      //без предделителя таймер щёлкает 63kHz
    TIMSK &= ~(1<<TOIE2);                                   // прерываение здесь не надо, PWM

    //Тут тонкий момент. Я клещами смотрел, при OCR2=15 - удары до 22 ампер были. Но прада по времени - 0.00003 с !
    OCR2 = 19;          // Ставим предел - это где-то 5-7 ампер.
    // Задаю OCR2 здесь, чтобы конденсаторы вокруг ОУ успели зарядиться и не было ложного срабатывания при включении

    DDRD|=(1<<7);                                           // вывод для OCR2
    DDRD|= (1<<5);                                          // Затвор полевика для открытия IR3313

    // Далее накопление и паузу выставим по умолчанию

    #if defined( DebugCommon )
    uart_puts("Short_shoot_Modus_activated ");  uart_new_line_Macro
    #endif
    
    
    // Первоначально.  Если не прописывать эти строчки, а вызвавть функцию, то она смещает на 1 единицу, что не нравится.
    LED_string[0]=10;
    LED_string[1]=6;
    LED_string[2] =21;                                      // Чёрточка
    LED_string[3] = 10;


    Modus_6_ctrl_byte |= Modus_6_ctrl_byte_SHOOT_active;  // Флаг поиска
    AntyReverce_relay_ON

}
                                                                        // далее используя макроопределение набираем пункты меню.
                                                                        //Нужно располагать ниже определений или хотя бы объявлений функций из этох определений.
//          name    next    prev    parent      Jump        select_f        enter_f     text
MENU_ITEM(No_Menu, NULL_MENU, NULL_MENU, NULL_MENU, NULL_MENU ,Set_Def_Modus , Menu_Modus_SELECT, "");          // Начальное Ноуменю.

MENU_ITEM(Menu_M1_0, Menu_M1_1, NULL_MENU, NULL_MENU, Menu_M2_0, Enter_PowerOut_Modus, Select_IR_Out_mode, ""); /// по входу применить значения и включить дальнейший режим. 
MENU_ITEM(Menu_M1_1, Menu_M1_10, NULL_MENU, NULL_MENU, Menu_M2_0, Enter_Set_OverCurrent_limit, Set_OverCurrent_limit, ""); //

MENU_ITEM(Menu_M1_10, Menu_M1_2, NULL_MENU, NULL_MENU, Menu_M2_0, ShowFilamentMode, ToggleFilamentMode, ""); // 

MENU_ITEM(Menu_M1_2, Menu_M1_2, NULL_MENU, NULL_MENU, Menu_M2_0, Set_IR_out_mode, NULL, ""); // 
 
 
MENU_ITEM(Menu_M1_3, Menu_M1_3, NULL_MENU, NULL_MENU, Menu_M2_0, NULL, NULL, ""); // Прямое управление. Строка - заглушка

MENU_ITEM(Menu_M1_4, Menu_M1_4, NULL_MENU, NULL_MENU, Menu_M2_0, NULL, DummySender2, ""); /// "Тумблер" кнопкой.

MENU_ITEM(Menu_M1_5, Menu_M1_6, NULL_MENU, NULL_MENU, Menu_M2_0, Show_Duty_for_IR_OUT, Set_Duty_for_IR_OUT, "");    //Изменение предела защиты
MENU_ITEM(Menu_M1_6, Menu_M1_5, NULL_MENU, NULL_MENU, Menu_M2_0, DummySender3_2, IR3313_PWM_switch, ""); /// "Тумблер" кнопкой. Здесь - PWM режим.

MENU_ITEM(Menu_M1_7, Menu_M2_7, NULL_MENU, NULL_MENU, Menu_M2_0, IGBT_PWM_Enter, ShiftPrescaler, ""); // Быстрый ШИМ по IGBT
MENU_ITEM(Menu_M2_7, Menu_M1_7, NULL_MENU, NULL_MENU, Menu_M2_0, IGBT_PWM_Start, NULL, ""); // Быстрый ШИМ по IGBT


//Далее подменю модуса (Stepper_p)
MENU_ITEM(Menu_M2_0, Menu_M2_1, NULL_MENU, NULL_MENU, Menu_M3_0, Enter_Stepper_Modus, NULL, ""); //
MENU_ITEM(Menu_M2_1, Menu_M2_2, NULL_MENU, NULL_MENU, Menu_M3_0, Menu_set_Stepper_speed, set_Stepper_speed, ""); //
MENU_ITEM(Menu_M2_2, Menu_M2_1, NULL_MENU, NULL_MENU, Menu_M3_0, Stepper_Start_Logic, NULL, ""); //


//Далее подменю модуса (Coils_p)
MENU_ITEM(Menu_M3_0, Menu_M3_1, NULL_MENU, NULL_MENU, Menu_M4_0, Enter_Coil_Modus, NULL, ""); //
MENU_ITEM(Menu_M3_1, Menu_M3_2, NULL_MENU, NULL_MENU, Menu_M4_0, SparkChargeShow,SparkChargeSet , ""); // Управление включением
MENU_ITEM(Menu_M3_2, Menu_M3_3, NULL_MENU, NULL_MENU, Menu_M4_0, SparkRPMShow, SparkRPMSet, "");    //Установка частоты
MENU_ITEM(Menu_M3_3, Menu_M3_1, NULL_MENU, NULL_MENU, Menu_M4_0, ShowSparkState, SparkOnOff, "");   // Установка накопления


//Далее подменю модуса  (SPEED_sensor_emul)
MENU_ITEM(Menu_M4_0, Menu_M4_1, NULL_MENU, NULL_MENU, Menu_M1_0, Enter_Safe_SPEED_sensor_emul_Modus, Speed_emul_Set, ""); //
MENU_ITEM(Menu_M4_1, NULL_MENU, NULL_MENU, NULL_MENU, Menu_M1_0, Restore_timer_and_go_UP, NULL, ""); // Управление включением

//Далее подменю модуса  (CKP_emulator_p)
MENU_ITEM(Menu_M5_0, Menu_M5_1, NULL_MENU, NULL_MENU, Menu_M1_0, Enter_CKP_emulator_Modus, SelCar, ""); // Выбираем программу
MENU_ITEM(Menu_M5_1, Menu_M5_2, NULL_MENU, NULL_MENU, Menu_M1_0, StartWaves , Set_RPM_for_CKP, ""); //Установка частоты
MENU_ITEM(Menu_M5_2, Menu_M5_1, NULL_MENU, NULL_MENU, Menu_M1_0, Enter_CKP_emulator_Modus, SelCar, "");

//Тестовый - поиск замыкания
MENU_ITEM(Menu_M6_0, NULL_MENU, NULL_MENU, NULL_MENU, Menu_M1_0, StartShortShoot, NULL, ""); 


uint8_t Check_NO_StepperErrors(void)                                            // Проверяем проводку степпера
{
    if ((SteppePin&(1<<Error1))&&(SteppePin&(1<<Error2)))return 1;              //Единица - подтяг. Шутнированние на массу - ошибка.
    else                                                                        //проиграть ошибку
    {
        Double_LED_out = Led_Red;   //Цвет ошибки.
                                            
                
        #if defined( DebugCommon )
        uart_new_line_Macro uart_puts("Error! Connect stepper properly."); uart_new_line_Macro
        #endif

        SOUND_PlaySong(2);                                                      // Мелодия "пандора".                                           
        LED_string[0] = 11;                                                     //E
        LED_string[1] = 13;                                                     //Г
        LED_string[2] = 21;                                                     //"_"
        if (SteppePin&(1<<Error1))LED_string[3] = 2;                            // От противного
        else if (SteppePin&(1<<Error2))LED_string[3] = 1;
        else LED_string[3] = 3;                                                 // Обе ошибки сразу. Пусть это будет 3.
                            
        return 0;
    }
}
void Menu_set_Stepper_speed(void)
{
    RestoreBtn();                                                               // Обычные кнопки
    Stepper_ControlByte &= ~Stepper_Enable;                                 // Степпер больше не реагирует на кнопки

    if(Check_NO_StepperErrors())
    {
        Blincked_Digits = 1;                                                    // Моргаем для прикола.
        set_Stepper_speed();

        #if defined( DebugCommon )
        uart_new_line_Macro uart_puts("Stepper wiring normal"); uart_new_line_Macro
        #endif
        
    }       
}

void set_Stepper_speed(void)
{
    if(!BtnTp)                                                                  // Кнопка нуль
    {
        if(StepperRotationSpeed<350)StepperRotationSpeed++;                     // Просто ради смеха столько
    }
    else
    {
        if(StepperRotationSpeed>1)StepperRotationSpeed--;
        
    }
    eeprom_update_byte(&EE_StepperRotationSpeed, StepperRotationSpeed);         // EEPROM!
    // Показать значение
    Common_LED_4_out((uint16_t)StepperRotationSpeed);
    LED_string[0] = 22;                                         // Костыль. слэшь -, что это не шаги а скорость.
    #if defined( DebugCommon )
    uart_puts("set_Stepper_speed end");uart_putbin_byte(Double_LED_out);    uart_new_line_Macro
    #endif
            

    
}
void Set_IR_out_mode (void)                                 //Ранее выставили пределы, переключаемся на режим.
{
LED_string[0]=22;
LED_string[1]=LED_string[2]=10;                             //Чистим
LED_string[3] = IR_Out_mode;                                                //Отображаем выбранное

    switch (IR_Out_mode)
    {
        case 1:
                                                                            // Отображать моргающее нижнее подчёркивание
        Blincked_Digits = 1;                                                // Включаем моргание 
        StraghtContr();                                                     // Переводим на прямые кнопки. 
        Menu_Navigate(&Menu_M1_3);                                          //Прыгаем на строку-заглушку. (хотя надо ли?
        IR_ctrl_Byte|=IR_ON_bit_straight;        
        break;
        case 2:
                                                                            //LED_string[3] = 22 ;  // Нижнее подчёркивание - мы ждём нажатия кнопки. Не моргаем.
        IR_Duty = Max_IR_duty;                                              //Обработчик общий с ШИМ режимом. Здесь просто управляем без ШИМ и всё.
        Menu_Navigate(&Menu_M1_4);                                          //Прыгаем на постоянное включение.
        break;
        case 3:
        Menu_Navigate(&Menu_M1_5);                                          //Прыгаем на настройку скважности
        break;
        case 4:
        Menu_Navigate(&Menu_M1_7);                                          // Быстрый ШИМ по IGBT
        break;
    }
                                                                            /*
                                                                            1)Прямое управление
                                                                            2)Постоянное включение
                                                                            3)ШИМ
                                                                            4)Работа при коротком замыкании(будущее)
                                                                            */
}


static void Menu_Modus_SELECT(void)                                         // Вызвается EnterFunction первой заглавной строчки меню NoMenu
{
    if(Modus_is_under_selection == FreshStartedDevice) Menu_move_UP();
 Modus_is_under_selection&=~(1<<7);                                         // Пока убием бит-напоминалку о изменённом модусе
    if(!BtnTp)
    {
        Modus_is_under_selection++;                                         // Изменяем число на экране ++
        if(Modus_is_under_selection > 6)Modus_is_under_selection= 1;
                                                                            // записываем его в буфер индикации
    }
    else
    {
        if (!Modus_is_under_selection)Modus_is_under_selection=1;           // Убрал баг. Не показывало значения при нажатии кнопки - при запуске прибора. (0.5 ч поиска).
        
        Modus_is_under_selection--;
        if(Modus_is_under_selection < 1)Modus_is_under_selection = 6;
                                                                            // Заgисываем его в буфер индикации
    }
         LED_string[0] = Modus_is_under_selection;
         LED_string[1] = 10;
         LED_string[2] = 10;
         LED_string[3] = 10;

    Modus_is_under_selection|=(1<<7);                                       // Модус изменён
    
      #if defined( Debug_moduses_menu )
      uart_putbin_byte(Modus_is_under_selection);uart_new_line_Macro
      #endif
      
}

static void Menu_move_UP(void)                                              //Вышли на карусель выбора модусов
{
    
                                                                            // Выставляем всё железо в нули

    AntyReverce_relay_desengage
    DynamicLedRelease();
    Stepper_ControlByte = 0;    
    IR_ctrl_Byte = 0;                                           //Степпер и IR выключаем
    IR_OFF;                                                                 // IR мог оставаться вкл. без этого.
    G_ADC_byte = 0;                                                         // БЫЛ БАГ. Не выключалось.
    SparkByte &= 0x7F;                                                      // Искры вырубаем
    TIMSK &= ~(1<<OCIE2);                                                   // Всякие датчики коленвала выключаем
    RestoreBtn();                                                           // Нормальные кнопки
    Blincked_Digits = 0;                                                    // Отключаем здесь моргания.

    // Допиши дублирующее отключение IGBT! LOW или HIGH надо? всё время путаюсь.
    IGBT_PWM_cntrl_byte  = 0; 

                                                                            // Вычитываем текущий модус
    uint8_t tmp;
    tmp = Modus & 0b11111100;                                               // отсекаем биты настройки модуса (Проверь, используются ли младшие два?

    switch (tmp)                                                             // Выбираем следующий модус,.
    {   
        case DefaultMode:
        Modus_is_under_selection= 0;
        break;
        case PowerOut:
        Modus_is_under_selection= 1;
        break;
        case Stepper:
        Modus_is_under_selection= 2;
        break;
        case Coils:
        Modus_is_under_selection= 3;
        break;
        case Speed_emulator:
        Modus_is_under_selection = 4;
        break;
        case CKP_emulator:
        Modus_is_under_selection = 5;
        break;
        case ShortShoot:
        Modus_is_under_selection = 6;
        break;
    }

    DotPrintByte = 0;                                                            // без точек
    
    LED_string[0] = Modus_is_under_selection;
    LED_string[1] = 10;
    LED_string[2] = 10;
    LED_string[3] = 10;


       Modus_is_under_selection|= (1<<7);                                       // Поствим старший бит, как напоминалочку, что только что он прочитан.
       Menu_Navigate(&No_Menu);

       #if defined( Debug_moduses_menu )
        uart_puts("Default_Zero_Level_SELECT_MODUS_Button_ 0 / 1 !!! ");  uart_new_line_Macro
       #endif
      
       
                                                                                // Сбрасываем настройки таймера 2, может быть и настройки портов ввода-вывода в будущем
    TCCR2 = 0;                                                                  // выключаем таймер
    TIMSK &= ~((1<<OCIE2)|(1<<TOIE2));                                          // запрещаем оба варианта прерываний от таймера 2
}

void Do_Action(uint8_t flag)
{
        SOUND_PlaySong(4);
        BtnTp = flag;                       // Записываю признак действия
        Menu_EnterCurrentItem();        // Отрабатываю действие 
}

void ChangeModus(void)                                                          // Звуковое оповещение  Вибро оповещение
{
    

    if (Modus_is_under_selection)                                               // Модус изменялся
    {

//В функции switch будет УЖЕ вызываться код из секций "Вход в ... модус осуществён).
// Этот баг стоил мне пол вечера. Нашёл просто аналитически. пробовал терминалом - выявить не смог. Вообще думал, глюк компилятора какой-то. Светодиод не горит и всё!
// Выставляем в нули ДО функции switch!

Double_LED_out = 0x00;                                                          // Может и всё остальное тоже надо инициализировать?
SOUND_PlaySong(3);                                                              // Обратная связь "операция с модусом".

    uint8_t NewModus = 0;                                                       // Инициализируем, чтоыб без warning
    uint8_t tmp = 0;
    tmp = Modus_is_under_selection & 0b00000111;
        switch (tmp)                                                           // Активируем модус,.
        {   
            case 0:
            NewModus = PowerOut;
            Menu_Navigate(&Menu_M1_0);
            break;
            case 1:
            NewModus = PowerOut;
            Menu_Navigate(&Menu_M1_0);
            break;
            case 2:
            NewModus = Stepper;
            Menu_Navigate(&Menu_M2_0);
            break;
            case 3:
            NewModus = Coils;
            Menu_Navigate(&Menu_M3_0);
            break;
            case 4:
            NewModus = Speed_emulator;
            Menu_Navigate(&Menu_M4_0);
            break;
            case 5:
            NewModus = CKP_emulator;
            Menu_Navigate(&Menu_M5_0);
            break;
            case 6:
            NewModus = ShortShoot;
            Menu_Navigate(&Menu_M6_0);
            break;
        }
        Modus_is_under_selection = 0;                                   // ВСё, ничего не надо. И битов тоже
                                                                        // Сбрасываем настройки таймера 2, может быть и настройки потров ввода-вывода в будущем

                                                                        //Конфигурируем таймер 2 под выбранный модус
    Modus = 0;      
    Modus |= NewModus;                                                  //Устанавливаем новый модус
    
    #if defined( Debug_moduses_menu )// Debug
    uart_putbin_byte(Modus);uart_new_line_Macro
    #endif
    }
    else                                                                // Зашли на изменение
    {
        Menu_move_UP();                                                 //Выходим в меню по длительному нажатию кнопки 2 (нижней).
        SOUND_PlaySong(5);
    }   
}

void ButtonRoutine(void)
{
//ButtonCheck возвращает buttonoutput. Предалгаю нейтрализоваывать его при "прямом управлении"
    switch(ButtonCheck()&Button_way_of_interpret)
    {
        case ButtonPressed_0_MASK:
        Do_Action(0);
        Vibro_short_zzz
        break;
        case ButtonPressed_1_MASK:
        Do_Action(1);
        Vibro_short_zzz
        break;
        case ButtonPressed_2_MASK:
        Vibro_short_zzz
                                                                        // Циклическое переключение подрежимов модуса внутри модуса
        Menu_Navigate(MENU_NEXT);
        SOUND_PlaySong(4);
        break;
        case ButtonPressed_0_LONG_MASK:
        SOUND_PlaySong(4);                                              // Если дописывать действия, учесть условия "прямого" упрвления кнопкой.
        Vibro_short_zzz
        break;
        case ButtonPressed_1_LONG_MASK:
        SOUND_PlaySong(4);      
        
        
        Vibro_short_zzz
        break;
        case ButtonPressed_2_LONG_MASK:

        ChangeModus();                                                  // Циклическое переключение  модусов.
        
        Vibro_Long_zzz
        break;
        case (ButtonPressed_0_LONG_MASK| ButtonPressed_1_LONG_MASK):
        break;
        case ButtonPressed_SHORT_Double_MASK:
        wdt_disable();                                                  // Дать возможность буту работать.
        ((void (*)(void))((FLASHEND - 2048)/2))();                       // Перезагрузка - переход к секции бута.
        break;

    }
}

static void Menu_Generic_Write(const char* pointer_to_string)
{   // Generic-функция, связанная с библиотекой.
    // Если хочешь финдиперцовый вывод в несколько строчек, или ещё круче, от Generic точно придётся отказаться, и делать вывод на каждое событие!!!

    if (pointer_to_string)
    {
        
        // Если передать const char* pointer_to_string то функция будет пытаться прейти по значению в первой ячейке строки(как это возможно?)
        // В то время как ей нужен указатель на это значение, чтобы она сама прочитала по этому указателю это занчение. Ей нужно НЕ значение,
        // А указатель! То есть адресс. (это можно прочитать в объявлении функции)
        // А сам указатель передаётся БЕЗ звёздочки. СО звёздочкой передаётся САМ объект, на который указывают. Но ЗДЕСЬ нужен ИМЕННО указатель
        // Поэтому БЕЗ звёздочки и С приведением типов 
    }
}

void Activate_Stepper(void)
{

Stepper_ControlByte &= ~Stepper_move_bit;                               // Убираем заявку на вызов этой функции.


uint8_t pos;
pos = Stepper_ControlByte & 0xF; 
                                                                            // При запуске в байте степпера нету позиции. Это баг. Надо поставить патч:
if (pos == 0x00)pos = 0x01;


        if(!(PINA&(1<<PA0)))                                                //Нажали кнопку
        {
        if(Steps_of_stepper<400)Steps_of_stepper++;
        
        Common_LED_4_out((uint16_t)Steps_of_stepper);
    
            switch(pos)
                {
                case 0x01:              //1
                Phase1_n;
                Phase2_n;
                pos = 0x02;
                Double_LED_out = Led_Green; 
                break;

                case 0x02:              //2 
                Phase1_p;
                Phase2_n;
                pos = 0x04;
                Double_LED_out = 0x00;
                break;

                case 0x04:              //3     
                Phase1_p;
                Phase2_p;
                pos = 0x08;
                Double_LED_out = Led_Red;
                break;

                case 0x08:              //4
                Phase1_n;
                Phase2_p;
                pos = 0x01;
                Double_LED_out = 0x00;
                break;
                }
            }
        
        if(!(PINA&(1<<PA1)))    //Нажали кнопку
        {
                if(Steps_of_stepper)Steps_of_stepper--;
                Common_LED_4_out((uint16_t)Steps_of_stepper);

            switch(pos)
            {
                case 0x01:
                //1
                Phase1_n;
                Phase2_n;
                pos = 0x02;
                Double_LED_out = Led_Green; 
                break;

                case 0x02:
                //2
                Phase1_n;
                Phase2_p;
                pos = 0x04;
                Double_LED_out = 0x00;
                break;

                case 0x04:
                //3
                Phase1_p;
                Phase2_p;
                pos = 0x08;
                Double_LED_out = Led_Red;
                break;

                case 0x08:
                //4
                Phase1_p;
                Phase2_n;
                pos = 0x01;
                Double_LED_out = 0x00;
                break;

                }
        }
        Stepper_ControlByte&=0xF0;                                  // Снесли младший, где положение степпера
        Stepper_ControlByte|= pos;                                   // ЗАгрузили новую позицию.
}



void Do_Straight_Logic(void)
{

    if (Stepper_ControlByte&Stepper_move_bit)Activate_Stepper();        // таймер- флажок. Здесь вызов флажка.

    if(IR_ctrl_Byte&IR_ON_bit_straight)                     //Если можно упралять IR
        {
            if((!(PINA&(1<<PA0)))||(!(PINA&(1<<PA1))))                  //Нажали любую из двух кнопок.
                {
                if(!(IR_ctrl_Byte&IR_outActive))                        //Однократность (Просто используем параметр "IR управляется сейчас)
                    {
                        Blincked_Digits = 0;                                // Больше не моргаем.
                        if (!(G_ADC_byte&(1<<Current_sense_ON))) Start_current_measuring();     //Если ещё не включено включаем (однократность)
                        Ir_3313_Start();                                    // Обслуживает пуск лампы накала, если задано
                    }   
                                                                        
                }
            else
                {
                    IR_OFF;
                    IR_ctrl_Byte&=~IR_OVERCURRENT;//  убираем флаг аварии
                    //Double_LED_out = 0x00;                                // Выключен IR - выключен и LED
                    
                    // Однократно!
                    if (G_ADC_byte&(1<<Current_sense_ON))
                    {
                    G_ADC_byte &= ~(1<<Current_sense_ON);
                    LED_string[0] = 22;                                                         
                    LED_string[1] = 10;
                    LED_string[2] = 10;
                    LED_string[3] = 1;
                    }
                }
        }
    if((IGBT_PWM_cntrl_byte&IGBT_PWM_en)&&(IGBT_PWM_cntrl_byte&IGBT_PWM_str_duty))
    {
        static uint8_t tmp;
        tmp =OCR2;
        if(!(PINA&(1<<PA0)))                                                //Нажали кнопку 0
        {   
            if(OCR2<255)OCR2++;
            _delay_ms(20);

        }
        if(!(PINA&(1<<PA1)))                                                //Нажали кнопку 1
        {
    
            if(OCR2)OCR2--;
            _delay_ms(20);

        }
        if(tmp!=OCR2)Common_LED_4_out((39*OCR2)/100);     //вычисляем количество процентов по разрядности 256.

    }

}
void EEPROM_reset_routine(void)
{
                                                    //Во-первых инициализируем кнопки.
    DDRA &=~( (1<<SA1)|(1<<SA2)|(1<<SA3) );         // Кнопки
    PORTA|= (1<<SA1)|(1<<SA2)|(1<<SA3);             // Подтяги (есть аппаратные, но это для отказоустойчивости)
                                                    //Далее сразу проверяем условия
    if(!(PINA&((1<<SA1)|(1<<SA2)|(1<<SA3))))        //При загрузке зажаты все три кнопки - стереть ЕЕПРОМ.
        {                                                       //Длительный  финдиперц-вжи-и-и-к --вжи-и-и-к --вжи-и-и-к  вибратор
            DDRD|=(1<<PD4);                             //Выход на ВИБРОМОТОР   PD4 (OC1B)
            TCCR1A = 0;                                 //Настраиваем таймер
            TCCR1B = 0;
            TCCR1A |= (1<<COM1B0);                      // Toggle oc1b    // WGM никакие не ставим - режим работы нормальный.
            TCCR1B |= (1<<CS11);                        //предделитель 8
                                                        //Паузу для тактильной отдачи выполненной команды.
            _delay_ms(2000);
            TCCR1A = 0;                                 //Всё, отвибрировали, выключаем вибро.
            TCCR1B = 0;                                 //Вырубаем таймер
        
            EEPROM_rewrite_from_ROM();                  // Массовая запись еепром. 
            _delay_ms(100);                             //Занимаем процессор nop nop - спокойно пишем EEPROM.           
        }   
}








void Do_Short_shoot_logic(void)
{

    // Если выбран модус
    if(Modus&ShortShoot)
    {

            ShortShootTimer++;

            if((ShortShootTimer==500)&&(Modus_6_ctrl_byte&Modus_6_ctrl_byte_SHORT_PRESENT))             // у нас таймер. Отщёлкав 500 ms мы перевлючаем
            {
                IR_OFF
                IR_ctrl_Byte &= ~IR_OVERCURRENT;// на дурака убираем флаг аварии.
            }


            if(ShortShootTimer>1200)                // нас в главном цикле дёргают. Вроде 500 ms
            {
                ShortShootTimer = 0;
                Debug_toggle_yellow;
        
                //Тут тонкий момент. Я клещами смотрел, при OCR2=15 - удары до 22 ампер были. Но прада по времени - 0.00003 с !
            //  перенёс в инициализцию модуса OCR2 = 15;            // Ставим предел - это где-то 5-7 ампер.


                IR_ON           // включаем вывод ( и устанавливаем бит - active)
                return;
            }
    
        
        
        if(IR_ctrl_Byte&IR_outActive)       // Именно сейчас IR3313 управляется сигналом вкл - смотрим что там.
        {
                    uint8_t tmp = PINB;
                    if(tmp&(1<<PB5))                                // вывод IR3313 выключен - он в режиме overcurrent
                    {
                         Double_LED_out = Led_Red;
                 
                         if(!(Modus_6_ctrl_byte&Modus_6_ctrl_byte_SHORT_PRESENT))       // Если нужно однократное действие
                         {
                             SOUND_PlaySong(3);         // бип   - убипрем сирену. Молчим
                             Modus_6_ctrl_byte &= ~Modus_6_ctrl_byte_sound_ON;  //
                             DynamicLedRelease();                               // И дисп включим, больше для дебага
                              #if defined( DebugCommon )
                              uart_puts("Circuit shorted");     uart_new_line_Macro
                              #endif
                         }
                         Modus_6_ctrl_byte|=Modus_6_ctrl_byte_SHORT_PRESENT;
                    }
                    else                                                        // IR включена нормально
                    {
                                Double_LED_out = Led_Green;
                                Modus_6_ctrl_byte&=~Modus_6_ctrl_byte_SHORT_PRESENT;
                         
                                 // Издавать истошный вопль - аварии нет.
                                 if(!(Modus_6_ctrl_byte&Modus_6_ctrl_byte_sound_ON))
                                 {
                                     DynamicLedHalt();          // Очень чисто орём
                                     SOUND_PlaySong(1);         // Сирена
                                     Modus_6_ctrl_byte|=Modus_6_ctrl_byte_sound_ON;     // Однократность.

                                     #if defined( DebugCommon )
                                     uart_puts("Circuit normal");       uart_new_line_Macro
                                     #endif
                                 }              
                     }  
            }
       }
}

uint16_t DebugINT = 0;

 int main (void)
 {

    
//////////////////////////   H A R D W A R E   I N I T   /////////////////////////////////

Hardware_init();
Hardware_set_for_shift();
InitPins_for_Stepper();
Set_Timer_1();                                                      //Для динамической индикации
adc_ext_voltage_SetUp();                                            // Запускаем АЦП в режиме non-stop

#if defined( Sound_ON )
SOUND_Init();
#endif
/* Set up the default menu text write callback, and navigate to an absolute menu item entry. */
Menu_SetGenericWriteCallback(Menu_Generic_Write);
Menu_Navigate(&No_Menu);


//////////////////////////////////////////////////////////////// E E P R O M /////////////
EEPROM_reset_routine();
EEPROM_mass_read();                                                 // Разом восстанавливаемся из энергонезависимой памяти
//************************************************************************************

sei();


SOUND_PlaySong(4);                                                  // Приветствие после запуска

#if defined( DebugCommon )
uart_init((UART_BAUD_SELECT((BAUD),F_CPU)));
uart_puts("Uart_active");       uart_new_line_Macro
#endif


Vibro_short_zzz



//****************************** К О Н Ф И Г У Р А Ц И Я *******************************************************
Modus_is_under_selection = FreshStartedDevice;                      // чтобы при нажатии кнопок 0 и 1 он знал, что надо перегрузить

G_ADC_byte|=(1<<Voltage_sense_ON);                                  // включаем режим замера (смотри прерывание таймера2)
DotPrintByte = 0x00;                                                // Убираем проверочные точки.

Button_way_of_interpret |=0xFF;                                     // Реагируем по-умолчанию по отпусканию кнопок

wdt_enable(WDTO_500MS);                                             //Раскомментить только если программатор рядом. И можно зашить БУТ снова

DynamicLedRelease();                                            // Так поздно для чистого звука
PORTB |=(1<<4);                                                 // Включаем все 595
ADC_readed = 1023; //Необходимая хитрость, чтобы отработала логика и вызвала процедуру - проверялку сгоревшего преда.


    while(1)
    {
    wdt_reset();                                                    // сбрасываем собачий таймер
    
    
    Do_Short_shoot_logic();
                                                                // Vibro_back_response
    if(Vibro_back_response) Vibro_back_response--;                  // Если выключаешь этот таймер, запрети эту логику
    else {PORTD &= ~(1<<PD4);}                                      //Выключить вибро.
    
    ButtonRoutine(); 
    
    if(G_ADC_byte&(1<<0))                                           // АЦП поймало что-то новое
    {
        G_ADC_byte &= ~(1<<0);                                      // Очищаем бит нового значения АЦП!!!
        if(G_ADC_byte&(1<<Voltage_sense_ON))
         {
             DIG_num_for_ADC((uint16_t)ADC_readed);             //Отрабатываем новое значение из АЦП, если есть
             if (ADC_readed<100)  // Меньше трёх вольт - нет или перегорел левый предохранитель - на IR3313
             {
                // Не будем церемониться с интеграторами - орём 

                DynamicLedHalt();           // Очень чисто орём
                SOUND_PlaySong(1);          // Сирена
            /// G_ADC_byte &= ~(1<<0);    // Эту строчку по ошибке вписал
             }
         }
        if(G_ADC_byte&(1<<Current_sense_ON))Show_current((uint16_t)ADC_readed);
        //uart_put_int(ADC_readed);     uart_new_line_Macro             //Debug only
    }

    Do_Straight_Logic();                                                            //Cтеппер, прямой IR

    /// БАГ. Разберись с оверкаррент и интергратором.


                                                                                    //Логика по IR
uint8_t tmp;
    if(IR_ctrl_Byte&(IR_ON_bit_straight|IR_ON_bit_PWM|IR_ON_bit_plain))                 //Логика управления включена
    {
            if(IR_ctrl_Byte&IR_outActive)       // Именно сейчас IR3313 управляется сигналом вкл.
                {
                     tmp =PINB;
                     DebugINT ++; // Считываю количество итераций интегратора.
                    if(IR_alert_debouncer((tmp&(1<<PB5))))                              // MAX раз подтвердилось и подтверждается каждый раз 
                    {
                        Double_LED_out = Led_Red;
                                                                                    // Мелодия однократно
                        if(!(IR_ctrl_Byte&IR_OVERCURRENT))
                        {
                            //DynamicLedHalt();                         // Перевлючать LED - переделывать Sound.c -лень было.               
                            SOUND_PlaySong(2);                                      //Обратный сигнал.
                            

                            #if defined( DebugCommon )
                            uart_puts("IR 3313 overcurrent occured");       uart_new_line_Macro
                            #endif

                            IR_ctrl_Byte &= ~IR_ON_bit_PWM;             // Иначе постоянное перевключение несмотря на сработку
                            //IR_ctrl_Byte &= ~IR_ON_bit_straight;      // Нельзя убирать - ставится один раз при входе.
                            //IR_ctrl_Byte &= ~IR_ON_bit_plain;         // Если убинрать, то надо интегратор самому обнулять.
                            uart_put_int(DebugINT);         uart_new_line_Macro
                            //IR_OFF                                                    // У меня баг - сработка при нормальном пуске. Пытаюсь вычислить
                        }
                        IR_ctrl_Byte|=IR_OVERCURRENT;
                        
                        //это и не обязательно - антидребезг возвращает нуль во всех промежуточных точках, хоть это и не правильно.
                        
                        
                    }
                    else                                                            // много раз считали 1 и считываем далее. 
                    {
                            Double_LED_out = Led_Green;                             // - IR включён и на выходе у нас 12V
                            //IR_ctrl_Byte&=~IR_OVERCURRENT;// Отказался. Вручную убираю флаг. Потому что перевключение при ШИМ. 
                            
                    }
                                    
                }
                else Double_LED_out = 0x00;                                         // выключеное состояние IR  // Проверить на режиме 1-4, возможены баги.
    }
                                
}    
}
