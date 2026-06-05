
#define MIN_BOARD          1       /* Minimum supported board size.   */

#if 0
#define MAX_BOARD          9       /* Maximum supported board size.   */
#define BOARD_MASK  0x1ff
#else
#define MAX_BOARD         19       /* Maximum supported board size.   */
#define BOARD_MASK  0x7ffff
#endif

#define DEFAULT_BOARD_SIZE MAX_BOARD

#define BOARDSIZE     ((MAX_BOARD + 2) * (MAX_BOARD + 1) + 1)
#define BOARDMIN      (MAX_BOARD + 2)
#define BOARDMAX      (MAX_BOARD + 1) * (MAX_BOARD + 1)
#define POS(i, j)     ((MAX_BOARD + 2) + (i) * (MAX_BOARD + 1) + (j))

#define BOARD(i, j)   board[POS(i, j)]

#ifndef EMPTY
#define EMPTY     0             /* . */
#define WHITE     1             /* O */
#define BLACK     2             /* X */
#endif
#define OUT_BOARD 3             /* # */

#define PASS_MOVE     0
#define NO_MOVE       PASS_MOVE

typedef unsigned char Intersection;

int          board_size = DEFAULT_BOARD_SIZE; /* board size */
Intersection board[BOARDSIZE];

#define MAX_MOVE_HISTORY 1       /* Max number of moves remembered. */

int          move_history_color[MAX_MOVE_HISTORY];
int          move_history_pos[MAX_MOVE_HISTORY];

/* This array contains +'s and -'s for the empty board positions.
   hspot_size contains the board size that the grid has been
   initialized to.
*/

static int hspot_size;
static char hspots[MAX_BOARD][MAX_BOARD];

int get_last_move();
int get_last_player();

/*
   Create letterbar for the top and bottom of the ASCII board.
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
    if (i + letteroffset >= 'I')
      letteroffset++;
    strcat(letterbar, spaces);
    sprintf(letter, "%c", i + letteroffset);
    strcat(letterbar, letter);
  }
}

/*
   Mark the handicap spots on the board.
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
    hspots[boardsize - 2][1] = '+';
    hspots[1][boardsize - 2] = '+';
    hspots[boardsize - 2][boardsize - 2] = '+';
    /* and the middle one */
    hspots[boardsize / 2][boardsize / 2] = '+';
    return;
  }

  if (!(boardsize % 2)) {
    /* If the board size is even, no center handicap spots. */
    if (boardsize > 2 && boardsize < 12) {
      /* Place the outer 4 only. */
      hspots[2][2] = '+';
      hspots[boardsize - 3][2] = '+';
      hspots[2][boardsize - 3] = '+';
      hspots[boardsize - 3][boardsize - 3] = '+';
    }
    else {
      /* Place the outer 4 only. */
      hspots[3][3] = '+';
      hspots[boardsize - 4][3] = '+';
      hspots[3][boardsize - 4] = '+';
      hspots[boardsize - 4][boardsize - 4] = '+';
    }
  }
  else {
    /* Uneven board size */
    if (boardsize > 2 && boardsize < 12) {
      /* Place the outer 4... */
      hspots[2][2] = '+';
      hspots[boardsize - 3][2] = '+';
      hspots[2][boardsize - 3] = '+';
      hspots[boardsize - 3][boardsize - 3] = '+';

      /* ...and the middle one. */
      hspots[boardsize / 2][boardsize / 2] = '+';
    }
    else if (boardsize > 12) {
      /* Place the outer 4... */
      hspots[3][3] = '+';
      hspots[boardsize - 4][3] = '+';
      hspots[3][boardsize - 4] = '+';
      hspots[boardsize - 4][boardsize - 4] = '+';

      /* ...and the inner 4... */
      hspots[3][boardsize / 2] = '+';
      hspots[boardsize / 2][3] = '+';
      hspots[boardsize / 2][boardsize - 4] = '+';
      hspots[boardsize - 4][boardsize / 2] = '+';

      /* ...and the middle one. */
      hspots[boardsize / 2][boardsize / 2] = '+';
    }
  }

  return;
}


/*
   Display the board position when playing in ASCII.
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

/* Return the last move done by anyone. Both if no move was found or
   if the last move was a pass, PASS_MOVE is returned.
*/
int
get_last_move()
{
  return move_history_pos[0];
}

/* Return the color of the player doing the last move. If no move was
   found, EMPTY is returned.
*/
int
get_last_player()
{
  return move_history_color[0];
}

