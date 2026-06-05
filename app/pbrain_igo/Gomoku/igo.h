
//#define DEBUG
#define BOARD_SAMPLE 3
#define BOARD_SAMPLE_THRESHOLD 2
#define TIMEOUT_KEY_PRESS 2500

#define KEY_ACTIVE_HIGH
//#define KEY_MASK 0xf00000
#define KEY_MASK 0x700000 //disable KEY1

enum KEY_BIT_SHIFT_t
{
KEY4_BIT_SHIFT = 0,
KEY3_BIT_SHIFT,
KEY2_BIT_SHIFT,
KEY1_BIT_SHIFT
};

enum EVENT_CMD_t
{
EVENT_IDLE = 0,
KEY4_UP,
KEY4_DOWN,
KEY3_UP,
KEY3_DOWN,
KEY2_UP,
KEY2_DOWN,
KEY1_UP,
KEY1_DOWN,
PIECE_UP,
PIECE_DOWN
};

enum ID_CMD_t
{
ID_CMD_DEFAULT_HANDLER = 0,
ID_CMD_ADJ_ADJUST,
ID_CMD_LINE_SELECT,
ID_CMD_COLOR_SELECT,
ID_CMD_SHOW_LED,
ID_CMD_SEND_BOARD_ARRAY,
ID_CMD_SEND_BOARD_ID,
ID_CMD_RECEIVE_BOARD_ARRAY,
ID_CMD_RECEIVE_BOARD_COLOR,
ID_CMD_PLAY_SOUND,
ID_CMD_DEBUG_CTRL,
ID_CMD_SHOW_STRIP
};

static int do_exit = 0;

static unsigned int board_status[DEFAULT_BOARD_SIZE];
unsigned int board_display_send[DEFAULT_BOARD_SIZE * 2];
unsigned int board_display_buffer[DEFAULT_BOARD_SIZE * 2];
unsigned int board_sample[BOARD_SAMPLE][DEFAULT_BOARD_SIZE];

static int mute_state = 0;
  
#ifdef DEBUG
#if 0
const unsigned int TestData[DEFAULT_BOARD_SIZE * 2] = {
  0, 0x20, 0x4e, 0x7e, 0xfc, 0x7c, 0x3c, 0x10, 0,
  0, 0x20, 0x0e, 0xa, 0x70, 0x38, 0x18, 0, 0
};
#else
const unsigned int TestData[DEFAULT_BOARD_SIZE * 2] = {
  0, 0x20, 0x4e, 0x7e, 0xfc, 0x7c, 0x3c, 0x10, 0, 0,0,0,0,0,0,0,0,0,0,
  0, 0x20, 0x0e, 0xa, 0x70, 0x38, 0x18, 0, 0, 0,0,0,0,0,0,0,0,0,0
};
#endif
#endif

void SetStatusLED(int index, unsigned int color);

#define reverse_byte4(x) ((0xff000000U & x) >> 24|(0x00ff0000U & x) >>  8 |(0x0000ff00U & x) <<  8 |(0x000000ffU & x) << 24)

unsigned char reverse8( unsigned char c )
{
c = ( c & 0x55 ) << 1 | ( c & 0xAA ) >> 1;
c = ( c & 0x33 ) << 2 | ( c & 0xCC ) >> 2;
c = ( c & 0x0F ) << 4 | ( c & 0xF0 ) >> 4;
return c;
}

// 返回自系统开机以来的毫秒数（tick）
static unsigned int millis()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (unsigned int)((long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* ctrl-c signal handler */
void sigintproc(int signum)
{
  signum = signum;
  do_exit = 1;
}

/* TERM signal handler */
void sigtermproc(int signum)
{
  signum = signum;
  do_exit = 1;
}

int set_interface_attribs (int fd, int speed, int parity) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0) {
    perror ("error from tcgetattr");
    return -1;
  }

  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);

  tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
  tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
  tty.c_cflag |= CS8; // 8 bits per byte (most common)
  tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
  tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO; // Disable echo
  tty.c_lflag &= ~ECHOE; // Disable erasure
  tty.c_lflag &= ~ECHONL; // Disable new-line echo
  tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
  // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
  // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

  tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
  tty.c_cc[VMIN] = 0;

  if (tcsetattr (fd, TCSANOW, &tty) != 0) {
    perror ("error from tcsetattr");
    return -1;
  }
  return 0;
}

