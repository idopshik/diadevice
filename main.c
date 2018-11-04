/*
 * Dia_device.c
 *
 * Created: 14.10.2017 0:01:43
 * Author : isairon
 */ 
/*
...................BUGs.................................
*/

//option ���������� (�����������, ���� �� ����)
//#define Debug595LED                           // �� �������� ���������� ��������� (������ 595). x16 slower
//#define Debug_moduses_menu                    //��������� �������� �������� ���������� "Modus_is_under_selection". � ����� ��� ������ ����.
//#define DebugCommon
#define Sound_ON

#define F_CPU 16000000UL                        // 16 MHz, ���������� RC-�������
#include <avr/io.h>

#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "avr/wdt.h"

#include <stddef.h>                             // �������� ����� � ���
#include <stdint.h>


#include "LED_shift.h"
#include "HW_definitions.h"
#include "Button_input.h"
#include "ADC.h"
#include "BipolarStepper.h"
#include "defs.h"
#include "uart.h"
#include "uart_addon.h"                          // ������������� DEBUG
#include "MicroMenu.h"                           //MICRO-MENU V2 Dean Camera, 2012. ������� ������.
#include "sound.h"                               // �������� ������



//--------------------------------����������������� ������.-----------------------------------
unsigned char EE_OverCurrent_limit_OCR2_value EEMEM;
unsigned char EE_IR_Duty EEMEM; 
unsigned char EE_StepperRotationSpeed EEMEM;  
unsigned char EE_SparkCharge EEMEM;
unsigned int  EE_SparkPause EEMEM ;
unsigned char EE_index_OCR2_mass EEMEM;




unsigned int EE_current_presc EEMEM;







//---------------���������� ���������� ��������� � ���������-------------------//

// 1 - ������� �������� ������� ������������ � ���������� ����� ����������
uint8_t const Current_Limits_PWM_OCR_values[] = {12,14,15,16,17,18,21,23,24,25,26,27,28,45,50,55};  //��������� ���� ��� ������ ������ �� ��������� ������.
uint8_t const Inrush_currents_mass[] =          {0 ,0 ,20,28,40,45,47,50,23,24,26,28,30,32,34,38};

uint8_t const Current_Limits_Actual_Currents[] ={0 ,0 ,0 ,1 ,5,10,15,20,25,30,35,40,45,55,60,70};  //������ ������� ��. ���� /20 ����� ������ � ����������� 256

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
volatile uint16_t SparkPause;                                   //= 520 488*0.256 = 125ms //8Hz * 60s= 480 /// 480 *2 ������� = 1000 ��/��� ����� ������, ������ �����

unsigned int current_presc;         // ��� ���� ���������� ����������.

//--------------- ���������� �������� � ������ ���������� ����������.�-------------------//
volatile unsigned char BtnTp;                                    // ��� ������� ������. (���� ��� �����)
volatile unsigned char Button_way_of_interpret;                  // ���� 0 1 2 ��� ������. ������� � ��� - ������������ �� ����������!
volatile static unsigned char Double_LED_out;
volatile unsigned char RGB_small_counter;
volatile unsigned char RGB_value = 3;
volatile unsigned int RGB_Large_prog_counter = 0;
volatile unsigned char RGB_COLOR0 =0;
volatile unsigned char RGB_COLOR1 =0;
volatile unsigned char Nested_shift_color_cntr =0;

volatile unsigned char Flirty_draw_cntr =0;
volatile unsigned char Digit = 0;                               //������ ��� ��������������������� ������.
volatile unsigned char LED_string[4];                           //������� ������ ������.
volatile unsigned int ADC_readed;                               // ����������� ����������� ��������
volatile unsigned char DotPrintByte;                            //���������� ����� � �������� ������ �����
volatile unsigned char ADC_punch_delay;                         // ������� �������� ����� ���
volatile unsigned int Timer2_nested_Counter = 0;
volatile unsigned char Modus_is_under_selection = 0;            // �������� ���������� � ���������� �����
volatile unsigned char VersatileDevider = 0;
volatile unsigned char Ir_duty_counter = 0;                     //inside timer 1
volatile unsigned char Symb_to_draw =0;                         //temp. ������ ��� flirty effect
volatile unsigned char IR_state_of_output;
volatile int DebugInt = 0;
volatile unsigned char Blincked_Digits = 0;                         //Blincked_Digits

volatile unsigned int Vibro_back_response;
volatile static uint16_t blink_counter;
volatile unsigned int Steps_of_stepper = 200;                   // �������� � ����� �����

uint16_t ShortShootTimer;
unsigned char IR_Duty = Max_IR_duty;                            // �� ��������� ��� ��������� ������ 100%. � ��������� ����������
uint8_t Timer2_nested_Counter_antibug;

//---------------��������� �������-------------------//
void Acceleration_to_LED (signed char acceleration_output);
void ReadAxis(void); 
void Get_time (void);
void ONE_WIRE_DO_IT_HERE(void);
void Threshold_reducer (void);
void Change_Mode_of_Operation(uint8_t NewMode);
void Inrush_current_handler (void);

//---------------��������� �� "�������" �������------------------//
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


void EEPROM_rewrite_from_ROM(void)                              //�������������� ����� ������
{
     eeprom_write_byte(&EE_OverCurrent_limit_OCR2_value,20);    //������ - 256 - ��� �������. ����� 15 ����� �� ��� �� �� �����.
    
     eeprom_write_byte(&EE_IR_Duty,5);                          //����� ������ ������� ����������
    
     eeprom_write_byte(&EE_StepperRotationSpeed,0x78);          //120 dec
     
     eeprom_write_byte(&EE_SparkCharge,0x0F);                   //������ ���������� 2��.                    
      
     eeprom_write_word(&EE_SparkPause,500);                     //������������� � ����������� ����� 1000 (���� ��/���).

     eeprom_write_byte(&EE_index_OCR2_mass,12);                 // ����� �� ����.

     eeprom_write_word(&EE_current_presc, 100); 
}

//***************************************** INTERRUPT SERVICE ROUTINES **********************
ISR(TIMER1_OVF_vect)                                                        //1ms   // ����������� ��������� � ����������
{

    Within_ISR_button_Long_press_monitor();
    Inrush_current_handler();   
                                                                        
    ADC_punch_delay++;                                                      //������ ����� ���
    if (ADC_punch_delay>125)                                                // ������� ����� � ��������� �������� �������)
    {
        if((G_ADC_byte&(1<<Current_sense_ON))||(G_ADC_byte&(1<<Voltage_sense_ON)))ADCSRA |=(1<<ADSC);    //   �������� �������������� ���
        ADC_punch_delay = 0;                                                 // �� � ��������
    }
                                                            
                                                                
    if(IR_ctrl_Byte & IR_ON_bit_PWM)                                        //IR  - T2 ����� ��� �������� ���������� �� ������������                        
    {                                                                       //  IR3313 �� ������� 200 ��. �������  50 ��. (���� ��������� �� ����� ���������    
            Ir_duty_counter++;
            if(Ir_duty_counter>Max_IR_duty){Ir_duty_counter = 0; IR_ON;}        
            if(Ir_duty_counter>IR_Duty){IR_OFF;}                    
    }

    if (DynamicLed_ctrl_byte&Led_run)                                                           // ���� ��� ������� 
    {
         if (!(Modus&0xFC))RGB_LED_idle_fading();                                   // ���� ������� ����� �� ������� (� ��������� �������� ���� ���������)
          
         Digit++;                                                               //��������� ������� ��� ��������������������� ������.
         if(Digit==4) Digit=0;
         uint8_t tmp;                                                           //������ �����
         tmp = 0x01;
         tmp<<=Digit;

         tmp &= DotPrintByte;                                                   // �������� ��� � ��������� �����.
         
         if (Blincked_Digits)
         {
             blink_counter++;
             if(blink_counter>500) blink_counter = 0;                           // ��������, ��������
             if(blink_counter>350) PutOneDigit(10,Digit,tmp,Double_LED_out);        // ������� �������.
             else PutOneDigit(LED_string[Digit],Digit,tmp,Double_LED_out);                                                  //���������� �����
         }
         else                                                                       //�� �������
         {
             if ((Modus&Speed_emulator)&&(Digit == 0)&&(LED_string[0] == 0))FlirtyEffect_1();       // ���� �� ������
             else
             PutOneDigit(LED_string[Digit],Digit,tmp,Double_LED_out);
         }
    }

}


