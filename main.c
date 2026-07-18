#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>

#define CTRL_KEY(key) ((key) & 0x1F)

/*** Functions Prototype ***/
void die(const char *err_msg);
void disableRawMode();
void enableRawMode();
char editorReadKey();
void editorProcessKeyPress();
void clearScreen();
void repositionCursor();
void refreshEditorScreen();
void drawEditorRows();
int getWindowSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);

/*** Data Section ***/
struct editorConfig
{
  int screenRows;
  int screenCols;
  struct termios original_term_attr;
};

struct editorConfig E_Config;

/*** Buffer Section ***/
struct buffer
{
  char *buf;
  int len;
};

#define BUFFER_INIT {NULL, 0}

void bufferAppend(struct buffer *abuf, char *s, int len)
{
  char *new = realloc(abuf->buf, abuf->len + len);

  if (new = NULL)
    return;

  memcpy(&new[abuf->len], s, len);
  abuf->buf = new;
  abuf->len += len;
}

void bufFree(struct buffer *b)
{
  free(b->buf);
}

/*** Error Handling Section ***/
void die(const char *err_msg)
{
  clearScreen();
  repositionCursor();
  perror(err_msg);
  exit(1);
}

/*** Terminal Section ***/
void disableRawMode()
{
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E_Config.original_term_attr) == -1)
    die("Error in tcsetattr");
}

void enableRawMode()
{
  if (tcgetattr(STDIN_FILENO, &E_Config.original_term_attr) == -1)
    die("Error in tcgetattr");
  atexit(disableRawMode);

  struct termios raw = E_Config.original_term_attr;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_iflag &= ~(IXON | ICRNL);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cflag |= (CS8);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("Error in tcsetattr");
}

char editorReadKey()
{
  char c;
  int nread;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
  {
    if (nread == -1 && errno != EAGAIN)
      die("Error in read");
  }
  return c;
}

int getWindowSize(int *rows, int *cols)
{
  struct winsize wsz;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsz) == -1 || wsz.ws_col == 0)
  {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;

    return getCursorPosition(rows, cols);
  }
  else
  {
    *cols = wsz.ws_col;
    *rows = wsz.ws_row;
    return 0;
  }
}

int getCursorPosition(int *rows, int *cols)
{
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  char buf[32];
  int i = 0;
  printf("\r\n");
  char c;
  while (i < (sizeof(buf) - 1))
  {
    if (read(STDOUT_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }

  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}

/*** Input section ***/
void editorProcessKeyPress()
{
  char c = editorReadKey();
  switch (c)
  {
  case CTRL_KEY('q'):
    clearScreen();
    repositionCursor();
    exit(0);
    break;
  }
}

/*** Output Section ***/
void clearScreen()
{
  write(STDOUT_FILENO, "\x1b[2J", 4);
}

void repositionCursor()
{
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void refreshEditorScreen()
{
  clearScreen();
  repositionCursor();
  drawEditorRows();
  repositionCursor();
}

void drawEditorRows()
{
  for (int i = 0; i < E_Config.screenRows; i++)
  {
    write(STDOUT_FILENO, "~", 1);
    if (i < E_Config.screenRows - 1)
      write(STDOUT_FILENO, "\r\n", 2);
  }
}

/*** init ***/
void initEditor()
{
  if (getWindowSize(&E_Config.screenRows, &E_Config.screenCols) == -1)
    die("Error in window size");
}

int main()
{
  enableRawMode();
  initEditor();
  while (1)
  {
    refreshEditorScreen();
    editorProcessKeyPress();
  }
  return 0;
}