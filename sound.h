#ifndef SOUND_H
#define SOUND_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

//*************************** ��������� *************************************

//#define SOUND_BPM       24  //���� ����������������, �� ������������ ��� ����� 
                              //������������� �� ��������� � ������� BPM`�

#define SOUND_F_CPU     16U //�������� ������� ��
#define SOUND_TIM_PRE   1U  //������� ������������ ������� 

#include "tone.h"   //����� ���������� ������� � ������������ ���

//��� �� �� ������� ����� ���������� ����
#define PORT_SOUND PORTB
#define PINX_SOUND 3

//���������� �������
#define SOUND_AMOUNT_MELODY 6

//***************************************************************************

//������� ��������� ������
#define SOUND_STOP      0
#define SOUND_PLAY      1
#define SOUND_PAUSE     2

//������� ��������� ������
void SOUND_Init(void);
void SOUND_SetSong(unsigned char numSong);
void SOUND_Com(unsigned char com);
void SOUND_PlaySong(unsigned char numSong);

#endif //SOUND_H