ISR(TIMER2_OVF_vect)                                                // ��������� ������.
{
    if (IGBT_PWM_cntrl_byte&IGBT_PWM_en)
    {
        IGBT_LOW_LEVEL;
    }
    else                                                            // �� ��������� 
    {                                                               // Spark (0,256 ms ������������)  0.128 ������������ - ������ �����!
        Timer2_nested_Counter++;
        Timer2_nested_Counter_antibug++;
        
        if(((Timer2_nested_Counter>SparkCharge)&&(SparkByte&0x01))||(!(SparkByte&0x80))) // ������ ������ �� ���������� � ������� �����
        {
            PORTC |= (1<<PC1);                                      //N-chanel Mosfet - 1 �� ���� � �� ������. �� � ��� ������������� �������
            PORTC |= (1<<PC2);                                      //����� 1 �� �������, �� ����� 0 �� ������, ��� ����������� � �����!!!
            
            Timer2_nested_Counter = 0;                              // ��������
            SparkByte &= 0xFE;                                      // ������� ���� ������1
            SparkByte |= 0x2;                                       //������ ��������1
        }

        if((Timer2_nested_Counter>SparkPause)&&(SparkByte&0x82))     // ������ ������ �� ����� � ������� ����� � ���� ����������
        {
            PORTC &= ~(1<<PC1);                                     //��� �������. 0 �� ����, �� ��� 1, mosfet ���������� - �����������
            PORTC &= ~(1<<PC2);                                     //
            SparkByte &= 0xFD;                                      // ������� ���� ������1
            SparkByte |= 0x1;                                       //������ ����������1// ���� ���� ������� ��� - ��� ������ �����.
            Timer2_nested_Counter = 0;                              // ��������
        }
        
        if(Stepper_ControlByte & Stepper_Enable)                    // ���� ������ ������ ���-����.
        {
            
            if (Timer2_nested_Counter_antibug >StepperRotationSpeed)
            {
                Timer2_nested_Counter_antibug = 0;                  //  ��������
                Stepper_ControlByte|= Stepper_move_bit;         // �������� �� �������� ����.
            }
        }
    }
}

ISR(TIMER2_COMP_vect)
{

                                                                            // �������������� ���. ������� ������� � ����� ���� ������� ������������� ����� ��������� � ������������� ������ 
                                                                            //�����������. ������ ������������ ����� ��������� �� ����� ��������� � ������������� ������� �����������.
                                                                            //������� ��������� ����� 60+2.

    if(Modus&CKP_emulator)                                                  // ���� ����������� ���� �����
    {   
        switch(Car_to_emulate)
        {
            case 1: vaz2108();          
            break;
            case 2: car2();
            break;
        }
    
    }       

    else if(Modus&Speed_emulator)                                           // ���� ����������� ���� �����
    {
    //   61��- ����������� �������, ���� �� ������������ ��������, � ������� ��� ����� ������ - 30.5 ��. 

        VersatileDevider++;

        if((VersatileDevider>8)||(Speed_CKP_cntrl_byte&&Speed_HIGH_freq))  // ����� ������ ����� ��������.
        {
            VersatileDevider = 0;
            if(Double_LED_out)Double_LED_out = 0x00;
            else Double_LED_out = Led_Green;
            

            if(Emul_temp_dummy_var)
                {
                    Emul_temp_dummy_var = 0;
                    PORTD &= ~((1<<PD2)|(1<<PD3));                                  //������� ��� ���������
        
                }
                else
                {
                    PORTD|= (1<<PD2)|(1<<PD3);
                    Emul_temp_dummy_var = 1;
                }
        }
    }   
                                                                                            // ������ ������ ��� ����� IGBT �����������, ��� �� �����.
    else if (IGBT_PWM_cntrl_byte&IGBT_PWM_en)IGBT_HIGH_LEVEL;               // ������ 30��� ��� � ����� �� ����, ��� -��,
                                                                            // ������� ���������� ������ ��� �� ����������. 
}

ISR(ADC_vect)                                                               // ���������� ���������� ���_���������_��������������
{   
                                                                            //C������ ���������.
    uint16_t tmp;
    tmp = ADC ;                                                             // ��������� 10-�� ������ ��������, ���� ��� �� ���������
                                                                            //�������� ���������, ���� �� �����, �������� �����.
    if(tmp!=ADC_readed)
    {ADC_readed=tmp;
    G_ADC_byte |= (1<<0);                                                   //������ ���

    }
}

//*******************************************************************************************
//--------------- ��������� � �������-----------------------//



void DynamicLedHalt (void)
{
    DynamicLed_ctrl_byte &= ~Led_run;
    
    uint8_t tmp;

    for (tmp=0;tmp<14;tmp++)            // ��������� 256 � �������� ������ �� �������� ������
    {
        PutOneDigit(10,tmp,1,Double_LED_out);                           //���������� �����
    }
    
}
void DynamicLedRelease (void)
{
    DynamicLed_ctrl_byte|= Led_run;                                             // Enable dynamic LED
}

void StopTimer_1(void)                                                      //���� ����� �������� � ���������� �� ����
{
    PutOneSymbol(6,4,0x00,0x00);                                            // 6-�������� 4-switch ���� �� default � ��������� ��� ������� 0x00 ��� �����, 0x00 - ���������, �� ����� �����������
    TIMSK&= ~(1<<TOIE1);                                                    //  ����������, �������� ����������
                                                                            //Button_way_of_interpret &= 0xF8; // ������� ���� 0,1 � 2 - ����� �� �������� ������� ������
}

