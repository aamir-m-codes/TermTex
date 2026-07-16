#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>

struct termios original_attr;

void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_attr);
}

void enableRawMode()
{
  tcgetattr(STDIN_FILENO, &original_attr);
  atexit(disableRawMode);

  struct termios raw = original_attr;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_iflag &= ~(IXON | ICRNL);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cflag |= (CS8);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
  enableRawMode();
  while (1)
  {
    char c = '\0';
    read(STDIN_FILENO, &c, 1);
    if (iscntrl(c))
    {
      printf("%d\r\n", c);
    }
    else
    {
      printf("%d ('%c')\r\n", c, c);
    }
    if (c == 'q')
      break;
  }
  return 0;
}