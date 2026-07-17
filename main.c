#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define CTRL_KEY(key) ((key) & 0x1F)

/*** Data Section ***/
struct termios original_attr;

/*** Error Handling Section ***/
void die(const char *err_msg)
{
  perror(err_msg);
  exit(1);
}

/*** Terminal Section ***/
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

/*** Input section ***/
void editorProcessKeyPress()
{
  char c = editorReadKey();
  switch (c)
  {
  case CTRL_KEY('q'):
    exit(0);
    break;
  }
}

/*** Output Section ***/
void refreshEditorScreen()
{
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** init ***/
int main()
{
  enableRawMode();
  while (1)
  {
    refreshEditorScreen();
    editorProcessKeyPress();
  }
  return 0;
}