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

static int parse_result(char *result, int *col, int *row)
{
  int v, v1 = 0, v2= 0;

  printf("parse [%s]\n", result);

  *col = letterbar_to_number(result[0]);
  if( isdigit(result[1]) )
    v1 = result[1]-'0';

  if( isdigit(result[2]) )
    v2 = (v1 * 10 ) + result[2]-'0';

  if(v2)
  v = v2;
  else
  v = v1;

  *row = DEFAULT_BOARD_SIZE - v;

  if( (*col != -1)&&(*row != -1))
  return 0;
  else
  return -1;
}

/* eg:=4 C3 */
enum fsm
{
find_start_byte,
//find_id_byte,
find_space_byte,
find_payload,
find_end_byte,
};

int state = find_start_byte;
int payIndex = 0;

static int parse_response(char c, char *response, int response_buf_size)
{

  //printf("state=%d, c=0x%02x\n",state, c);

  switch (state)
  {
    case find_start_byte:
         if((c == '=')||(c == '?')) {
              //state = find_id_byte;
              state = find_space_byte;
         }
    break;
/*
    case find_id_byte:
              state = find_space_byte;
    break;
*/
    case find_space_byte:
         if(c == ' ') {
         state = find_payload;
         payIndex = 0;
         }

         if( c == '\n' ) {
             state = find_start_byte;
             response[0]='\0';
             return 1;
         }
    break;

    case find_payload:
         if( c == '\n' ) {
             state = find_end_byte;
         }
         else
         { 
           response[payIndex] = c;
           payIndex++;
         }

    break;

    case find_end_byte:
         if( c == '\n' ) {
             state = find_start_byte;
             response[payIndex]='\0';
             payIndex++;
             return payIndex;
         }
    break;

    default:
             state  = find_start_byte;
    break;

  }
    return 0;
}

int send_command(int output_fd, char *command, int input_fd, char *response, int response_buf_size)
{
  int r;
  char in_char;

#ifdef DEBUG
  fprintf(stderr, "->%s", command);
#endif

  r = write(output_fd, command, strlen(command));
  if (r < 0) {
    printf("write failed with %i: %s\n", errno, strerror(errno));
    return -1;
  }


  do {
  r = read(input_fd, &in_char, 1);
  if (r < 0) {
    printf("read failed with %i: %s\n", errno, strerror(errno));
    return -1;
  }

   if( parse_response(in_char, response, response_buf_size) )
	break;
  } while(!do_exit);

  return -1;
}