void set_blocking (int fd, int should_block) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    perror ("error from tggetattr");
    return;
  }

  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    perror ("error setting term attributes");
}

char number_to_letterbar(unsigned char n)
{
  //(A B C D E F G H J K L M N O P Q R S T)
  if ((n >= 0) && (n <= 7))
    return 'A' + n;

  if ((n >= 8) && (n <= 18))
    return 'J' + n - 8;

  return 0;
}

int letterbar_to_number(char c)
{
  int n;

  //(A B C D E F G H J K L M N O P Q R S T)
  if ((c >= 'A') && (c <= 'H'))
    return c - 'A';

  if ((c >= 'J') && (c <= 'T'))
    return c - 'J' + 8;

  return -1;
}

void dump_data(unsigned char *buf,int n)
{
   int i;
   printf("{");
   for(i=0;i<n;i++){
     if(i%16==0) printf("\n");
     printf("0x%02x ",*(buf+i));
    };
    printf("}\n");
    return;
}

int get_id(int *id0, int *id1, int *id2)
{
  int i, j, n;
  int id_receive[3];
  char *p;

   TransferSendCommand(ID_CMD_SEND_BOARD_ID, "X\n", 2);

   if(TransferAvailable() && (TransferGetPacketID() == ID_CMD_SEND_BOARD_ID))
   {
   n = TransferGetObj((char*)&id_receive[0], sizeof(id_receive));
//   printf("obj len=%d\n", n);
   *id0 = id_receive[0];
   *id1 = id_receive[1];
   *id2 = id_receive[2];
//    printf("ID0[0x%08x],ID1[0x%08x],ID2[0x%08x]\n",*id0,*id1,*id2);
   return 0;
   }

   return -1;
}

int get_version(void)
{
  int n;	
  char version_receive[64];

   TransferSendCommand(ID_CMD_DEFAULT_HANDLER, "X\n", 2);

   if(TransferAvailable() && (TransferGetPacketID() == ID_CMD_DEFAULT_HANDLER))
   {
   n = TransferGetObj(version_receive, sizeof(version_receive));
   //printf("obj len=%d\n", n);
   printf("%s\n",version_receive);
   return 0;
   }
   
   return -1;
}

void SampleToResult(unsigned int *pBoard)
{
  int i, j, k;
  int count;
  uint32_t index;
  uint32_t result;

  for (i = 0; i < DEFAULT_BOARD_SIZE; i++)
  {
    index = 1;
    result = 0;
    for (j = 0; j < DEFAULT_BOARD_SIZE; j++)
    {
      count = 0;
      for (k = 0; k < BOARD_SAMPLE; k++) {
        count += (board_sample[k][i] & index) >> j;
      }
      if ( count >= BOARD_SAMPLE_THRESHOLD ) result |= index;
      index = index << 1;
    }
	*(pBoard + i) = result;
  }
}

