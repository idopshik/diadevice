#ifndef SOUND_H
#define SOUND_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

//*************************** настройки *************************************

//#define SOUND_BPM       24  //если закомментировать, то длительность нот будет 
                              //расчитываться из заданного в мелодии BPM`а

#define SOUND_F_CPU     16U //тактовая частота мк
#define SOUND_TIM_PRE   1U  //зачение предделителя таймера 

#include "tone.h"   //здесь определены частота и длительности нот

//пин мк на котором будет генериться звук
#define PORT_SOUND PORTB
#define PINX_SOUND 3

//количество мелодий
#define SOUND_AMOUNT_MELODY 6

//***************************************************************************

//команды звукового модуля
#define SOUND_STOP      0
#define SOUND_PLAY      1
#define SOUND_PAUSE     2

//функции звукового модуля
void SOUND_Init(void);
void SOUND_SetSong(unsigned char numSong);
void SOUND_Com(unsigned char com);
void SOUND_PlaySong(unsigned char numSong);

#endif //SOUND_H
