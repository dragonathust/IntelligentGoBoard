
enum NOTE_FREQ_t
{
REST = 0, //0
NOTE_AS2, 
NOTE_B2,  
NOTE_C3,  
NOTE_CS3, 
NOTE_D3,  //5
NOTE_DS3, 
NOTE_E3,  
NOTE_F3,  
NOTE_FS3, 
NOTE_G3,  //10
NOTE_GS3, 
NOTE_A3,  
NOTE_AS3, 
NOTE_B3,  
NOTE_C4,  //15
NOTE_CS4, 
NOTE_D4,  
NOTE_DS4, 
NOTE_E4,  
NOTE_F4,  //20
NOTE_FS4, 
NOTE_G4,  
NOTE_GS4, 
NOTE_A4,  
NOTE_AS4, //25
NOTE_B4,  
NOTE_C5,  
NOTE_CS5, 
NOTE_D5,  
NOTE_DS5, //30
NOTE_E5,  
NOTE_F5,  
NOTE_FS5, 
NOTE_G5,  
NOTE_GS5, //35
NOTE_A5,  
NOTE_AS5, 
NOTE_B5,  
NOTE_C6,  
NOTE_CS6, //40
NOTE_D6,  
NOTE_DS6, 
NOTE_E6,  
NOTE_F6,  
NOTE_FS6, //45
NOTE_G6,  
NOTE_GS6, 
NOTE_A6,  
NOTE_AS6, 
NOTE_B6,  //50
NOTE_C7,  
NOTE_CS7, 
NOTE_D7,  
NOTE_DS7, 
NOTE_E7,  //55
NOTE_F7,  
NOTE_FS7, 
NOTE_G7,  
NOTE_GS7, 
NOTE_A7,  //60
NOTE_AS7, 
NOTE_B7,  
NOTE_C8 	
};

/* Duration */
#define D2  (0)
#define D4  (1<<6)
#define D8  (2<<6)
#define D16 (3<<6)

#ifdef ALSA
#include "wave.h"
#else

unsigned char melodyCode[] = {
  'C',
  15|D4, 10|D8, 10|D8, 12|D4, 10|D4, 0|D4, 14|D4, 15|D4
};

unsigned char MoveCode[] = {
  'C',
  24|D16, 26|D16, 27|D16
};

unsigned char DownCode[]={
  'C',
   32|D8, 22|D16
};

unsigned char UpCode[]={
  'C',
   19|D16
};

unsigned char NopCode[]={
  'C',
   24|D4
};

unsigned char UndoCode[]={
  'C',
   24|D2, 24|D2
};

unsigned char BeepCode[]={
  'C',
   46|D2, 46|D2
};

  // Super Mario Bros theme
  // Score available at https://musescore.com/user/2123/scores/2145
  // Theme by Koji Kondo
/*
  //game over sound
  NOTE_C5,-4, NOTE_G4,-4, NOTE_E4,4, //45
  NOTE_A4,-8, NOTE_B4,-8, NOTE_A4,-8, NOTE_GS4,-8, NOTE_AS4,-8, NOTE_GS4,-8,
  NOTE_G4,8, NOTE_D4,8, NOTE_E4,-2,  
*/
  
unsigned char musicCodeLose[]={
  'C',
  NOTE_C5|D4, NOTE_G4|D4, NOTE_E4|D4, //45
  NOTE_A4|D8, NOTE_B4|D8, NOTE_A4|D8, NOTE_GS4|D8, NOTE_AS4|D8, NOTE_GS4|D8,
  NOTE_G4|D8, NOTE_D4|D8, NOTE_E4|D2
};

  //Based on the arrangement at https://www.flutetunes.com/tunes.php?id=192
/* 
  NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
  NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8,  NOTE_A4,4,  NOTE_B4,8,  NOTE_C5,8,

  NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
  NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,

  NOTE_E5,2,  NOTE_C5,2,
  NOTE_D5,2,   NOTE_B4,2,
  NOTE_C5,2,   NOTE_A4,2,
  NOTE_GS4,2,  NOTE_B4,4,  REST,8, 
*/

unsigned char musicCodeWin[]={
  'C',
  NOTE_E5|D4,  NOTE_B4|D8,  NOTE_C5|D8,  NOTE_D5|D4,  NOTE_C5|D8,  NOTE_B4|D8,
  NOTE_A4|D4,  NOTE_A4|D8,  NOTE_C5|D8,  NOTE_E5|D4,  NOTE_D5|D8,  NOTE_C5|D8,
  NOTE_B4|D4,  NOTE_C5|D8,  NOTE_D5|D4,  NOTE_E5|D4,
  NOTE_C5|D4,  NOTE_A4|D4,  NOTE_A4|D8,  NOTE_A4|D4,  NOTE_B4|D8,  NOTE_C5|D8,

  NOTE_D5|D4,  NOTE_F5|D8,  NOTE_A5|D4,  NOTE_G5|D8,  NOTE_F5|D8,
  NOTE_E5|D4,  NOTE_C5|D8,  NOTE_E5|D4,  NOTE_D5|D8,  NOTE_C5|D8,
  NOTE_B4|D4,  NOTE_B4|D8,  NOTE_C5|D8,  NOTE_D5|D4,  NOTE_E5|D4,
  NOTE_C5|D4,  NOTE_A4|D4,  NOTE_A4|D4, REST|D4,

  NOTE_E5|D2,  NOTE_C5|D2,
  NOTE_D5|D2,   NOTE_B4|D2,
  NOTE_C5|D2,   NOTE_A4|D2,
  NOTE_GS4|D2,  NOTE_B4|D4,  REST|D8
};
#endif