void RestartTimer_1(void)                                                   // ��������� ���������� ��� ������ ������ � �� ������� ������.
{                                                                       //  Button_way_of_interpret |= 0x07; // ����-����� ������ ������. ���� ������, ��� ��� �� ���� ���.
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
    case 1:                                                         // �������� � ����� 0.3 c.
    RGB_COLOR0 = RGB_COLOR1 = 0x00;
    RGB_value=0;                                                    //���������� �������������.
    break;
    case 150:                                                       // ������ �������� ���� ����.
    RGB_COLOR0 = 0x00 ;                                             //�������� � ����
    RGB_COLOR1 = Led_Green;                                         //��������� ������.
    RGB_value=0;                                                    //���������� �������������.
    break;
    case  550:                                                      // ��������� �� ����� � ����.
    RGB_COLOR0 = Led_Green;
    RGB_COLOR1 = Led_Red;
    RGB_value=0;                                                    //���������� �������������.
    break;
    case  950   :                                                   // ������ ����� �������
    RGB_COLOR0 = Led_Red;
    RGB_COLOR1 = 0x00;
    RGB_value=0;                                                    //���������� �������������.
    break;
    case 1500:                                                      // �������� � ����� 0.3 c.
    RGB_COLOR0 = RGB_COLOR1 = 0x00;
    RGB_value=0;                                                    //���������� �������������.
    break;
    case  1650:                                                     // ������ �������� ������.
    RGB_COLOR0 = 0x00;
    RGB_COLOR1 = Led_Red;                                           // ������� ������ �������
    RGB_value=0;                                                    //���������� �������������.
    break;
    case  2050  :                                                   // ��������� �� ����� � ����.
    RGB_COLOR0 = Led_Red;
    RGB_COLOR1 = Led_Green;
    RGB_value=0;                                                    //���������� �������������.
    break;
    case  2450  :                                                   // ������ ����� ������.
    RGB_COLOR0 = Led_Green;
    RGB_COLOR1 = 0x00;
    RGB_value=0;                                                    //���������� �������������.
    
    break;
    case  3000:                                                     // ���� �������� ������.
    RGB_COLOR0 = RGB_COLOR1 = 0x00;
    RGB_Large_prog_counter = 0;                                     // ���� �������� ������.
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
                                                                                        //��� ������������ ���������
    TCCR1A = 0;
    TCCR1B = 0;  
                                                                                        //���� �������� �����������. ����� ���� ��� ������������ ������� ����� ������� ������ - 100��. 
    TCCR1A |= (1<<WGM12)|(1<<WGM10);                                                     // fast pwm 8 bit


        #if defined( Debug595LED )
        TCCR1B&= ~(1<<CS11); TCCR1B |=(1<<CS12);                                        // 1024!
        #else  
        TCCR1B |=(1<<CS10)|(1<<CS11);                                                   // 64       ���������� ������.
        #endif
            
    TIMSK|=(1<<TOIE1);                                                                  // �����������

//  LED_string[0] = {8,8,8,8};                                                          //������-�� ��� �� ����� ��������
    DotPrintByte = 0xFF;
}

uint16_t DIG_digit(uint16_t dig, uint16_t sub,uint8_t DIGIT) {
    char c = 0;
    while (dig >= sub) {
        dig -= sub;
        c++;
    }
    LED_string[DIGIT]=c;                // ����������� �������!
    return dig;
}
void Common_LED_4_out(int16_t num) {        // �������������� ������ ��� �������� ������
    
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

        tmp -=10;                                                       // �������� ��� ����� ������. �� ���� ���������, �� ������� �����������
        tmp-=snum;                                                      // ������ ������� � 15 ��������
        tmp*=27;                                                        // ������  � � �������.
        tmp/=10;                                                        //!!!!!!!! ��������� � ������� �����.
        snum = 1500-tmp;

                                                                        // ������ ����������� ��� �������. ������� ��������� ��������� ����� 15 �����. ����������

        DotPrintByte|=0x02;                                             // ��������� ���� ����� (����������). ��� ����� � �������� ����
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
        if(snum<value)snum = 0; // ��� ��������, �������� ������
        else
        {
            snum -= 10; // ������� ��������
            if(OCR2<20)snum = (13*snum);        // ��� ������ �������� �������� ���������
            else
            {snum = (16*snum);      // ��� ocr = 20 ��� �� ����� ������ �� ������.
            }
            
            DotPrintByte|=0x02;
        }
        
        Common_LED_4_out(snum);
        

        /*  ��������-��������� �� �������
        if (snum>123)snum -=124;   // ��� 30-�� ����������� 123 
        else snum = 0;              // ������ ������. ��������.
        
        if (snum<4)snum = 0;        //����� ������ �� ���������

        //1.86 ������ - �������� 134 � �������
        snum*=19;           // ����� �������� ����� � �������. 
        Common_LED_4_out(snum);
        if(snum<1000)LED_string[0]=10; //������ ���� ���� �����
        DotPrintByte = 0x02; // ����� ����������. ������ � ������ ������.
        */
}


#define IR_alert_debouncerValue_MAX 20          // ��� ���, ������� ��������� � ���� ���.

static uint8_t  Ir_cntr_integr;

char IR_alert_debouncer (unsigned char input)                                   //������� ������
{
    char out;
    
    out = 0;

    if (input == 0)
    {
        if (Ir_cntr_integr > 0)Ir_cntr_integr--;                            //�������� �� ����  � ����� ����� ����.
    }
    else if (Ir_cntr_integr < IR_alert_debouncerValue_MAX) Ir_cntr_integr++;        // �������� �� ��������
    
    
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
    Ir_cntr_integr = 0;  // �������� ������� ������������ ������� ��������� ������ IR
}


//88888888888888888888888888888888888888888888888888888888888888888888888888888888888888
// ��������� ������ ����� ������� ���� � ������ �����.
#include "CodePart_2.h"                                                 // PowerOutModusFunction
#include "CodePart_3.h"                                                 // CKP_EMUL_ModusFunction


void StartWaves(void)
{
    
    TIMSK |= (1<<OCIE2);                                                // ����������� �� ���������� ! ������ ���������� ����������.
    Set_RPM_for_CKP();

    Double_LED_out = Led_Green;                                         //����� ��� ����. ������ - ���������� ������.
}

static void Enter_Coil_Modus(void)
{
                                                                        // ������
    TCCR2 = 0;                                                          // ������������� �������
    TCCR2 |= (1<<CS21);                                                 // 8. 3.9k 0.256�
    TIMSK |= (1<<TOIE2);                                                // �����������
                                                                        // ������
    DDRC |= (1<<PC1)|(1<<PC2); 

     #if defined( DebugCommon )
     uart_puts("Coil_Modus_Active "); uart_new_line_Macro
     #endif                                                                 // ����� ���������� � ����� �������� �� ���������
                                                                //����� ��������� �� ���� ����, �������� ������������ ����������
    Menu_Navigate(MENU_NEXT);
}

static void Stepper_Start_Logic(void)
{
    StraghtContr();
    Blincked_Digits = 0;                                                // �� ������������ �������� � ��������� 
    Stepper_ControlByte |= Stepper_Enable;                          //������ ��� � ������������ ������  
    Common_LED_4_out((uint16_t)Steps_of_stepper);                       // ���������� ������� �������� �������.
}
static void Enter_Stepper_Modus(void)
{
                                                                        // ������
    TCCR2 = 0;                                                          // ������������� �������
    TCCR2 |= (1<<CS21);                                                 // 8. 3.9k 0.256�
    TIMSK |= (1<<TOIE2);                                                // �����������
                                                                        // ������
    InitPins_for_Stepper();

    Menu_Navigate(MENU_NEXT);
}


static void Enter_Safe_SPEED_sensor_emul_Modus(void)
{                                                                   // ����������������� ������ 2
    TCCR2 = 0;                                                          // ������������� �������
    TCCR2|= (1<<WGM21);                                                 // ����� �� ���������� (CTC).
    TCCR2 |= (1<<CS20)|(1<<CS21)|(1<<CS22);                             // 1024  61��. // ������������ ��� 256 �������. 16MHz
    TIMSK |= (1<<OCIE2);                                                // ����������� �� ���������� ! ������ ���������� ����������.
                                                                    
//����� ����������� ���������� �������. 
    index_mass_speed = eeprom_read_byte(&EE_index_OCR2_mass);               // ��������� �� EEPROM
    OCR2 = OCR_values[index_mass_speed];
    
    if(index_mass_speed>19)Speed_CKP_cntrl_byte|= Speed_HIGH_freq;
    else Speed_CKP_cntrl_byte &= ~Speed_HIGH_freq;      // �� ��� �� ���?
                                                                        // ������
    DDRD|= (1<<PD2)|(1<<PD3);                                           // ������ CMP ���������. (����������� npn � �������� 5V)


    #if defined( DebugCommon )
    uart_puts("Speed_sensor_emul_Modus_activated ");    uart_new_line_Macro
    #endif

    Frequency_to_show();   
    Double_LED_out = Led_Green;                                         //����� ��� ����. ������ - ���������� ������.
}



