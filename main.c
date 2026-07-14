#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

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
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
  enableRawMode();
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
  return 0;
}