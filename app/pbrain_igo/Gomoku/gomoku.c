#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

static int do_exit = 0;

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


#define MIN_BOARD          1       /* Minimum supported board size.   */
#define MAX_BOARD         9       /* Maximum supported board size.   */

#define DEFAULT_BOARD_SIZE MAX_BOARD

#define BOARDSIZE     ((MAX_BOARD + 2) * (MAX_BOARD + 1) + 1)
#define BOARDMIN      (MAX_BOARD + 2)
#define BOARDMAX      (MAX_BOARD + 1) * (MAX_BOARD + 1)
#define POS(i, j)     ((MAX_BOARD + 2) + (i) * (MAX_BOARD + 1) + (j))

#define BOARD(i, j)   board[POS(i, j)]

#ifndef EMPTY
#define EMPTY     0		/* . */
#define WHITE     1		/* O */
#define BLACK     2		/* X */
#endif
#define OUT_BOARD 3		/* # */

#define PASS_MOVE     0
#define NO_MOVE       PASS_MOVE

typedef unsigned char Intersection;

int          board_size = DEFAULT_BOARD_SIZE; /* board size */
Intersection board[BOARDSIZE];

#define MAX_MOVE_HISTORY 1       /* Max number of moves remembered. */

int          move_history_color[MAX_MOVE_HISTORY];
int          move_history_pos[MAX_MOVE_HISTORY];

/*
 * Create letterbar for the top and bottom of the ASCII board.
 */

static void
make_letterbar(int boardsize, char *letterbar)
{
  int i, letteroffset;
  char spaces[64];
  char letter[64];

  if (boardsize <= 25)
    strcpy(spaces, " ");
  strcpy(letterbar, "   ");
  
  for (i = 0; i < boardsize; i++) {
    letteroffset = 'A';
    if (i+letteroffset >= 'I')
      letteroffset++;
    strcat(letterbar, spaces);
    sprintf(letter, "%c", i+letteroffset);
    strcat(letterbar, letter);
  }
}


/* This array contains +'s and -'s for the empty board positions.
 * hspot_size contains the board size that the grid has been
 * initialized to.
 */

static int hspot_size;
static char hspots[MAX_BOARD][MAX_BOARD];


/*
 * Mark the handicap spots on the board.
 */

static void
set_handicap_spots(int boardsize)
{
  if (hspot_size == boardsize)
    return;
  
  hspot_size = boardsize;
  
  memset(hspots, '.', sizeof(hspots));

  if (boardsize == 5) {
    /* place the outer 4 */
    hspots[1][1] = '+';
    hspots[boardsize-2][1] = '+';
    hspots[1][boardsize-2] = '+';
    hspots[boardsize-2][boardsize-2] = '+';
    /* and the middle one */
    hspots[boardsize/2][boardsize/2] = '+';
    return;
  }

  if (!(boardsize%2)) {
    /* If the board size is even, no center handicap spots. */
    if (boardsize > 2 && boardsize < 12) {
      /* Place the outer 4 only. */
      hspots[2][2] = '+';
      hspots[boardsize-3][2] = '+';
      hspots[2][boardsize-3] = '+';
      hspots[boardsize-3][boardsize-3] = '+';
    }
    else {
      /* Place the outer 4 only. */
      hspots[3][3] = '+';
      hspots[boardsize-4][3] = '+';
      hspots[3][boardsize-4] = '+';
      hspots[boardsize-4][boardsize-4] = '+';
    }
  }
  else {
    /* Uneven board size */
    if (boardsize > 2 && boardsize < 12) {
      /* Place the outer 4... */
      hspots[2][2] = '+';
      hspots[boardsize-3][2] = '+';
      hspots[2][boardsize-3] = '+';
      hspots[boardsize-3][boardsize-3] = '+';

      /* ...and the middle one. */
      hspots[boardsize/2][boardsize/2] = '+';
    }
    else if (boardsize > 12) {
      /* Place the outer 4... */
      hspots[3][3] = '+';
      hspots[boardsize-4][3] = '+';
      hspots[3][boardsize-4] = '+';
      hspots[boardsize-4][boardsize-4] = '+';

      /* ...and the inner 4... */
      hspots[3][boardsize/2] = '+';
      hspots[boardsize/2][3] = '+';
      hspots[boardsize/2][boardsize-4] = '+';
      hspots[boardsize-4][boardsize/2] = '+';

      /* ...and the middle one. */
      hspots[boardsize/2][boardsize/2] = '+';
    }
  }

  return;
}


/*
 * Display the board position when playing in ASCII.
 */

