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

#include "board.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

static int do_exit = 0;

#define reverse32(x) ((0xff000000U & x) >> 24|(0x00ff0000U & x) >>  8 |(0x0000ff00U & x) <<  8 |(0x000000ffU & x) << 24)

void BigOrSmall()
{
  int n = 1;
  int x = 0x12345678;
  char *c;

  printf(*(char *)(&n) ? "small\n" : "big\n");

  c = (char *)&x;
  printf("%02x,", *c++);
  printf("%02x,", *c++);
  printf("%02x,", *c++);
  printf("%02x\n", *c++);

  x = reverse32(x);

  c = (char *)&x;
  printf("%02x,", *c++);
  printf("%02x,", *c++);
  printf("%02x,", *c++);
  printf("%02x\n", *c++);

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

/** read a line from STDIN */
static void get_line(char *cmd, int length)
{
  int c, bytes;

  bytes = 0;
  do {
    c = getchar();
    if (c == EOF) exit(0);
    if (bytes < length) cmd[bytes++] = (char)c;
  } while (c != '\n');
  cmd[bytes - 1] = 0;
  if (cmd[bytes - 2] == '\r') cmd[bytes - 2] = 0;
}

/*
   Convert the string str to a 1D coordinate. Return NO_MOVE if invalid
   string.
*/

int
string_to_location(int boardsize, const char *str, int *x, int *y)
{
  int m, n;

  if (*str == '\0')
    return NO_MOVE;

  if (!isalpha((int) *str))
    return NO_MOVE;

  n = tolower((int) * str) - 'a';
  if (tolower((int) *str) >= 'i')
    --n;
  if (n < 0 || n > boardsize - 1)
    return NO_MOVE;

  if (!isdigit((int) * (str + 1)))
    return NO_MOVE;

  m = boardsize - atoi(str + 1);
  if (m < 0 || m > boardsize - 1)
    return NO_MOVE;

  *x = m;
  *y = n;
  return POS(m, n);
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

void get_move(char *cmd, int len, int *row, int *col)
{
      get_line(cmd,len);
      string_to_location(board_size, cmd, row, col);
}

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

  fprintf(stderr, "write:%s", command);

  r = write(output_fd, command, strlen(command));
  if (r < 0) {
    printf("write failed with %i: %s\n", errno, strerror(errno));
    return -1;
  }


  while (!do_exit) {
  r = read(input_fd, &in_char, 1);
  if (r < 0) {
    printf("read failed with %i: %s\n", errno, strerror(errno));
    return -1;
  }

   if( parse_response(in_char, response, response_buf_size) )
	break;
  }

  return -1;
}

void usage__r(void)
{
  printf("\ncmd for gnugo.\n");
  printf("\n");
  printf("Usage: cmd_gnugo [OPTIONS]\n");
  printf("\n");
  printf("Options:\n");
  printf("  -b gnugo\n");
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
  int length;
  int step = 0;
  int to_move, is_win;
  int x, y;
  int boardsize = MAX_BOARD;

  char pbrain[UNIX_PATH_MAX];

  int pipe_stdin[2], pipe_stdout[2];

  char cmd[128];
  char result[512];

  BigOrSmall();

  while ((opt = getopt(argc, argv, "b:p:s:Hh")) != -1)
  {
    switch (opt)
    {
      case 'b':
        strncpy(pbrain, argv[ optind - 1 ], sizeof(pbrain) - 1);
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

  if (config != 1)
  {
    usage__r();
    return -1;
  }

  printf("boardsize is %d\n", boardsize);

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

    execl( pbrain, pbrain, "--mode", "gtp", NULL );
    printf("App '%s' , errno:%d\n", pbrain, errno);

    return 0;
  }

  close(pipe_stdin[0]);
  close(pipe_stdout[1]);

  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "boardsize %d\n", DEFAULT_BOARD_SIZE);
  send_command(pipe_stdin[1], cmd, pipe_stdout[0], result, sizeof(result));

  sprintf(cmd, "clear_board\n");
  send_command(pipe_stdin[1], cmd, pipe_stdout[0], result, sizeof(result));

  while (!do_exit) {
    step++;
 
    sprintf(cmd, "showboard_raw\n");
    send_command(pipe_stdin[1], cmd, pipe_stdout[0], result, sizeof(result));
    printf("result=[%s]\n",result);
    ascii_showboard();

    to_move = BLACK;
    printf("%s(%d): ", to_move == WHITE ? "WHITE" : "BLACK", step);
    /* wait input from user */
    get_move(cmd, sizeof(cmd), &x, &y);

    move_history_pos[0] = POS(x, y);
    BOARD(x, y) = to_move;

    printf("set move %d,%d\n",x,y);
    sprintf(cmd, "play black %c%d\n", number_to_letterbar(y), DEFAULT_BOARD_SIZE - x);
    send_command(pipe_stdin[1], cmd, pipe_stdout[0], result, sizeof(result));

    printf("result=[%s]\n",result);
    if ( strcmp(result,"illegal move") == 0 ) {
      fprintf(stderr, "Illegal move- %d,%d\n", x, y);
      continue;
    }

    sprintf(cmd, "showboard_raw\n");
    send_command(pipe_stdin[1], cmd, pipe_stdout[0], result, sizeof(result));
    printf("result=[%s]\n",result);
    ascii_showboard();

    printf("%s is thinking...\n", pbrain);
    
    sprintf(cmd, "genmove white\n");
    send_command(pipe_stdin[1], cmd, pipe_stdout[0], result, sizeof(result));

    parse_result(result, &y, &x);
    
    printf("get move %d,%d\n",x,y);
       
    to_move = WHITE;
    move_history_pos[0] = POS(x, y);
    BOARD(x, y) = to_move;
  }

  ascii_showboard();

  sprintf(cmd, "quit\n");
  send_command(pipe_stdin[1], cmd, pipe_stdout[0], result, sizeof(result));

  sleep(1);

  error = waitpid( pid,
                   &status,
                   0 /*options*/);
  printf("child '%s' status %d, ret %d, errno:%d\n", pbrain, status,
         error, errno);

  return 0;
}