int ScanBoard(unsigned int *pBoard, int key_down)
{
  int i, n;
  char *c;  
  unsigned int board_status_receive[DEFAULT_BOARD_SIZE];
  int test = 1;
  int bigendian = !*(char *)(&test);
  unsigned int key_state_begin = 0;
  unsigned int key_state_end = 0;
  unsigned int key_state, index;
  
  TransferSendCommand(ID_CMD_SEND_BOARD_ARRAY, "A\n", 2);

  if(TransferAvailable() && (TransferGetPacketID() == ID_CMD_SEND_BOARD_ARRAY))
  {
    n = TransferGetObj((char*)&board_status_receive[0], sizeof(board_status_receive));
    if( n != sizeof(board_status_receive) ) {
    printf("TransferGetObj length error!\n");
    usleep(100 * 1000);
    return 0;
    }
  }
  else
  {
    usleep(100 * 1000);
    return 0;
  }

    /*
      printf("receive board status:\n");
      for ( i = 0; i < DEFAULT_BOARD_SIZE; i++ ) {
      c = (char*)&board_status_receive[i];
      printf("%c",*c++);
      printf("%c",*c++);
      printf("%c",*c++);
      printf("%c ",*c++);
      }
      printf("\n");
    */
	
    for ( i = 0; i < DEFAULT_BOARD_SIZE; i++ ) {
      Base64_decode((char*)&pBoard[i], (char*)&board_status_receive[i]);
      if ( bigendian ) {
        pBoard[i] = reverse_byte4(pBoard[i]);
      }
      //printf("[%d]0x%08x\n", i, pBoard[i]);
	  if( i == 0 ) {
	  key_state_begin = (pBoard[i]&KEY_MASK)>>20;
	  }
	  if( i == DEFAULT_BOARD_SIZE - 1 ) {
	  key_state_end = (pBoard[i]&KEY_MASK)>>20;		  
	  }
      pBoard[i] = pBoard[i] & BOARD_MASK;
    }
	
	key_state = key_state_begin ^ key_state_end;
	if( key_state ) {
#ifdef DEBUG		
      printf("keystate changed in scanboard\n");
#endif	  
      return 0;	  
	}
	
	index = 1;
	for( i = 0; i < 4; i++ ) {
#ifdef KEY_ACTIVE_HIGH
		  if( key_state_end & index ) {
			  return i*2+2;
		  }
		  if( (key_down & index) && (~key_state_end & index) ) {
			  return i*2+1;
		  }
#else
                  if( ~key_state_end & index ) {
                          return i*2+2;
                  }
                  if( (key_down & index) && (key_state_end & index) ) {
                          return i*2+1;
                  }
#endif
        index = index << 1;		
	}
	return 0;
}

void UpdateInfoLED(unsigned int *pBoard)
{
  int i, j;
  int line, index = 1;  
  int set_remove_flag = 0;
  
    for ( i = 0; i < DEFAULT_BOARD_SIZE; i++ ) {
      index = 1;
      line = pBoard[i];
      for (j = 0; j < DEFAULT_BOARD_SIZE; j++ ) {
          if(line & index) {
            if( BOARD(i, j) == EMPTY )
            set_remove_flag = 1;
          } 
          index = index << 1;
      }
    }

    if( set_remove_flag )
    SetStatusLED(LED_INFO, COLOR_PURPLE_LOW);
    else
    SetStatusLED(LED_INFO, 0);	
}

int get_move(int *row, int *col, int key_down)
{
  int i, j;
  unsigned int board_status_new[DEFAULT_BOARD_SIZE];
  int line, index = 1;
  int status = 0;
  int key;
  
  memset(board_status_new, 0, sizeof(board_status_new));
  while (!do_exit)
  {
/*
    for (i = 0; i < BOARD_SAMPLE; i++) {  
      ScanBoard((unsigned int*)&board_sample[i][0]);
    }
    SampleToResult(board_status_new);
*/
    key = ScanBoard(board_status_new, key_down);
	if( key ) return key;
	
    UpdateInfoLED(board_status_new);

    index = 1;
    for ( i = 0; i < DEFAULT_BOARD_SIZE; i++ ) {
      if ( board_status_new[i] != board_status[i] ) {
        //printf("0x%08x\n", board_status_new[i]);
        line = board_status_new[i] ^ board_status[i];
        for (j = 0; j < DEFAULT_BOARD_SIZE; j++) {
          if (line & index) {
            if( board_status_new[i] & index ) {
            status = PIECE_DOWN; //down
            } else {
            status = PIECE_UP; //up
            }
            *col = j;
            break;
          }
          index = index << 1;
        }
        *row = i;
        memcpy(board_status, board_status_new, sizeof(board_status));
   
        return status;
      }
    }
    usleep(50 * 1000);
  }

  printf("Break by ctrl+c\n");
  return -1;
}

