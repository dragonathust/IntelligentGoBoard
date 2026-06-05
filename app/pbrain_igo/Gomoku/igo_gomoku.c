#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>

#include "SerialTransfer/Wrapper.h"
#include "sound.h"
#include "ws2812b.h"
#include "base64.h"
#include "board.h"
#include "english_16_8.h"
#include "igo.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

//从左到右相同棋子数
int LeftRight(int i, int j, int side)
{

  int tempi, count;
  tempi = i;
  count = 1;
  //toleft
  while ( --tempi > 0 && BOARD(tempi, j) == side)
  {
    count ++;
  }
  tempi = i;
  while ( ++ tempi < 20 && BOARD(tempi, j) == side )
  {
    count ++;
  }
  return count;
}

//从上到下相同棋子数
int UpDown(int i, int j, int side)
{
  int tempj, count;
  tempj = j;
  count = 1;

  while ( --tempj > 0 && BOARD(i, tempj) == side)
  {
    count ++;
  }
  tempj = j;
  while ( ++ tempj < 20 && BOARD(i, tempj) == side )
  {
    count ++;
  }
  return count;
}

//从左上到右下相同的棋子数
int LupToRdown(int i, int j, int side)
{
  int tempi, tempj, count;
  tempi = i, tempj = j;
  count = 1;

  while ( --tempi > 0 && -- tempj > 0 && BOARD(tempi, tempj) == side)
  {
    count ++;
  }
  tempi = i, tempj = j;
  while ( ++ tempi < 20 && ++ tempj < 20 && BOARD(tempi, tempj) == side )
  {
    count ++;
  }
  return count;
}

//从右上到左下的相同棋子数
int RuptoLdown(int i, int j, int side)
{
  int tempi, tempj, count;
  tempi = i, tempj = j;
  count = 1;

  while ( --tempi > 0 && ++tempj < 20 && BOARD(tempi, tempj) == side)
  {
    count ++;
  }
  tempi = i, tempj = j;
  while ( ++ tempi < 20 && -- tempj > 0 && BOARD(tempi, tempj) == side )
  {
    count ++;
  }
  return count;
}

int IsSuccess(int i, int j, int side)
{
  if ( LeftRight(i, j, side) >= 5 || UpDown(i, j, side) >= 5
       || LupToRdown(i, j, side) >= 5 || RuptoLdown(i, j, side) >= 5)
  {
    return 1;
  }

  return 0;
}