static void
ascii_showboard(void)
{
  int i, j;
  char letterbar[64];
  int last_pos_was_move;
  int pos_is_move;
  int dead;
  int last_move = get_last_move();
  
  make_letterbar(board_size, letterbar);
  set_handicap_spots(board_size);

  printf("%s", letterbar);

  if (get_last_player() != EMPTY) {
    fprintf(stdout, "        Last move: %s %1m",
	     get_last_player() == WHITE ? "White" : "Black",
	     last_move);
  }

  printf("\n");
  fflush(stdout);
  
  for (i = 0; i < board_size; i++) {
    printf(" %2d", board_size - i);
    last_pos_was_move = 0;
    for (j = 0; j < board_size; j++) {
      if (POS(i, j) == last_move)
	pos_is_move = 128;
      else
	pos_is_move = 0;
      dead = 0;
      switch (BOARD(i, j) + pos_is_move + last_pos_was_move) {
	case EMPTY+128:
	case EMPTY:
	  printf(" %c", hspots[i][j]);
	  last_pos_was_move = 0;
	  break;
	case BLACK:
	  printf(" %c", dead ? 'x' : 'X');
	  last_pos_was_move = 0;
	  break;
	case WHITE:
	  printf(" %c", dead ? 'o' : 'O');
	  last_pos_was_move = 0;
	  break;
	case BLACK+128:
	  printf("(%c)", 'X');
	  last_pos_was_move = 256;
	  break;
	case WHITE+128:
	  printf("(%c)", 'O');
	  last_pos_was_move = 256;
	  break;
	case EMPTY+256:
	  printf("%c", hspots[i][j]);
	  last_pos_was_move = 0;
	  break;
	case BLACK+256:
	  printf("%c", dead ? 'x' : 'X');
	  last_pos_was_move = 0;
	  break;
	case WHITE+256:
	  printf("%c", dead ? 'o' : 'O');
	  last_pos_was_move = 0;
	  break;
	default: 
	  fprintf(stderr, "Illegal board value %d\n", (int) BOARD(i, j));
	  exit(EXIT_FAILURE);
	  break;
      }
    }
    
    if (last_pos_was_move == 0) {
      if (board_size > 10)
	printf(" %2d", board_size - i);
      else
	printf(" %1d", board_size - i);
    }
    else {
      if (board_size > 10)
	printf("%2d", board_size - i);
      else
	printf("%1d", board_size - i);
    }
    printf("\n");
  }
  
  fflush(stdout);
  printf("%s\n\n", letterbar);
  fflush(stdout);
  
}  /* end ascii_showboard */

/*
 * Convert the string str to a 1D coordinate. Return NO_MOVE if invalid
 * string.
 */

int
string_to_location(int boardsize, const char *str, int *x, int *y)
{
  int m, n;
  
  if (*str == '\0')
    return NO_MOVE;

  if (!isalpha((int) *str))
    return NO_MOVE;
  
  n = tolower((int) *str) - 'a';
  if (tolower((int) *str) >= 'i')
    --n;
  if (n < 0 || n > boardsize - 1)
    return NO_MOVE;

  if (!isdigit((int) *(str + 1)))
    return NO_MOVE;
  
  m = boardsize - atoi(str + 1);
  if (m < 0 || m > boardsize - 1)
    return NO_MOVE;

  *x=m;
  *y=n;
  return POS(m, n);
}

/* Return the last move done by anyone. Both if no move was found or
 * if the last move was a pass, PASS_MOVE is returned.
 */
int
get_last_move()
{
	return move_history_pos[0];
}

/* Return the color of the player doing the last move. If no move was
 * found, EMPTY is returned.
 */
int
get_last_player()
{
	return move_history_color[0];
}

static char cmd[256];
static char result[8196];

/** read a line from STDIN */
static void get_line()
{
	int c, bytes;

	bytes=0;
	do{
		c=getchar();
		if(c==EOF) exit(0);
		if(bytes<sizeof(cmd)) cmd[bytes++]=(char)c;
	} while(c!='\n');
	cmd[bytes-1]=0;
	if(cmd[bytes-2]=='\r') cmd[bytes-2]=0;
}

/** parse coordinates x,y */
static int parse_coord(const char *param, int *x, int *y)
{
	if(sscanf(param, "%d,%d", x, y)!=2 ||
		*x<0 || *y<0 || *x>=DEFAULT_BOARD_SIZE || *y>=DEFAULT_BOARD_SIZE){
		return 0;
	}
	return 1;
}

static int parse_result(const char *result, int *x, int *y)
{
	char *tmp;
	
	if( parse_coord(result, x, y) ) {
	return 1;
	} 
	
	tmp = result;
	while( tmp= strchr(tmp, '\n') ) {
		if( parse_coord(tmp, x, y) ) {
			return 1;
		}
		tmp++;
	}
	
	return 0;
}

//从左到右相同棋子数
int LeftRight(int i, int j,int side)
{

	int tempi,count;
	tempi = i;
	count = 1;
	//toleft
	while ( --tempi > 0 && BOARD(tempi, j) == side)
	{
		count ++;
	}
	tempi = i;
	while ( ++ tempi <20 && BOARD(tempi, j) == side )
	{
		count ++;
	}
	return count;
}

//从上到下相同棋子数
int UpDown(int i, int j,int side)
{
	int tempj,count;
	tempj = j;
	count = 1;

	while ( --tempj > 0 && BOARD(i, tempj) == side)
	{
		count ++;
	}
	tempj = j;
	while ( ++ tempj <20 &&BOARD(i, tempj) == side )
	{
		count ++;
	}
	return count;
}

