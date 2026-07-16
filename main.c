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
  raw.c_cflag &= ~(CS8);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
  enableRawMode();
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
  {
    if (iscntrl(c))
    {
      printf("%d\n", c);
    }
    else
    {
      printf("%d ('%c')\n", c, c);
    }
  }
  return 0;
}