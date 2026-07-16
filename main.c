#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

struct termios original_attr;

void die(const char *err_msg)
{
  perror(err_msg);
  exit(1);
}

void disableRawMode()
{
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_attr) == -1)
    die("Error in tcsetattr");
}

void enableRawMode()
{
  if (tcgetattr(STDIN_FILENO, &original_attr) == -1)
    die("Error in tcgetattr");
  atexit(disableRawMode);

  struct termios raw = original_attr;
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

int main()
{
  enableRawMode();
  while (1)
  {
    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
      die("Error in read");
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