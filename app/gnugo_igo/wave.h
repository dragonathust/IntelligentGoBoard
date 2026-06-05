
void *sound_open(void);
int sound_close(void *handle);
int play_wav(void *handle, unsigned char *file_buf, int length);

void *HandleSound;

unsigned char melodyCode[] = {
  'C',
  15|D4, 10|D8, 10|D8, 12|D4, 10|D4, 0|D4, 14|D4, 15|D4
};

#include "../sound/ClickCode.h"
#include "../sound/MoveCode.h"

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

#include "../sound/UndoCode.h"

unsigned char BeepCode[]={
  'C',
   46|D2, 46|D2
};

