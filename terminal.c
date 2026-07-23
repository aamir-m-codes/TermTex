#include "termtex.h"

/*** Error Handling Section ***/
void die(const char *err_msg)
{
  struct buffer ab = BUFFER_INIT;
  clearScreen(&ab);
  repositionCursor(&ab);

  write(STDOUT_FILENO, ab.buf, ab.len);
  bufFree(&ab);
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

int editorReadKey()
{
  char c;
  int nread;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
  {
    if (nread == -1 && errno != EAGAIN)
      die("Error in read");
  }
  if (c == '\x1b')
  {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';
    if (seq[0] == '[')
    {
      if (seq[1] >= '0' && seq[1] <= '9')
      {
        if (read(STDIN_FILENO, &seq[2], 1) != 1)
          return '\x1b';
        if (seq[2] == '~')
        {
          switch (seq[1])
          {
          case '1':
          case '7':
            return HOME_KEY;
          case '4':
          case '8':
            return END_KEY;
          case '5':
            return PAGE_UP;
          case '6':
            return PAGE_DOWN;
          case '3':
            return DEL_KEY;
          }
        }
      }
      else
      {
        switch (seq[1])
        {
        case 'A':
          return UP_ARROW;
        case 'B':
          return DOWN_ARROW;
        case 'C':
          return RIGHT_ARROW;
        case 'D':
          return LEFT_ARROW;
        case 'H':
          return HOME_KEY;
        case 'F':
          return END_KEY;
        }
      }
    }
    else
    {
      if (seq[0] == 'O')
      {
        switch (seq[1])
        {
        case 'H':
          return HOME_KEY;
        case 'F':
          return END_KEY;
        }
      }
    }

    return '\x1b';
  }
  else
  {
    return c;
  }
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
  size_t i = 0;
  printf("\r\n");
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