void Speed_emul_Set(void)   // ���������� ������� �� 60 �� 5000 ��. ���� �������� 400 ��. � �� ����.
{

    //����� ��������� �������
    //������ ���. �������� �������� OCR2 �������������, �� ������� �� �������� - �����������! ������� ������ 2 - �����.
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
    else Speed_CKP_cntrl_byte &= ~Speed_HIGH_freq;      // �� ��� �� ���?


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
    LED_string[3] = 10;                                                 // ���� ������
    Double_LED_out = Led_Green;                                         //����� ��� ��� ����������� "�� ��������".
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
 { SparkByte|=0x80;                                                      // ��������� ����������������
    SparkByte |=0x05;                                                    //��� ��� ������ � �������

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
    LED_string[3]=10;                                                   // ������� ������ �����.

}
void SparkChargeSet(void)
{
    if(!BtnTp)                                                          // ������ ����
    {
    SparkCharge++;
    }
    else
    {
        if(SparkCharge>1)SparkCharge--; 
    }
    eeprom_update_byte(&EE_SparkCharge, SparkCharge);                   // EEPROM!
                                                                        // �������� ��������
     SparkChargeShow();
}

void SparkRPMShow(void)
{
    uint16_t tmp;
    uart_put_int(SparkPause);
    uart_new_line_Macro
    tmp = (10000/((SparkPause/4)+2))*12;

    Common_LED_4_out(tmp);
    DotPrintByte&=0xF0;                                                 // ��� �����

    #if defined( DebugCommon )
    uart_put_int(SparkPause); uart_new_line_Macro
    #endif
}

void SparkRPMSet(void)                                                  // ���������� ��� � �������� � ������. ����� ��� ������ ��������. 
{

// // 488*0.128 = 125ms //8Hz * 60s= 480 /// 480 *2 ������� = 1000 ��/���.. ����� ������, ������ �����.

    if(!BtnTp)  // ������ ����
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
                                                                            // �������� ��������
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
                                                                                //������� ��� ������� �� ����� �������� ������� �� ������
}
void RestoreBtn(void)
    { Button_way_of_interpret = 0xFF; }
void Set_Def_Modus(void)
    { Modus = 0x00; }
void Restore_timer_and_go_UP(void)                                              //������ ���������� �������� ���������
{
    
     TIMSK &= ~(1<<OCIE2);                                                      // � ��������� ���������� ��� ��������
                                                                                        //����������� ���������� ������
Button_way_of_interpret = 0xFF;                                                 // ������ ��������� ������, ������� ����� ���.
                                                                                //��� ������������ � ����
Stepper_ControlByte = 0;                                                        // �����. �������� ��� ����� ������ ��� ����� ���� ���-�� ���� ��������

    RestartTimer_1();                                                           // ����������� ������ LED
    ChangeModus();                                                              // ���� ������ �� �������� � ���� ���� � �����.
}

void Start_current_measuring(void)
{
    adc_for_IR3313_current_SetUp();
    G_ADC_byte|=(1<<Current_sense_ON);                                              // �������� ����� ���������
    ADCSRA |=(1<<ADSC);    //   �������� �������������� ���
}




void ShowPrescaler(void)
{
    uint16_t freq_IR_PWM = 0;
        switch(current_presc)
        {
            case 001:
            freq_IR_PWM = 62500 ;   ///���??? �������� ������� �����.
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

void ShiftPrescaler (void)                                                              // ������ ���������!
{
    /*
        CS22 CS21 CS20 Description    � ������� ������� ��� ���� �������������

        001     clkT2S/(No prescaling)  62500
        010     clT2S/8 (From prescaler)    7812
        011     clkT2S/32 (From prescaler)  1953
        100     clkT2S/64 (From prescaler)  975
        101     clkT2S/128 (From prescaler) 488
        110     clkT2S/256 (From prescaler) 244
        111     clkT2S/1024 (From prescaler)    61
        */

    TCCR2 &=~((1<<CS22)|(1<<CS21)|(1<<CS20));                                           //������� ����� ��������� �����
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
        // �������������� ���� ��� ���� �������� 
        IGBT_PWM_cntrl_byte &= ~IGBT_PWM_en;                            // ��������� ������ ������
        TIMSK &= ~((1<<OCIE2)|(1<<TOIE2));                              // ��� ���������� �����!
        RestoreBtn();                                                   // ���������� ������.
        Double_LED_out= 0x00;                                           // ���������� ���������� ������
        
    
    
        DDRC |= (1<<PC1)|(1<<PC2);
                                                                        //������ 2 - ��� �����������
        Blincked_Digits = 1;                                            // ������� � ��������� �������. 
    
                                                                        // ����������������� ������ 2
        TCCR2 = 0;                                                      // ������������� �������
        TCCR2|= (1<<WGM20)|(1<<WGM21);                                  // FAST PWM

        current_presc = eeprom_read_word(&EE_current_presc);            // ��������� �� EEPROM
        ShowPrescaler();    
}


void IGBT_PWM_Start(void)   
{
    
    IGBT_PWM_cntrl_byte|=IGBT_PWM_en;                                   // ��������� ������
    TIMSK |= (1<<OCIE2)|(1<<TOIE2);                                     // ��� ���������� �����!
    Double_LED_out = Led_Green;
    Blincked_Digits = 0;

    StraghtContr();
    IGBT_PWM_cntrl_byte|=IGBT_PWM_str_duty;

                                                                        // ������ ��� ��������� �������.
    OCR2 = 128;                                                         // �������� � 50% ����� ���� ������.
    Common_LED_4_out((39*OCR2)/100);                                    //��������� ���������� ��������� �� ����������� 256.
    
}

//�������� 1

void DummySender2(void)
{
LED_string[0] = 22;                                 //������� �������
LED_string[1] = 22;
LED_string[2] = 22;
LED_string[3] = 2;
IR3313_plain_switch();
}
// �������� 2
void DummySender3(void)
{
LED_string[0] = 22;                                 //������� �������
LED_string[1] = 22;
LED_string[2] = 22;
LED_string[3] = 3;
IR3313_plain_switch();

}
//�������� 3
void DummySender3_2(void)
{
    if(IR_ctrl_Byte & IR_ON_bit_PWM)Start_current_measuring();          // �������� ����� ���������
    else
    {
        LED_string[0] = 22;                                 //������� �������
        LED_string[1] = 22;
        LED_string[2] = 22;
        LED_string[3] = 3;  
    }
    
}


void ShowFilamentMode(void)
{
    if(I_shutdown_normal_OCR2>22) // �� ������ ������� "����������� �������� ����� ���� �����������" ��� ������� �������� ����.
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
        LED_string[0] = 25;         //�
        LED_string[1] = 0;
        LED_string[2] = 21;         // L
        LED_string[3] = 15;

    }

}
void ToggleFilamentMode(void)
{
/* �������� � ����������� OCR2 ����������. ������ ������� �� ����������� ���������� ��� ��������.
������������ ���������� - ����� ���������.
-�������� ��������� - �������������, �� ������� #define. ��� ������ 0.1 �������. 100 ms
-�������������� ��������� � ����������
-���������� � ����� �� �������� ���������.
*/

IR_ctrl_Byte ^= IR_Filament_setted;
ShowFilamentMode();
}

uint8_t Modus_6_ctrl_byte;

 
void StartShortShoot(void)
{
//������
    // ����������� ������
    TCCR2 = 0;                                              // ������������� �������
    TCCR2 |=(1<<WGM20)|(1<<WGM21);                          // fast pwm
    TCCR2 |=(1<<COM21);                                     // OC2
    TCCR2 |=(1<<CS20);                                      //��� ������������ ������ ������� 63kHz
    TIMSK &= ~(1<<TOIE2);                                   // ����������� ����� �� ����, PWM

    //��� ������ ������. � ������� �������, ��� OCR2=15 - ����� �� 22 ����� ����. �� ����� �� ������� - 0.00003 � !
    OCR2 = 19;          // ������ ������ - ��� ���-�� 5-7 �����.
    // ����� OCR2 �����, ����� ������������ ������ �� ������ ���������� � �� ���� ������� ������������ ��� ���������

    DDRD|=(1<<7);                                           // ����� ��� OCR2
    DDRD|= (1<<5);                                          // ������ �������� ��� �������� IR3313

    // ����� ���������� � ����� �������� �� ���������

    #if defined( DebugCommon )
    uart_puts("Short_shoot_Modus_activated ");  uart_new_line_Macro
    #endif
    
    
    // �������������.  ���� �� ����������� ��� �������, � �������� �������, �� ��� ������� �� 1 �������, ��� �� ��������.
    LED_string[0]=10;
    LED_string[1]=6;
    LED_string[2] =21;                                      // ׸������
    LED_string[3] = 10;


    Modus_6_ctrl_byte |= Modus_6_ctrl_byte_SHOOT_active;  // ���� ������
    AntyReverce_relay_ON

}
                                                                        // ����� ��������� ���������������� �������� ������ ����.
                                                                        //����� ����������� ���� ����������� ��� ���� �� ���������� ������� �� ���� �����������.
//          name    next    prev    parent      Jump        select_f        enter_f     text
MENU_ITEM(No_Menu, NULL_MENU, NULL_MENU, NULL_MENU, NULL_MENU ,Set_Def_Modus , Menu_Modus_SELECT, "");          // ��������� �������.

MENU_ITEM(Menu_M1_0, Menu_M1_1, NULL_MENU, NULL_MENU, Menu_M2_0, Enter_PowerOut_Modus, Select_IR_Out_mode, ""); /// �� ����� ��������� �������� � �������� ���������� �����. 
MENU_ITEM(Menu_M1_1, Menu_M1_10, NULL_MENU, NULL_MENU, Menu_M2_0, Enter_Set_OverCurrent_limit, Set_OverCurrent_limit, ""); //

MENU_ITEM(Menu_M1_10, Menu_M1_2, NULL_MENU, NULL_MENU, Menu_M2_0, ShowFilamentMode, ToggleFilamentMode, ""); // 

MENU_ITEM(Menu_M1_2, Menu_M1_2, NULL_MENU, NULL_MENU, Menu_M2_0, Set_IR_out_mode, NULL, ""); // 
 
 
MENU_ITEM(Menu_M1_3, Menu_M1_3, NULL_MENU, NULL_MENU, Menu_M2_0, NULL, NULL, ""); // ������ ����������. ������ - ��������

MENU_ITEM(Menu_M1_4, Menu_M1_4, NULL_MENU, NULL_MENU, Menu_M2_0, NULL, DummySender2, ""); /// "�������" �������.

MENU_ITEM(Menu_M1_5, Menu_M1_6, NULL_MENU, NULL_MENU, Menu_M2_0, Show_Duty_for_IR_OUT, Set_Duty_for_IR_OUT, "");    //��������� ������� ������
MENU_ITEM(Menu_M1_6, Menu_M1_5, NULL_MENU, NULL_MENU, Menu_M2_0, DummySender3_2, IR3313_PWM_switch, ""); /// "�������" �������. ����� - PWM �����.

MENU_ITEM(Menu_M1_7, Menu_M2_7, NULL_MENU, NULL_MENU, Menu_M2_0, IGBT_PWM_Enter, ShiftPrescaler, ""); // ������� ��� �� IGBT
MENU_ITEM(Menu_M2_7, Menu_M1_7, NULL_MENU, NULL_MENU, Menu_M2_0, IGBT_PWM_Start, NULL, ""); // ������� ��� �� IGBT


//����� ������� ������ (Stepper_p)
MENU_ITEM(Menu_M2_0, Menu_M2_1, NULL_MENU, NULL_MENU, Menu_M3_0, Enter_Stepper_Modus, NULL, ""); //
MENU_ITEM(Menu_M2_1, Menu_M2_2, NULL_MENU, NULL_MENU, Menu_M3_0, Menu_set_Stepper_speed, set_Stepper_speed, ""); //
MENU_ITEM(Menu_M2_2, Menu_M2_1, NULL_MENU, NULL_MENU, Menu_M3_0, Stepper_Start_Logic, NULL, ""); //


//����� ������� ������ (Coils_p)
MENU_ITEM(Menu_M3_0, Menu_M3_1, NULL_MENU, NULL_MENU, Menu_M4_0, Enter_Coil_Modus, NULL, ""); //
MENU_ITEM(Menu_M3_1, Menu_M3_2, NULL_MENU, NULL_MENU, Menu_M4_0, SparkChargeShow,SparkChargeSet , ""); // ���������� ����������
MENU_ITEM(Menu_M3_2, Menu_M3_3, NULL_MENU, NULL_MENU, Menu_M4_0, SparkRPMShow, SparkRPMSet, "");    //��������� �������
MENU_ITEM(Menu_M3_3, Menu_M3_1, NULL_MENU, NULL_MENU, Menu_M4_0, ShowSparkState, SparkOnOff, "");   // ��������� ����������


//����� ������� ������  (SPEED_sensor_emul)
MENU_ITEM(Menu_M4_0, Menu_M4_1, NULL_MENU, NULL_MENU, Menu_M1_0, Enter_Safe_SPEED_sensor_emul_Modus, Speed_emul_Set, ""); //
MENU_ITEM(Menu_M4_1, NULL_MENU, NULL_MENU, NULL_MENU, Menu_M1_0, Restore_timer_and_go_UP, NULL, ""); // ���������� ����������

//����� ������� ������  (CKP_emulator_p)
MENU_ITEM(Menu_M5_0, Menu_M5_1, NULL_MENU, NULL_MENU, Menu_M1_0, Enter_CKP_emulator_Modus, SelCar, ""); // �������� ���������
MENU_ITEM(Menu_M5_1, Menu_M5_2, NULL_MENU, NULL_MENU, Menu_M1_0, StartWaves , Set_RPM_for_CKP, ""); //��������� �������
MENU_ITEM(Menu_M5_2, Menu_M5_1, NULL_MENU, NULL_MENU, Menu_M1_0, Enter_CKP_emulator_Modus, SelCar, "");

//�������� - ����� ���������
MENU_ITEM(Menu_M6_0, NULL_MENU, NULL_MENU, NULL_MENU, Menu_M1_0, StartShortShoot, NULL, ""); 


uint8_t Check_NO_StepperErrors(void)                                            // ��������� �������� ��������
{
    if ((SteppePin&(1<<Error1))&&(SteppePin&(1<<Error2)))return 1;              //������� - ������. ������������� �� ����� - ������.
    else                                                                        //��������� ������
    {
        Double_LED_out = Led_Red;   //���� ������.
                                            
                
        #if defined( DebugCommon )
        uart_new_line_Macro uart_puts("Error! Connect stepper properly."); uart_new_line_Macro
        #endif

        SOUND_PlaySong(2);                                                      // ������� "�������".                                           
        LED_string[0] = 11;                                                     //E
        LED_string[1] = 13;                                                     //�
        LED_string[2] = 21;                                                     //"_"
        if (SteppePin&(1<<Error1))LED_string[3] = 2;                            // �� ����������
        else if (SteppePin&(1<<Error2))LED_string[3] = 1;
        else LED_string[3] = 3;                                                 // ��� ������ �����. ����� ��� ����� 3.
                            
        return 0;
    }
}
void Menu_set_Stepper_speed(void)
{
    RestoreBtn();                                                               // ������� ������
    Stepper_ControlByte &= ~Stepper_Enable;                                 // ������� ������ �� ��������� �� ������

    if(Check_NO_StepperErrors())
    {
        Blincked_Digits = 1;                                                    // ������� ��� �������.
        set_Stepper_speed();

        #if defined( DebugCommon )
        uart_new_line_Macro uart_puts("Stepper wiring normal"); uart_new_line_Macro
        #endif
        
    }       
}

void set_Stepper_speed(void)
{
    if(!BtnTp)                                                                  // ������ ����
    {
        if(StepperRotationSpeed<350)StepperRotationSpeed++;                     // ������ ���� ����� �������
    }
    else
    {
        if(StepperRotationSpeed>1)StepperRotationSpeed--;
        
    }
    eeprom_update_byte(&EE_StepperRotationSpeed, StepperRotationSpeed);         // EEPROM!
    // �������� ��������
    Common_LED_4_out((uint16_t)StepperRotationSpeed);
    LED_string[0] = 22;                                         // �������. ����� -, ��� ��� �� ���� � ��������.
    #if defined( DebugCommon )
    uart_puts("set_Stepper_speed end");uart_putbin_byte(Double_LED_out);    uart_new_line_Macro
    #endif
            

    
}
void Set_IR_out_mode (void)                                 //����� ��������� �������, ������������� �� �����.
{
LED_string[0]=22;
LED_string[1]=LED_string[2]=10;                             //������
LED_string[3] = IR_Out_mode;                                                //���������� ���������

    switch (IR_Out_mode)
    {
        case 1:
                                                                            // ���������� ��������� ������ �������������
        Blincked_Digits = 1;                                                // �������� �������� 
        StraghtContr();                                                     // ��������� �� ������ ������. 
        Menu_Navigate(&Menu_M1_3);                                          //������� �� ������-��������. (���� ���� ��?
        IR_ctrl_Byte|=IR_ON_bit_straight;        
        break;
        case 2:
                                                                            //LED_string[3] = 22 ;  // ������ ������������� - �� ��� ������� ������. �� �������.
        IR_Duty = Max_IR_duty;                                              //���������� ����� � ��� �������. ����� ������ ��������� ��� ��� � ��.
        Menu_Navigate(&Menu_M1_4);                                          //������� �� ���������� ���������.
        break;
        case 3:
        Menu_Navigate(&Menu_M1_5);                                          //������� �� ��������� ����������
        break;
        case 4:
        Menu_Navigate(&Menu_M1_7);                                          // ������� ��� �� IGBT
        break;
    }
                                                                            /*
                                                                            1)������ ����������
                                                                            2)���������� ���������
                                                                            3)���
                                                                            4)������ ��� �������� ���������(�������)
                                                                            */
}

static void Menu_Modus_SELECT(void)                                         // ��������� EnterFunction ������ ��������� ������� ���� NoMenu
{
    if(Modus_is_under_selection == FreshStartedDevice) Menu_move_UP();
 Modus_is_under_selection&=~(1<<7);                                         // ���� ����� ���-����������� � ��������� ������
    if(!BtnTp)
    {
        Modus_is_under_selection++;                                         // �������� ����� �� ������ ++
        if(Modus_is_under_selection > 6)Modus_is_under_selection= 1;
                                                                            // ���������� ��� � ����� ���������
    }
    else
    {
        if (!Modus_is_under_selection)Modus_is_under_selection=1;           // ����� ���. �� ���������� �������� ��� ������� ������ - ��� ������� �������. (0.5 � ������).
        
        Modus_is_under_selection--;
        if(Modus_is_under_selection < 1)Modus_is_under_selection = 6;
                                                                            // ��g������� ��� � ����� ���������
    }
         LED_string[0] = Modus_is_under_selection;
         LED_string[1] = 10;
         LED_string[2] = 10;
         LED_string[3] = 10;

    Modus_is_under_selection|=(1<<7);                                       // ����� ������
    
      #if defined( Debug_moduses_menu )
      uart_putbin_byte(Modus_is_under_selection);uart_new_line_Macro
      #endif
      
}

static void Menu_move_UP(void)                                              //����� �� �������� ������ �������
{
    
                                                                            // ���������� �� ������ � ����

    AntyReverce_relay_desengage
    DynamicLedRelease();
    Stepper_ControlByte = 0;    
    IR_ctrl_Byte = 0;                                           //������� � IR ���������
    IR_OFF;                                                                 // IR ��� ���������� ���. ��� �����.
    G_ADC_byte = 0;                                                         // ��� ���. �� �����������.
    SparkByte &= 0x7F;                                                      // ����� ��������
    TIMSK &= ~(1<<OCIE2);                                                   // ������ ������� ��������� ���������
    RestoreBtn();                                                           // ���������� ������
    Blincked_Digits = 0;                                                    // ��������� ����� ��������.

    // ������ ����������� ���������� IGBT! LOW ��� HIGH ����? �� ����� �������.
    IGBT_PWM_cntrl_byte  = 0; 

                                                                            // ���������� ������� �����
    uint8_t tmp;
    tmp = Modus & 0b11111100;                                               // �������� ���� ��������� ������ (�������, ������������ �� ������� ���?

    switch (tmp)                                                             // �������� ��������� �����,.
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

    DotPrintByte = 0;                                                            // ��� �����
    
    LED_string[0] = Modus_is_under_selection;
    LED_string[1] = 10;
    LED_string[2] = 10;
    LED_string[3] = 10;


       Modus_is_under_selection|= (1<<7);                                       // ������� ������� ���, ��� �������������, ��� ������ ��� �� ��������.
       Menu_Navigate(&No_Menu);

       #if defined( Debug_moduses_menu )
        uart_puts("Default_Zero_Level_SELECT_MODUS_Button_ 0 / 1 !!! ");  uart_new_line_Macro
       #endif
      
       
                                                                                // ���������� ��������� ������� 2, ����� ���� � ��������� ������ �����-������ � �������
    TCCR2 = 0;                                                                  // ��������� ������
    TIMSK &= ~((1<<OCIE2)|(1<<TOIE2));                                          // ��������� ��� �������� ���������� �� ������� 2
}

void Do_Action(uint8_t flag)
{
        SOUND_PlaySong(4);
        BtnTp = flag;                       // ��������� ������� ��������
        Menu_EnterCurrentItem();        // ����������� �������� 
}

void ChangeModus(void)                                                          // �������� ����������  ����� ����������
{
    

    if (Modus_is_under_selection)                                               // ����� ���������
    {

//� ������� switch ����� ��� ���������� ��� �� ������ "���� � ... ����� ���������).
// ���� ��� ����� ��� ��� ������. ����� ������ ������������. �������� ���������� - ������� �� ����. ������ �����, ���� ����������� �����-��. ��������� �� ����� � ��!
// ���������� � ���� �� ������� switch!

Double_LED_out = 0x00;                                                          // ����� � �� ��������� ���� ���� ����������������?
SOUND_PlaySong(3);                                                              // �������� ����� "�������� � �������".

    uint8_t NewModus = 0;                                                       // ��������������, ����� ��� warning
    uint8_t tmp = 0;
    tmp = Modus_is_under_selection & 0b00000111;
        switch (tmp)                                                           // ���������� �����,.
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
        Modus_is_under_selection = 0;                                   // �Ѹ, ������ �� ����. � ����� ����
                                                                        // ���������� ��������� ������� 2, ����� ���� � ��������� ������ �����-������ � �������

                                                                        //������������� ������ 2 ��� ��������� �����
    Modus = 0;      
    Modus |= NewModus;                                                  //������������� ����� �����
    
    #if defined( Debug_moduses_menu )// Debug
    uart_putbin_byte(Modus);uart_new_line_Macro
    #endif
    }
    else                                                                // ����� �� ���������
    {
        Menu_move_UP();                                                 //������� � ���� �� ����������� ������� ������ 2 (������).
        SOUND_PlaySong(5);
    }   
}

void ButtonRoutine(void)
{
//ButtonCheck ���������� buttonoutput. ��������� ����������������� ��� ��� "������ ����������"
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
                                                                        // ����������� ������������ ���������� ������ ������ ������
        Menu_Navigate(MENU_NEXT);
        SOUND_PlaySong(4);
        break;
        case ButtonPressed_0_LONG_MASK:
        SOUND_PlaySong(4);                                              // ���� ���������� ��������, ������ ������� "�������" ��������� �������.
        Vibro_short_zzz
        break;
        case ButtonPressed_1_LONG_MASK:
        SOUND_PlaySong(4);      
        
        
        Vibro_short_zzz
        break;
        case ButtonPressed_2_LONG_MASK:

        ChangeModus();                                                  // ����������� ������������  �������.
        
        Vibro_Long_zzz
        break;
        case (ButtonPressed_0_LONG_MASK| ButtonPressed_1_LONG_MASK):
        break;
        case ButtonPressed_SHORT_Double_MASK:
        wdt_disable();                                                  // ���� ����������� ���� ��������.
        ((void (*)(void))((FLASHEND - 2048)/2))();                       // ������������ - ������� � ������ ����.
        break;

    }
}

static void Menu_Generic_Write(const char* pointer_to_string)
{   // Generic-�������, ��������� � �����������.
    // ���� ������ ������������� ����� � ��������� �������, ��� ��� �����, �� Generic ����� ������� ����������, � ������ ����� �� ������ �������!!!

    if (pointer_to_string)
    {
        
        // ���� �������� const char* pointer_to_string �� ������� ����� �������� ������ �� �������� � ������ ������ ������(��� ��� ��������?)
        // � �� ����� ��� �� ����� ��������� �� ��� ��������, ����� ��� ���� ��������� �� ����� ��������� ��� ��������. �� ����� �� ��������,
        // � ���������! �� ���� ������. (��� ����� ��������� � ���������� �������)
        // � ��� ��������� ��������� ��� ��������. �� ��������� ��������� ��� ������, �� ������� ���������. �� ����� ����� ������ ���������
        // ������� ��� �������� � � ����������� ����� 
    }
}

void Activate_Stepper(void)
{

Stepper_ControlByte &= ~Stepper_move_bit;                               // ������� ������ �� ����� ���� �������.


uint8_t pos;
pos = Stepper_ControlByte & 0xF; 
                                                                            // ��� ������� � ����� �������� ���� �������. ��� ���. ���� ��������� ����:
if (pos == 0x00)pos = 0x01;


        if(!(PINA&(1<<PA0)))                                                //������ ������
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
        
        if(!(PINA&(1<<PA1)))    //������ ������
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
        Stepper_ControlByte&=0xF0;                                  // ������ �������, ��� ��������� ��������
        Stepper_ControlByte|= pos;                                   // ��������� ����� �������.
}



void Do_Straight_Logic(void)
{

    if (Stepper_ControlByte&Stepper_move_bit)Activate_Stepper();        // ������- ������. ����� ����� ������.

    if(IR_ctrl_Byte&IR_ON_bit_straight)                     //���� ����� �������� IR
        {
            if((!(PINA&(1<<PA0)))||(!(PINA&(1<<PA1))))                  //������ ����� �� ���� ������.
                {
                if(!(IR_ctrl_Byte&IR_outActive))                        //������������� (������ ���������� �������� "IR ����������� ������)
                    {
                        Blincked_Digits = 0;                                // ������ �� �������.
                        if (!(G_ADC_byte&(1<<Current_sense_ON))) Start_current_measuring();     //���� ��� �� �������� �������� (�������������)
                        Ir_3313_Start();                                    // ����������� ���� ����� ������, ���� ������
                    }   
                                                                        
                }
            else
                {
                    IR_OFF;
                    IR_ctrl_Byte&=~IR_OVERCURRENT;//  ������� ���� ������
                    //Double_LED_out = 0x00;                                // �������� IR - �������� � LED
                    
                    // ����������!
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
        if(!(PINA&(1<<PA0)))                                                //������ ������ 0
        {   
            if(OCR2<255)OCR2++;
            _delay_ms(20);

        }
        if(!(PINA&(1<<PA1)))                                                //������ ������ 1
        {
    
            if(OCR2)OCR2--;
            _delay_ms(20);

        }
        if(tmp!=OCR2)Common_LED_4_out((39*OCR2)/100);     //��������� ���������� ��������� �� ����������� 256.

    }

}
void EEPROM_reset_routine(void)
{
                                                    //��-������ �������������� ������.
    DDRA &=~( (1<<SA1)|(1<<SA2)|(1<<SA3) );         // ������
    PORTA|= (1<<SA1)|(1<<SA2)|(1<<SA3);             // ������� (���� ����������, �� ��� ��� ������������������)
                                                    //����� ����� ��������� �������
    if(!(PINA&((1<<SA1)|(1<<SA2)|(1<<SA3))))        //��� �������� ������ ��� ��� ������ - ������� ������.
        {                                                       //����������  ���������-���-�-�-� --���-�-�-� --���-�-�-�  ��������
            DDRD|=(1<<PD4);                             //����� �� ����������   PD4 (OC1B)
            TCCR1A = 0;                                 //����������� ������
            TCCR1B = 0;
            TCCR1A |= (1<<COM1B0);                      // Toggle oc1b    // WGM ������� �� ������ - ����� ������ ����������.
            TCCR1B |= (1<<CS11);                        //������������ 8
                                                        //����� ��� ���������� ������ ����������� �������.
            _delay_ms(2000);
            TCCR1A = 0;                                 //��, �������������, ��������� �����.
            TCCR1B = 0;                                 //�������� ������
        
            EEPROM_rewrite_from_ROM();                  // �������� ������ ������. 
            _delay_ms(100);                             //�������� ��������� nop nop - �������� ����� EEPROM.           
        }   
}








void Do_Short_shoot_logic(void)
{

    // ���� ������ �����
    if(Modus&ShortShoot)
    {

            ShortShootTimer++;

            if((ShortShootTimer==500)&&(Modus_6_ctrl_byte&Modus_6_ctrl_byte_SHORT_PRESENT))             // � ��� ������. �������� 500 ms �� �����������
            {
                IR_OFF
                IR_ctrl_Byte &= ~IR_OVERCURRENT;// �� ������ ������� ���� ������.
            }


            if(ShortShootTimer>1200)                // ��� � ������� ����� ������. ����� 500 ms
            {
                ShortShootTimer = 0;
                Debug_toggle_yellow;
        
                //��� ������ ������. � ������� �������, ��� OCR2=15 - ����� �� 22 ����� ����. �� ����� �� ������� - 0.00003 � !
            //  ������ � ������������ ������ OCR2 = 15;            // ������ ������ - ��� ���-�� 5-7 �����.


                IR_ON           // �������� ����� ( � ������������� ��� - active)
                return;
            }
    
        
        
        if(IR_ctrl_Byte&IR_outActive)       // ������ ������ IR3313 ����������� �������� ��� - ������� ��� ���.
        {
                    uint8_t tmp = PINB;
                    if(tmp&(1<<PB5))                                // ����� IR3313 �������� - �� � ������ overcurrent
                    {
                         Double_LED_out = Led_Red;
                 
                         if(!(Modus_6_ctrl_byte&Modus_6_ctrl_byte_SHORT_PRESENT))       // ���� ����� ����������� ��������
                         {
                             SOUND_PlaySong(3);         // ���   - ������� ������. ������
                             Modus_6_ctrl_byte &= ~Modus_6_ctrl_byte_sound_ON;  //
                             DynamicLedRelease();                               // � ���� �������, ������ ��� ������
                              #if defined( DebugCommon )
                              uart_puts("Circuit shorted");     uart_new_line_Macro
                              #endif
                         }
                         Modus_6_ctrl_byte|=Modus_6_ctrl_byte_SHORT_PRESENT;
                    }
                    else                                                        // IR �������� ���������
                    {
                                Double_LED_out = Led_Green;
                                Modus_6_ctrl_byte&=~Modus_6_ctrl_byte_SHORT_PRESENT;
                         
                                 // �������� �������� ����� - ������ ���.
                                 if(!(Modus_6_ctrl_byte&Modus_6_ctrl_byte_sound_ON))
                                 {
                                     DynamicLedHalt();          // ����� ����� ���
                                     SOUND_PlaySong(1);         // ������
                                     Modus_6_ctrl_byte|=Modus_6_ctrl_byte_sound_ON;     // �������������.

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
Set_Timer_1();                                                      //��� ������������ ���������
adc_ext_voltage_SetUp();                                            // ��������� ��� � ������ non-stop

#if defined( Sound_ON )
SOUND_Init();
#endif
/* Set up the default menu text write callback, and navigate to an absolute menu item entry. */
Menu_SetGenericWriteCallback(Menu_Generic_Write);
Menu_Navigate(&No_Menu);


//////////////////////////////////////////////////////////////// E E P R O M /////////////
EEPROM_reset_routine();
EEPROM_mass_read();                                                 // ����� ����������������� �� ����������������� ������
//************************************************************************************

sei();


SOUND_PlaySong(4);                                                  // ����������� ����� �������

#if defined( DebugCommon )
uart_init((UART_BAUD_SELECT((BAUD),F_CPU)));
uart_puts("Uart_active");       uart_new_line_Macro
#endif


Vibro_short_zzz



//****************************** � � � � � � � � � � � � *******************************************************
Modus_is_under_selection = FreshStartedDevice;                      // ����� ��� ������� ������ 0 � 1 �� ����, ��� ���� �����������

G_ADC_byte|=(1<<Voltage_sense_ON);                                  // �������� ����� ������ (������ ���������� �������2)
DotPrintByte = 0x00;                                                // ������� ����������� �����.

Button_way_of_interpret |=0xFF;                                     // ��������� ��-��������� �� ���������� ������

wdt_enable(WDTO_500MS);                                             //������������� ������ ���� ������������ �����. � ����� ������ ��� �����

DynamicLedRelease();                                            // ��� ������ ��� ������� �����
PORTB |=(1<<4);                                                 // �������� ��� 595
ADC_readed = 1023; //����������� ��������, ����� ���������� ������ � ������� ��������� - ���������� ���������� �����.




    while(1)
    {
    wdt_reset();                                                    // ���������� ������� ������
    
    
    Do_Short_shoot_logic();
                                                                // Vibro_back_response
    if(Vibro_back_response) Vibro_back_response--;                  // ���� ���������� ���� ������, ������� ��� ������
    else {PORTD &= ~(1<<PD4);}                                      //��������� �����.
    
    ButtonRoutine(); 
    
    if(G_ADC_byte&(1<<0))                                           // ��� ������� ���-�� �����
    {
        G_ADC_byte &= ~(1<<0);                                      // ������� ��� ������ �������� ���!!!
        if(G_ADC_byte&(1<<Voltage_sense_ON))
         {
             DIG_num_for_ADC((uint16_t)ADC_readed);             //������������ ����� �������� �� ���, ���� ����
             if (ADC_readed<100)  // ������ ��� ����� - ��� ��� ��������� ����� �������������� - �� IR3313
             {
                // �� ����� ������������ � ������������� - ��� 

                DynamicLedHalt();           // ����� ����� ���
                SOUND_PlaySong(1);          // ������
            /// G_ADC_byte &= ~(1<<0);    // ��� ������� �� ������ ������
             }
         }
        if(G_ADC_byte&(1<<Current_sense_ON))Show_current((uint16_t)ADC_readed);
        //uart_put_int(ADC_readed);     uart_new_line_Macro             //Debug only
    }

    Do_Straight_Logic();                                                            //C������, ������ IR

    /// ���. ��������� � ����������� � �������������.


                                                                                    //������ �� IR
uint8_t tmp;
    if(IR_ctrl_Byte&(IR_ON_bit_straight|IR_ON_bit_PWM|IR_ON_bit_plain))                 //������ ���������� ��������
    {
            if(IR_ctrl_Byte&IR_outActive)       // ������ ������ IR3313 ����������� �������� ���.
                {
                     tmp =PINB;
                     DebugINT ++; // �������� ���������� �������� �����������.
                    if(IR_alert_debouncer((tmp&(1<<PB5))))                              // MAX ��� ������������� � �������������� ������ ��� 
                    {
                        Double_LED_out = Led_Red;
                                                                                    // ������� ����������
                        if(!(IR_ctrl_Byte&IR_OVERCURRENT))
                        {
                            //DynamicLedHalt();                         // ����������� LED - ������������ Sound.c -���� ����.               
                            SOUND_PlaySong(2);                                      //�������� ������.
                            

                            #if defined( DebugCommon )
                            uart_puts("IR 3313 overcurrent occured");       uart_new_line_Macro
                            #endif

                            IR_ctrl_Byte &= ~IR_ON_bit_PWM;             // ����� ���������� ������������� �������� �� ��������
                            //IR_ctrl_Byte &= ~IR_ON_bit_straight;      // ������ ������� - �������� ���� ��� ��� �����.
                            //IR_ctrl_Byte &= ~IR_ON_bit_plain;         // ���� ��������, �� ���� ���������� ������ ��������.
                            uart_put_int(DebugINT);         uart_new_line_Macro
                            //IR_OFF                                                    // � ���� ��� - �������� ��� ���������� �����. ������� ���������
                        }
                        IR_ctrl_Byte|=IR_OVERCURRENT;
                        
                        //��� � �� ����������� - ����������� ���������� ���� �� ���� ������������� ������, ���� ��� � �� ���������.
                        
                        
                    }
                    else                                                            // ����� ��� ������� 1 � ��������� �����. 
                    {
                            Double_LED_out = Led_Green;                             // - IR ������� � �� ������ � ��� 12V
                            //IR_ctrl_Byte&=~IR_OVERCURRENT;// ���������. ������� ������ ����. ������ ��� ������������� ��� ���. 
                            
                    }
                                    
                }
                else Double_LED_out = 0x00;                                         // ���������� ��������� IR  // ��������� �� ������ 1-4, ��������� ����.
    }

    
                                
}    
}