int chess_main(int output_fd, int input_fd, int player_color, char *name)
{
  int length;
  int step = 0;	
  int to_move, is_win;
  int x, y;
  int x_last_player = 0, y_last_player = 0;
  int x_last_computer = 0, y_last_computer = 0;
  int x_last2_computer = 0, y_last2_computer = 0;
  char cmd[256];
  char result[8196];
  int key_down_state = 0;
  unsigned int key_down_time[4];
  int undo = 0;
  int player_color_next = player_color;
  int play_go_next = 0;
  int timeout_turn = 1000;

  memset(cmd, 0, sizeof(cmd));
#ifdef DEBUG  
  sprintf(cmd, "ABOUT\n");
  write(output_fd, cmd, strlen(cmd));
  length = read(input_fd, result, sizeof(result));
  result[length] = '\0';
  fprintf(stderr, "read:%s\n", result);
#endif

  sprintf(cmd,"INFO timeout_turn %d\n",timeout_turn);
  write(output_fd, cmd, strlen(cmd));

  sprintf(cmd, "START %d\n", DEFAULT_BOARD_SIZE);
  write(output_fd, cmd, strlen(cmd));
  length = read(input_fd, result, sizeof(result));
  result[length] = '\0';
#ifdef DEBUG
  fprintf(stderr, "read:%s\n", result);
#endif

  memset(board_display_buffer, 0, sizeof(board_display_buffer));
  
  while (!do_exit) {
    step++;

    if( (step != 1) || (player_color == 0)) {
#ifdef DEBUG		
    ascii_showboard();
#endif	
    send_showboard(board_display_buffer);;
    to_move = (player_color == 0)?BLACK:WHITE;
#ifdef DEBUG
    printf("%s(%d): \n", to_move == WHITE ? "WHITE" : "BLACK", step);
#endif

    if(player_color) {
    SetStatusLED(LED_TURN, COLOR_GREEN_LOW);
	} else {
    SetStatusLED(LED_TURN, COLOR_RED_LOW);
	}

    /* wait input from user */
	switch( get_move(&x, &y, key_down_state) )
	{
	  case PIECE_DOWN:
      if ( BOARD(x, y) ) {
       //PlaySound(BeepCode, sizeof(BeepCode));
       fprintf(stderr, "Illegal move- %d,%d\n", x, y);
        continue;
      }
	  //PlaySound(DownCode, sizeof(DownCode));	  
	  PlaySound(ClickCode, sizeof(ClickCode));	  
	  break;

	  case PIECE_UP:
	  PlaySound(UpCode, sizeof(UpCode));
      printf("remove %d,%d\n",x,y);
      continue;	  
	  break;

	  case KEY2_DOWN:
      printf("KEY2_DOWN\n");
	  if( !(key_down_state & (1<<KEY2_BIT_SHIFT)) ) {
		  key_down_state |= 1<<KEY2_BIT_SHIFT;
		  key_down_time[KEY2_BIT_SHIFT] = millis();//start the timer
	  } else {
		  if( (millis() - key_down_time[KEY2_BIT_SHIFT]) > TIMEOUT_KEY_PRESS ) {

			  continue;
		  }
	  }
      continue;
	  break;

	  case KEY2_UP:
            printf("KEY2_UP\n");
            key_down_state &= ~(1<<KEY2_BIT_SHIFT);
            mute_state = !mute_state;
            SetStatusLED(LED_MUTE, mute_state?COLOR_YELLOW:0);
			PlaySound("B\n", 2);
      continue;
	  break;
	  
	  case KEY3_DOWN:
      printf("KEY3_DOWN\n");
	  if( !(key_down_state & (1<<KEY3_BIT_SHIFT)) ) {
		  key_down_state |= 1<<KEY3_BIT_SHIFT;
		  key_down_time[KEY3_BIT_SHIFT] = millis();//start the timer
	  } else {
		  if( (millis() - key_down_time[KEY3_BIT_SHIFT]) > TIMEOUT_KEY_PRESS ) {
			  play_go_next = 1;
			  do_exit = 1;
			  PlaySound(melodyCode, sizeof(melodyCode));
			  printf("Exit program\n");
			  continue;
		  }
	  }
      continue;
	  break;
	  
	  case KEY3_UP:
            printf("KEY3_UP\n");
            key_down_state &= ~(1<<KEY3_BIT_SHIFT);
            player_color_next = !player_color_next;
            SetStatusLED(LED_GOMOKU, (player_color_next == 0)?COLOR_RED_LOW:COLOR_GREEN_LOW);
			PlaySound("B\n", 2);
      continue;
	  break;
	  
	  case KEY4_DOWN:
      printf("KEY4_DOWN\n");
	  if( !(key_down_state & (1<<KEY4_BIT_SHIFT)) ) {
		  key_down_state |= 1<<KEY4_BIT_SHIFT;
		  key_down_time[KEY4_BIT_SHIFT] = millis();//start the timer
	  } else {
		  if( (millis() - key_down_time[KEY4_BIT_SHIFT]) > TIMEOUT_KEY_PRESS ) {
			  do_exit = 1;
			  PlaySound(melodyCode, sizeof(melodyCode));
			  printf("Exit program\n");
			  continue;
		  }
	  }
      continue;
	  break;

	  case KEY4_UP:
		printf("KEY4_UP\n");
		key_down_state &= ~(1<<KEY4_BIT_SHIFT);
		if( !undo ) {
		PlaySound(UndoCode, sizeof(UndoCode));
		sprintf(cmd, "TAKEBACK %d,%d\n", x_last_computer, y_last_computer);
		fprintf(stderr, "write:%s", cmd);
		write(output_fd, cmd, strlen(cmd));
		BOARD(x_last_computer, y_last_computer) = 0;
		clear_showboard(x_last_computer, y_last_computer, (player_color == 0)?WHITE:BLACK);
		
		sprintf(cmd, "TAKEBACK %d,%d\n", x_last_player, y_last_player);
		fprintf(stderr, "write:%s", cmd);
		write(output_fd, cmd, strlen(cmd));
		BOARD(x_last_player, y_last_player) = 0;
		clear_showboard(x_last_player, y_last_player, (player_color == 0)?BLACK:WHITE);
		
		x_last_computer = x_last2_computer; y_last_computer = y_last2_computer;
		move_history_pos[0] = POS(x_last2_computer, y_last2_computer);
		set_move(x_last2_computer, y_last2_computer);
		undo = 1;
		} else {
		  PlaySound(NopCode, sizeof(NopCode));			
		}
        continue;
	  break;

      default:
      continue;		  
	  break;      	  
	}
	
    sprintf(cmd, "TURN %d,%d\n", x, y);
    fprintf(stderr, "write:%s", cmd);
	set_move(x, y);
	x_last_player = x; y_last_player = y;
    move_history_pos[0] = POS(x, y);
    BOARD(x, y) = to_move;
    set_showboard(x, y, to_move);
    if ( is_win = IsSuccess(x, y, to_move) ) {
      break;
    }
    write(output_fd, cmd, strlen(cmd));
#ifdef DEBUG	
    ascii_showboard();
#endif
    send_showboard(board_display_buffer);
	} else {
    sprintf(cmd, "BEGIN\n");
    fprintf(stderr, "write:%s", cmd);	
    write(output_fd, cmd, strlen(cmd));
	}
	
	SetStatusLED(LED_INFO, 0);
#ifdef DEBUG 	
    printf("%s is thinking...\n", name);
#endif

    if(player_color) {
    SetStatusLED(LED_TURN, COLOR_RED_LOW);
	} else {
    SetStatusLED(LED_TURN, COLOR_GREEN_LOW);
	}	
		
    while (!do_exit) {
      length = read(input_fd, result, sizeof(result));
      result[length] = '\0';
      fprintf(stderr, "read:%s\n", result);
      if ( parse_result(result, &x, &y) ) {
        sprintf(cmd, "TURN %d,%d\n", x, y);
        break;
      } else {
        continue;
      }
    }

    set_move(x, y);
	x_last2_computer = x_last_computer; y_last2_computer = y_last_computer;	
	x_last_computer = x; y_last_computer = y;	
    to_move = (player_color == 0)?WHITE:BLACK;
    move_history_pos[0] = POS(x, y);
    BOARD(x, y) = to_move;
    set_showboard(x, y, to_move);
	PlaySound(MoveCode, sizeof(MoveCode));
	undo = 0;
	
    if ( is_win = IsSuccess(x, y, to_move) ) {
      break;
    }
  }

#ifdef DEBUG
  ascii_showboard();
#endif  
  send_showboard(board_display_buffer);;
  if (is_win) printf("%s WIN!\n", to_move == WHITE ? "WHITE" : "BLACK");

  sprintf(cmd, "END\n");
  write(output_fd, cmd, strlen(cmd));
  sleep(1);
  
  if(!do_exit) {
  if( to_move == ((player_color == 0)?BLACK:WHITE) ) {
#ifndef ALSA
    PlaySound(musicCodeWin, sizeof(musicCodeWin));
    sleep(15);
#endif
    return 3; //win
  } else {
#ifndef ALSA
    PlaySound(musicCodeLose, sizeof(musicCodeLose));
    sleep(10);
#endif
    return 4; //lose
  }
    //wait music finish else MCU will be blocked
  }
  
  if( play_go_next ) {
	  return 2;//restart next game
  }
  
  if( player_color_next != player_color ) {
	  return 1;//restart same game with different player color
  }
  
  return 0;//restart same game  
}

