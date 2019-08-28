#include "sound.h"

// ћодефицированна€ под таймер 0

#define LOOP 0xff

const PROGMEM unsigned int Sirene2[] = 
        {
            1, LOOP,
            ms(500), c2, ms(500), g2, 
            0
        };

const PROGMEM unsigned int pandora[] =
{
16,1,
n16,C3,  n32,p, n16,B2,   n32,p,   n16,C3,   n32,p,    n16,B2, n32,p,
n16,xD3 ,n8,p, n16,xD3,  n8,P ,n8,C3,
0
};
const PROGMEM unsigned int beep_1[] =
{
	16,1,
	
	n16,C3,n32,p,n16,F3,
	0
};

const PROGMEM unsigned int beep_2[] =
{
	16,1,
	
	n32,C1,n32,p,n32,F1,
	0
};
const PROGMEM unsigned int beep_1_inv[] =
{
	16,1,
	
	n16,F3,n32,p,n16,C3,
	0
};
//-----------------------------звуковой модуль----------------------------------
//указатели на регистры порта
#define PIN_SOUND (*(&PORT_SOUND-2))
#define DDR_SOUND (*(&PORT_SOUND-1))

//заглушка - пуста€ мелоди€
const PROGMEM  unsigned int Empty[] = 
        {
            1, 1,
            n4, p,
            0
        };

PROGMEM  const unsigned int   PROGMEM* const melody[] = {Empty, Sirene2, pandora, beep_1, beep_2, beep_1_inv};

//переменные звукового модул€
volatile static unsigned int *pSong;
volatile static unsigned char state = SOUND_STOP; 
volatile static unsigned int  durationNote = 0; 
volatile static unsigned int  toneNote = 0; 
volatile static unsigned char indexNote = 0;
volatile static unsigned char statReg = 0;
volatile static unsigned char repeat = 0;

#ifndef SOUND_BPM  
  static unsigned char bpm = 0;
#endif


//флаги
#define SOUND_VOLUME     0
#define SOUND_GEN        1

#define SOUND_BPM_SONG    0
#define SOUND_REPEAT_SONG 1
#define SOUND_START_SONG  2
#define SOUND_COUNTER_CAP 256
#define SOUND_PROG_COUNTER 31
  
//инициализаци€ звукового модул€
void SOUND_Init(void)
{
  //настройка вывода мк на выход
  PORT_SOUND &= ~(1<<PINX_SOUND);
  DDR_SOUND |= (1<<PINX_SOUND);
  
  //настройка таймера T0. Ёто dia device, T2 зан€т под RC цепь
  TIMSK |= (1<<TOIE0);   
  TCCR0 = (0<<WGM01)|(0<<WGM00)|(0<<CS02)|(0<<CS01)|(1<<CS00);  //режим - нормал, прескалер - нуль 
  TCNT0 = 0;    
  OCR0 = 0;
   
  //инициализаци€ переменных
  pSong = (unsigned int *)pgm_read_word(&(Empty));
  state = SOUND_STOP;
  durationNote = 0;
  toneNote = 0;
  repeat = 0;
  indexNote = 0;
  statReg = 0;
#ifndef SOUND_BPM  
  bpm = 0;
#endif
}


void SOUND_SetSong(unsigned char numSong)
{
    if (numSong <= SOUND_AMOUNT_MELODY) {
      pSong = (unsigned int *)pgm_read_word(&(melody[numSong]));
    }
}



//обработчик команд звукового модул€
 void SOUND_Com(unsigned char com)
{
unsigned char saveSreg = SREG;
  cli();
  switch (com){
    
    /*команда стоп:*/
    case SOUND_STOP:
      state = SOUND_STOP;
      TIMSK &= ~(1<<OCIE0);
      PORT_SOUND &= ~(1<<PINX_SOUND);
      break;
      
   /*команда воспроизведение*/
    case SOUND_PLAY:
      if (state == SOUND_PAUSE){
        state = SOUND_PLAY;          
        TIMSK |= (1<<OCIE0);
      }
      else {
      #ifndef SOUND_BPM  
        bpm = pgm_read_word(&(pSong[SOUND_BPM_SONG]));
      #endif
        indexNote = SOUND_START_SONG;
        repeat = pgm_read_word(&(pSong[SOUND_REPEAT_SONG]));
        durationNote = 0;
        state = SOUND_PLAY;          
        TIMSK |= (1<<OCIE0);        
      }
      break;  
      
      /*команда пауза*/ 
      case SOUND_PAUSE:
        state = SOUND_PAUSE;  
        TIMSK &= ~(1<<OCIE0);
      break;
      
    default:
      break;
  }
  SREG = saveSreg;
}

//проиграть мелодию под номером numSong
void SOUND_PlaySong(unsigned char numSong)
{
   if (numSong <= SOUND_AMOUNT_MELODY) {
      pSong = (unsigned int *)pgm_read_word(&(melody[numSong]));
   }
   #ifndef SOUND_BPM  
     bpm = pgm_read_word(&(pSong[SOUND_BPM_SONG]));
   #endif
   indexNote = SOUND_START_SONG;
   repeat = pgm_read_word(&(pSong[SOUND_REPEAT_SONG]));
   durationNote = 0;
   state = SOUND_PLAY;          
   TIMSK |= (1<<OCIE0);       
}




inline static void SOUND_Duration(void)
{
  static unsigned char counter = 0;
  
  if (state == SOUND_PLAY){  
    if (durationNote){
       counter++;
       counter &= SOUND_PROG_COUNTER;
       if (!counter){
          durationNote--;   
       }
    }
    else {
      durationNote = pgm_read_word(&(pSong[indexNote]));
      if (durationNote) {
#ifndef SOUND_BPM
        durationNote = durationNote/bpm;
#endif
        indexNote++;
        toneNote = pgm_read_word(&(pSong[indexNote]));
        if (toneNote!=P) {
          statReg |= (1<<SOUND_VOLUME);
        }
        else{
          statReg &= ~(1<<SOUND_VOLUME);
        }
        indexNote++;
        TIFR |=(1<<OCF2); //вот здесь сомнени€
      }
      else{
        if (repeat == LOOP){
          indexNote = SOUND_START_SONG;
          durationNote = 0;
          return;
        }
        repeat--;
        if (!repeat){
          state = SOUND_STOP;
          TIMSK &= ~(1<<OCIE0); 
          PORT_SOUND &= ~(1<<PINX_SOUND);
          return;
        }
        else{
          indexNote = SOUND_START_SONG;
          durationNote = 0;
        }
        
      }        
    } 
 }
  
}


inline static void SOUND_Tone(void)
{
  static unsigned int tone = 0;
  
  if (statReg & (1<<SOUND_GEN)){
    if (statReg & (1<<SOUND_VOLUME)){
       PORT_SOUND ^= (1<<PINX_SOUND);
    }
    tone = toneNote;
    statReg &= ~(1<<SOUND_GEN);
  }

  if (tone > SOUND_COUNTER_CAP) {
    tone -= SOUND_COUNTER_CAP;
  }
  else {
    OCR0 = tone;
    statReg |= (1<<SOUND_GEN);
  }
}

//прерывани€ таймера “0_____________________________________
ISR(TIMER0_OVF_vect)
{
  SOUND_Duration();
  //сюда можно добавить код
  //например, опрос кнопок
}


ISR(TIMER0_COMP_vect)
{
  SOUND_Tone();
}