//从左上到右下相同的棋子数
int LupToRdown(int i, int j,int side)
{
	int tempi,tempj,count;
	tempi = i,tempj = j;
	count = 1;

	while ( --tempi > 0 && -- tempj >0 && BOARD(tempi, tempj) == side)
	{
		count ++;
	}
	tempi = i,tempj = j;
	while ( ++ tempi<20 && ++ tempj < 20 &&BOARD(tempi, tempj) == side )
	{
		count ++;
	}
	return count;
}

//从右上到左下的相同棋子数
int RuptoLdown(int i, int j,int side)
{
	int tempi,tempj,count;
	tempi = i,tempj = j;
	count = 1;

	while ( --tempi > 0 && ++tempj <20 && BOARD(tempi, tempj) == side)
	{
		count ++;
	}
	tempi = i,tempj = j;
	while ( ++ tempi<20 && -- tempj > 0 && BOARD(tempi, tempj) == side )
	{
		count ++;
	}
	return count;
}

int IsSuccess(int i, int j, int side)
{
	if ( LeftRight(i,j,side) >= 5 || UpDown(i,j,side) >= 5
		||LupToRdown(i,j,side) >= 5 || RuptoLdown(i,j,side)>=5)
	{
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
    int pid;
    int              error;
    int         status;
	int length;
	int step = 0;
	int to_move,is_win;
	int x,y;
	
	int pipe_stdin[2], pipe_stdout[2];

	if( argc != 2 ) {
		printf("Usage: gomoku pbrain\n");
		return -1;
	}

    /* setup signal handlers */
    signal(SIGINT, sigintproc);
    signal(SIGTERM, sigtermproc);
	
    if ( (pipe(pipe_stdin)<0) || (pipe(pipe_stdout)<0) ) {
        fprintf(stderr, "%s", "PIPE BUILD ERROR\n");
        exit(-1);
    }
	
	pid = fork();
	
	if(!pid)
	{

	printf("start app %s\n",argv[1]);
	close(pipe_stdin[1]);
	close(pipe_stdout[0]);
    if ((dup2(pipe_stdin[0], /*fileno(stdin)*/STDIN_FILENO) < 0)||(dup2(pipe_stdout[1], /*fileno(stdout)*/STDOUT_FILENO) < 0)) { 
        fprintf(stderr, "%s", "REDRIECT ERROR\n");
        exit(-1);
    }

		execl( argv[1], argv[1], NULL );
	    printf("App '%s' , errno:%d\n", argv[1], errno);

		return 0;
	}

	close(pipe_stdin[0]);
	close(pipe_stdout[1]);

	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,"ABOUT\n");
	write(pipe_stdin[1],cmd,strlen(cmd));
	length=read(pipe_stdout[0],result,sizeof(result));
	result[length]='\0';
	fprintf(stderr,"read:%s\n",result);

	sprintf(cmd,"START %d\n",DEFAULT_BOARD_SIZE);
	write(pipe_stdin[1],cmd,strlen(cmd));
	length=read(pipe_stdout[0],result,sizeof(result));
	result[length]='\0';
	fprintf(stderr,"read:%s\n",result);
	
	while(!do_exit) {
	step++;		
	ascii_showboard();
	to_move = BLACK;
	printf("%s(%d): ", to_move == WHITE ? "WHITE":"BLACK", step);
	get_line();
	string_to_location(board_size, cmd, &x, &y);
	sprintf(cmd,"TURN %d,%d\n",x,y);
	fprintf(stderr,"write:%s",cmd);
        if( BOARD(x, y) ) {
        fprintf(stderr,"Illegal move- %d,%d\n",x,y);
        continue;
        }
	move_history_pos[0] = POS(x, y);
	BOARD(x, y) = to_move;
	if( is_win = IsSuccess(x,y,to_move) ){
	  break;
	}
	write(pipe_stdin[1],cmd,strlen(cmd));
	ascii_showboard();	
	printf("%s is thinking...\n",argv[1]);
	while(!do_exit) {	
	length=read(pipe_stdout[0],result,sizeof(result));
	result[length]='\0';
	fprintf(stderr,"read:%s\n",result);
	if( parse_result(result, &x, &y) ) {
		sprintf(cmd, "TURN %d,%d\n",x,y);
		break;
	  } else {
		continue;
	  }
	}
	to_move = WHITE;
	move_history_pos[0] = POS(x, y);
	BOARD(x, y) = to_move;
	if( is_win = IsSuccess(x,y,to_move) ){
	  break;
	}	
	}
	
	ascii_showboard();
    if(is_win) printf("%s WIN!\n", to_move == WHITE ? "WHITE":"BLACK");

	sprintf(cmd,"END\n");
	write(pipe_stdin[1],cmd,strlen(cmd));
	
	error = waitpid( pid, 
                       &status,
                       0 /*options*/);
	printf("child '%s' status %d, ret %d, errno:%d\n", argv[1], status,
                error, errno);

	return 0;
}