void usage__r(void)
{
  printf("\nIGO for gomoku.\n");
  printf("\n");
  printf("Usage: igo_gomoku [OPTIONS]\n");
  printf("\n");
  printf("Options:\n");
  printf("  -b pbrain\n");
  printf("  -c color, 0 black(default), 1 white\n");  
  printf("  -p serial device\n");
  printf("  -s boardsize, default MAX_BOARD\n");
  printf("  -h Show this help\n");
  printf("\n");
}

int main(int argc, char *argv[])
{
  int opt;
  char *end;
  int config = 0;
  int pid;
  int error;
  int status;
  int boardsize = MAX_BOARD;
  int player_color = 0;
  int ref_init = 1;
  char pbrain[UNIX_PATH_MAX];
  char serial_device[UNIX_PATH_MAX];
  int serial_fd;
  int id0 = 0, id1 = 0, id2 = 0;
  int ret;

  int pipe_stdin[2], pipe_stdout[2];

  printf("Copyright YiXiaolong, all rights reserved.\n");
  printf("Build time %s %s\n", __DATE__,__TIME__);

  while ((opt = getopt(argc, argv, "b:c:r:p:s:Hh")) != -1)
  {
    switch (opt)
    {
      case 'b':
        strncpy(pbrain, argv[ optind - 1 ], sizeof(pbrain) - 1);
        config++;
        break;

      case 'c':
        player_color = strtol(argv[ optind - 1 ], &end, 0);
        break;

      case 'r':
        ref_init = strtol(argv[ optind - 1 ], &end, 0);
        break;

      case 'p':
        strncpy(serial_device, argv[ optind - 1 ], sizeof(serial_device) - 1);
        config++;
        break;
		
      case 's':
        boardsize = strtol(argv[ optind - 1 ], &end, 0);
        break;

      case 'H':
      case 'h':
        usage__r();
        return 0;

      default:
        usage__r();
        return -1;
    }
  }

  if (config < 2)
  {
    usage__r();
    return -1;
  }

  printf("boardsize is %d\n", boardsize);
  printf("player_color is %s\n", (player_color == 0)?"black":"white");
  
  serial_fd = open(serial_device, O_RDWR | O_NOCTTY | O_SYNC);
  if ( serial_fd < 0 )
  {
    printf("open %s failed!\n", serial_device);
    return -1;
  }

  //set_interface_attribs (serial_fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
  set_interface_attribs (serial_fd, B921600, 0);  // set speed to 115,200 bps, 8n1 (no parity)
  set_blocking (serial_fd, 0);                // set no blocking
  //set_blocking (serial_fd, 1);                // set blocking
  //fcntl(serial_fd, F_SETFL, FNDELAY);
  //fcntl(serial_fd, F_SETFL, 0);

  memset(board_status, 0, sizeof(board_status));
  memset(board_display_send, 0, sizeof(board_display_send));
  memset(board_display_buffer, 0, sizeof(board_display_buffer));
  memset(pixels, 0, sizeof(pixels));

  /* setup signal handlers */
  signal(SIGINT, sigintproc);
  signal(SIGTERM, sigtermproc);

  if ( (pipe(pipe_stdin) < 0) || (pipe(pipe_stdout) < 0) ) {
    fprintf(stderr, "%s", "PIPE BUILD ERROR\n");
    exit(-1);
  }

  pid = fork();

  if (!pid)
  {
    printf("start app %s\n", pbrain);
    close(pipe_stdin[1]);
    close(pipe_stdout[0]);
    if ((dup2(pipe_stdin[0], /*fileno(stdin)*/STDIN_FILENO) < 0) || (dup2(pipe_stdout[1], /*fileno(stdout)*/STDOUT_FILENO) < 0)) {
      fprintf(stderr, "%s", "REDRIECT ERROR\n");
      exit(-1);
    }

    execl( pbrain, pbrain, NULL );
    printf("App '%s' , errno:%d\n", pbrain, errno);
    return 0;
  }

  close(pipe_stdin[0]);
  close(pipe_stdout[1]);

#ifdef ALSA
  HandleSound = sound_open();
#endif
  TransferBegin(serial_fd);
  board_init(ref_init);
  get_id(&id0, &id1, &id2);
  printf("ID0[0x%08x],ID1[0x%08x],ID2[0x%08x]\n",id0,id1,id2);
  get_version();
  
  SetStatusLED(LED_GOMOKU, (player_color == 0)?COLOR_RED_LOW:COLOR_GREEN_LOW);
  ret = chess_main(pipe_stdin[1], pipe_stdout[0], player_color, pbrain); 

  error = waitpid( pid,
                   &status,
                   0 /*options*/);
  printf("child '%s' status %d, ret %d, errno:%d\n", pbrain, status,
         error, errno);

  close(serial_fd);
#ifdef ALSA
  sound_close(HandleSound);
#endif

  return ret;
}