void set_move(int row, int col)
{
  char x, y;
  char buf[64];
  int size;

  x = number_to_letterbar(row);
  y = number_to_letterbar(col);

  size = snprintf(buf, sizeof(buf), "G%c%c\n", x, y);

  TransferSendCommand(ID_CMD_SHOW_LED, buf, size);
}

int send_rawboard(char *data)
{
  TransferSendCommand(ID_CMD_RECEIVE_BOARD_ARRAY, data, sizeof(board_display_send)/2);
  TransferSendCommand(ID_CMD_RECEIVE_BOARD_COLOR, data + DEFAULT_BOARD_SIZE * 4, sizeof(board_display_send)/2);
  TransferSendCommand(ID_CMD_SHOW_LED, "F\n", 2);
}

int send_showboard(unsigned int *pBuffer)
{
  int i;
  int test = 1;
  int bigendian = !*(char *)(&test);
  unsigned int line;

  for ( i = 0; i < DEFAULT_BOARD_SIZE * 2; i++ ) {
     line = *(pBuffer+i);
     if ( bigendian ) {
        line = reverse_byte4(line);
      }
     Base64_encode((char*)&board_display_send[i], (char*)&line);
  }

  TransferSendCommand(ID_CMD_RECEIVE_BOARD_ARRAY, (char*)&board_display_send[0], sizeof(board_display_send)/2);
  TransferSendCommand(ID_CMD_RECEIVE_BOARD_COLOR, (char*)&board_display_send[DEFAULT_BOARD_SIZE], sizeof(board_display_send)/2);
  TransferSendCommand(ID_CMD_SHOW_LED, "F\n", 2);

  return 0;
}

int update_board(char *pBuffer)
{
  unsigned int board_status_update[DEFAULT_BOARD_SIZE * 2];
  int i,j;
  int index,line,color;
  int test = 1;
  int bigendian = !*(char *)(&test);

  for (i = 0; i < DEFAULT_BOARD_SIZE * 2; i++ ) {
  Base64_decode((char*)&board_status_update[i], pBuffer + i * 4);
  if ( bigendian ) {
     board_status_update[i] = reverse_byte4(board_status_update[i]);
  }
  board_status_update[i] = board_status_update[i] & BOARD_MASK;
  }
  
  for (i = 0; i < DEFAULT_BOARD_SIZE; i++ ) {
    index = 1;
    line = board_status_update[i];
    color = board_status_update[DEFAULT_BOARD_SIZE + i];
    for (j = 0; j < DEFAULT_BOARD_SIZE; j++ ) {
    if( line & index ) {
       if( color & index ) {
         BOARD(i, j) = WHITE;
       } else {
         BOARD(i, j) = BLACK;
       }
    } else {
         BOARD(i, j) = EMPTY;
    }
    index = index << 1;
    }
  }
}

void StringToBuffer(char *str, unsigned int *buffer, int offset, int color)
{
  unsigned char data1, data2, data3;
  int i,x;
  int col;

  col = offset%8;
  x = offset/8;

  for(i=0; i<16; i++) {
   data1 = reverse8(acFont8x16[*(str+x) - ' '][i]);
   data2 = reverse8(acFont8x16[*(str+x+1) - ' '][i]);
   data3 = reverse8(acFont8x16[*(str+x+2) - ' '][i]);

   buffer[i] = (data1>>col)|(data2<<(8-col))|(data3<<(16-col));
  }

  if(color) {
  memcpy((char*)buffer + DEFAULT_BOARD_SIZE * 4, (char*)buffer, DEFAULT_BOARD_SIZE * 4);
  }
}

void DisplayString(char *str, int color)
{
   int n = strlen(str);
   
   if(n >= 2) { 
     for(int i=0; i<8*(n-2); i++) {
     StringToBuffer(str, board_display_buffer,i, color);
     send_showboard(board_display_buffer);
     usleep(100 * 1000);
     if(do_exit) break;
     }
   }
}