int chess_main(int output_fd, int input_fd, int player_color, char *name)
{
  int step = 0;
  int x, y;
  char cmd[128];
  char result[512];
  int undo = 0;
  int color_win;
  int x_last_computer = 0, y_last_computer = 0;
  int x_last2_computer = 0, y_last2_computer = 0;
  int key_down_state = 0;
  unsigned int key_down_time[4];
  int player_color_next = player_color;
  int play_gomoku_next = 0;
  
  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "boardsize %d\n", DEFAULT_BOARD_SIZE);
  send_command(output_fd, cmd, input_fd, result, sizeof(result));

  send_command(output_fd, "clear_board\n", input_fd, result, sizeof(result));

  while (!do_exit) {
    step++;

    if( (step != 1) || (player_color == 0)) {
    send_command(output_fd, "showboard_raw\n", input_fd, result, sizeof(result));
    send_rawboard(result);
    update_board(result);
#ifdef DEBUG
    ascii_showboard();
#endif

#ifdef DEBUG
    if(player_color){
    printf("WHITE(%d): ", step);
	} else {
    printf("BLACK(%d): ", step);
	}
#endif

/*
    send_command(output_fd, "final_score\n", input_fd, result, sizeof(result));
    printf("score:%s\n",result);
*/
    if(player_color) {
    SetStatusLED(LED_TURN, COLOR_GREEN_LOW);
	} else {
    SetStatusLED(LED_TURN, COLOR_RED_LOW);
	}
    /* wait input from user */
	switch( get_move(&x, &y, key_down_state) )	
	{
	  case PIECE_DOWN:
      if( BOARD(x, y) != EMPTY ) {
      printf("put %d,%d\n",x,y);
      continue;
      }

      printf("set move %d,%d\n",x,y);
    if(player_color) {	  
      sprintf(cmd, "play white %c%d\n", number_to_letterbar(y), DEFAULT_BOARD_SIZE - x);
	} else {
      sprintf(cmd, "play black %c%d\n", number_to_letterbar(y), DEFAULT_BOARD_SIZE - x);
	}
      send_command(output_fd, cmd, input_fd, result, sizeof(result));

      if ( strcasecmp(result,"illegal move") == 0 ) {
        PlaySound(BeepCode, sizeof(BeepCode));
        fprintf(stderr, "Illegal move - %d,%d\n", x, y);
        continue;
      } else {
		//PlaySound(DownCode, sizeof(DownCode));
		PlaySound(ClickCode, sizeof(ClickCode));
	  }	  
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
			  play_gomoku_next = 1;
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
            SetStatusLED(LED_GO, (player_color_next == 0)?COLOR_RED_LOW:COLOR_GREEN_LOW);
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
		send_command(output_fd, "undo\n", input_fd, result, sizeof(result));
		send_command(output_fd, "undo\n", input_fd, result, sizeof(result));
		x_last_computer = x_last2_computer; y_last_computer = y_last2_computer;		
		set_move(x_last2_computer, y_last2_computer);
#ifdef DEBUG
		move_history_pos[0] = POS(x_last2_computer, y_last2_computer);;
#endif		
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
	
    set_move(x, y);
#ifdef DEBUG
    move_history_pos[0] = POS(x, y);
#endif

    send_command(output_fd, "showboard_raw\n", input_fd, result, sizeof(result));
    send_rawboard(result);
    update_board(result);
#ifdef DEBUG
    ascii_showboard();
#endif
    }
	
    SetStatusLED(LED_INFO, 0);
#ifdef DEBUG 
    printf("%s is thinking...\n", name);
#endif

    if(player_color) {
    SetStatusLED(LED_TURN, COLOR_RED_LOW);
    send_command(output_fd, "genmove black\n", input_fd, result, sizeof(result));
	} else {
    SetStatusLED(LED_TURN, COLOR_GREEN_LOW);	
    send_command(output_fd, "genmove white\n", input_fd, result, sizeof(result));	
	}

    if (( strcasecmp(result,"PASS") == 0 )||( strcasecmp(result,"resign") == 0 )) {
       break;
    }
    parse_result(result, &y, &x);
    
    printf("get move %d,%d\n",x,y);
       
    set_move(x, y);
#ifdef DEBUG
    move_history_pos[0] = POS(x, y);
#endif
	x_last2_computer = x_last_computer; y_last2_computer = y_last_computer;	
	x_last_computer = x; y_last_computer = y;	
	
    PlaySound(MoveCode, sizeof(MoveCode));
	undo = 0;
  }
  
  if(!do_exit) {
  send_command(output_fd, "showboard_raw\n", input_fd, result, sizeof(result));
  send_rawboard(result);
  update_board(result);
#ifdef DEBUG
  ascii_showboard();
#endif

  send_command(output_fd, "final_score\n", input_fd, result, sizeof(result));
  printf("Final score:%s\n",result);

  if( result[0] == 'B' )
  color_win = 0;
  else
  color_win = 1;

  DisplayString(result, color_win);
  DisplayString(result, color_win);
  DisplayString(result, color_win);
  sleep(1);
  }
  
  send_command(output_fd, "quit\n", input_fd, result, sizeof(result));

  if(!do_exit) {
  if( color_win == player_color ) {
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
  
  if( play_gomoku_next ) {
	  return 2;//restart next game
  }
  
  if( player_color_next != player_color ) {
	  return 1;//restart same game with different player color
  }
  
  return 0;//restart same game  
}

void usage__r(void)
{
  printf("\ncmd for gnugo.\n");
  printf("\n");
  printf("Usage: cmd_gnugo [OPTIONS]\n");
  printf("\n");
  printf("Options:\n");
  printf("  -b gnugo\n");
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

    if(player_color) {
    execl( pbrain, pbrain, "--mode", "gtp", "--cache-size", "16", "--color", "white", NULL );
	} else {
    execl( pbrain, pbrain, "--mode", "gtp", "--cache-size", "16", NULL );
	}
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
  
  SetStatusLED(LED_GO, (player_color == 0)?COLOR_RED_LOW:COLOR_GREEN_LOW);
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