void SetStatusLED(int index, unsigned int color)
{
   if( index >= NUM_LEDS ) return;

   WS2812B_setPixelColor(index, color);
   TransferSendCommand(ID_CMD_SHOW_STRIP, pixels, sizeof(pixels));
   usleep(1000); //On mips platform
}

void SetRefAdj(int adj)
{
   char buf[2];
  
   if( (adj >=0) && (adj <= 9) ) {
   buf[0]= '0' + adj;
   buf[1]= '\n';
   TransferSendCommand(ID_CMD_ADJ_ADJUST, buf, 2);
   }
}

void CalibriateRef(void)
{
  int i,adj;
  unsigned int board_status_new[DEFAULT_BOARD_SIZE];

  int test = 1;
  int bigendian = !*(char *)(&test);
  int status;
  int min = -1, max = -1;
  int mean;
  char *c;

   for(adj=0; adj<=9; adj++) {
   SetRefAdj(adj);
   ScanBoard(board_status_new, 0);

    status = 0;
    for ( i = 0; i < DEFAULT_BOARD_SIZE; i++ ) {
      if(board_status_new[i] != 0 ) {
	     status = 1;
	     break;
	  }
    }

    if( status != 0 ) {
	break;
    }

    if( min == -1 ) {
      min = adj;
    }

    if( adj > max ) {
      max = adj;
    }
#ifdef DEBUG
    printf("adj=%d ok\n",adj);
#endif
    usleep(100 * 1000);
  }

  mean = (min + max)/2;
  printf("adj min=%d,max=%d,mean=%d\n",min,max,mean);

  if( max > 1 ) {
  adj = max;//max -1 ;
  SetRefAdj(adj);
  printf("Set Ref=%d\n",adj);
  }
}

void board_init(int ref_init)
{
   TransferSendCommand(ID_CMD_DEBUG_CTRL, "D\n", 2);

  if(ref_init) {
   CalibriateRef();
  }

#ifdef DEBUG
   CalibriateRef();
  //while(!do_exit)
  { 
   TransferSendCommand(ID_CMD_SHOW_LED, "r\n", 2);
   sleep(1);
   TransferSendCommand(ID_CMD_SHOW_LED, "g\n", 2);
   sleep(1);
  }
#endif

   TransferSendCommand(ID_CMD_SHOW_LED, "X\n", 2);

#ifdef DEBUG
  send_showboard((unsigned int*)TestData);
  sleep(1);
  //while(!do_exit)
  {
   DisplayString("READY", 1);
  }
#else
   send_showboard(board_display_buffer);
#endif
   TransferSendCommand(ID_CMD_PLAY_SOUND, "B\n", 2);

   WS2812B_clear();
   TransferSendCommand(ID_CMD_SHOW_STRIP, pixels, sizeof(pixels));

  //while(!do_exit)
  {
   SetStatusLED(LED_INFO, COLOR_PURPLE_LOW);
   sleep(1);
   SetStatusLED(LED_INFO, COLOR_GREEN_LOW);
   sleep(1);
  }

}

int set_showboard(int x, int y, int color)
{
 board_display_buffer[x] |= 1<<y;
 if( color == WHITE )
 board_display_buffer[DEFAULT_BOARD_SIZE + x] |= 1<<y;
}

int clear_showboard(int x, int y, int color)
{
 board_display_buffer[x] &= ~(1<<y);
 if( color == WHITE )
 board_display_buffer[DEFAULT_BOARD_SIZE + x] &= ~(1<<y);
}

int PlaySound(unsigned char *data, int len)
{
#ifdef ALSA
        if(*data == 'B') {
        return TransferSendCommand(ID_CMD_PLAY_SOUND,data,len);
        }


	if(!mute_state) {
        if(*data == 'C') {
        return TransferSendCommand(ID_CMD_PLAY_SOUND,data,len);
        }

	return play_wav(HandleSound,data,len);
	}
#else
	if( (!mute_state) || (*data == 'B') ) {
	TransferSendCommand(ID_CMD_PLAY_SOUND,data,len);
	}
#endif
}
