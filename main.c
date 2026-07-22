#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>

/*** Defines Section ***/
#define TERMTEX_VERSION "0.0.1"
#define QUIT_TIMES 3

#define CTRL_KEY(key) ((key) & 0x1F)

#define BACKSPACE 127
#define UP_ARROW 1000
#define DOWN_ARROW 1001
#define RIGHT_ARROW 1002
#define LEFT_ARROW 1003
#define HOME_KEY 1004
#define END_KEY 1005
#define PAGE_UP 1006
#define PAGE_DOWN 1007
#define DEL_KEY 1008

/*** Types declaration ***/
typedef struct eRow eRow;
struct editorConfig;
struct buffer;

/*** Functions Prototype ***/
void bufferAppend(struct buffer *abuf, char *s, int len);
void bufFree(struct buffer *b);
void die(const char *err_msg);
void disableRawMode();
void enableRawMode();
int editorReadKey();
void editorProcessKeyPress();
void clearScreen(struct buffer *ab);
void repositionCursor(struct buffer *ab);
void refreshEditorScreen();
void drawEditorRows(struct buffer *ab);
int getWindowSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);
void initEditor();
void updateCursor(int c);
void editorOpen(char *filename);
void rowAppendAt(int at, char *line, size_t lineLen);
void editorScroll();
void statusBar(struct buffer *ab);
void setStatusMessage(const char *fmt, ...);
void statusMessage(struct buffer *ab);
void rowInsertChar(eRow *row, int at, char c);
void editorInsertChar(int c);
void rowDelChar(eRow *row, int at);
void editorDelChar(int at);
void rowAppendString(eRow *row, char *s, int len);
void rowDel(int at);
void rowFree(eRow *row);
void editorInserNewLine();
char *editorRowsToString(int *len);
void editorSaveFile();
char *editorPrompt(char *prompt);

/*** Data Section ***/
struct eRow
{
  int size;
  char *chars;
};

struct editorConfig
{
  int cursor_x;
  int cursor_y;
  int row_offset;
  int col_offset;
  int screenRows;
  int screenCols;
  int numRows;
  int dirty;
  char *filename;
  char statusMsg[80];
  time_t status_msg_time;
  eRow *row;
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

  if (new == NULL)
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
  static int quit_times = QUIT_TIMES;

  int c = editorReadKey();
  switch (c)
  {
  case '\r':
    editorInserNewLine();
    break;

  case CTRL_KEY('q'):
    if (E_Config.dirty && quit_times > 0)
    {
      setStatusMessage("WARNING!!! File has unsaved changes."
                       "Press Ctrl-Q %d more times to quit.",
                       quit_times);
      quit_times--;
      return;
    }
    clearScreen(NULL);
    repositionCursor(NULL);
    exit(0);
    break;

  case CTRL_KEY('s'):
    editorSaveFile();
    break;

  case UP_ARROW:
  case DOWN_ARROW:
  case RIGHT_ARROW:
  case LEFT_ARROW:
  case HOME_KEY:
  case END_KEY:
    updateCursor(c);
    break;

  case PAGE_UP:
  case PAGE_DOWN:
  {
    if (c == PAGE_UP && E_Config.cursor_y != E_Config.row_offset)
    {
      E_Config.cursor_y = E_Config.row_offset;
    }
    else if (c == PAGE_DOWN && E_Config.cursor_y != E_Config.row_offset + E_Config.screenRows - 1)
    {
      E_Config.cursor_y = E_Config.row_offset + E_Config.screenRows - 1;
      if (E_Config.cursor_y > E_Config.numRows)
      {
        E_Config.cursor_y = E_Config.numRows - 1;
        E_Config.cursor_x = E_Config.row[E_Config.cursor_y].size;
      }
    }
    else
    {
      if (E_Config.cursor_y == 0)
      {
        E_Config.cursor_x = 0;
        break;
      }
      int times = E_Config.screenRows;
      while (times)
      {
        updateCursor(c == PAGE_UP ? UP_ARROW : DOWN_ARROW);
        times--;
      }
    }
  }
  break;

  case BACKSPACE:
  case DEL_KEY:
  case CTRL_KEY('h'):
    if (c == DEL_KEY)
      updateCursor(RIGHT_ARROW);
    editorDelChar(c);
    break;

  case CTRL_KEY('l'):
  case '\x1b':
    /* IGNORE THEM for now because it escape*/
    break;

  default:
    editorInsertChar(c);
    break;
  }

  quit_times = QUIT_TIMES;
}

void updateCursor(int c)
{
  eRow *row = (E_Config.cursor_y >= E_Config.numRows) ? NULL : &E_Config.row[E_Config.cursor_y];
  switch (c)
  {
  case UP_ARROW:
    if (E_Config.cursor_y != 0)
      E_Config.cursor_y--;
    break;
  case DOWN_ARROW:
    if (E_Config.cursor_y < E_Config.numRows - 1)
      E_Config.cursor_y++;
    break;
  case RIGHT_ARROW:
    if (row && E_Config.cursor_x < row->size)
    {
      E_Config.cursor_x++;
    }
    else if (E_Config.cursor_y < E_Config.numRows - 1)
    {
      E_Config.cursor_y++;
      E_Config.cursor_x = 0;
    }
    break;
  case LEFT_ARROW:
    if (E_Config.cursor_x != 0)
    {
      E_Config.cursor_x--;
    }
    else if (E_Config.cursor_x == 0 && E_Config.cursor_y > 0)
    {
      E_Config.cursor_y--;
      E_Config.cursor_x = E_Config.row[E_Config.cursor_y].size;
    }
    break;
  case HOME_KEY:
    E_Config.cursor_x = 0;
    break;
  case END_KEY:
    if (row)
      E_Config.cursor_x = row->size;
    break;
  }

  row = (E_Config.cursor_y >= E_Config.numRows) ? NULL : &E_Config.row[E_Config.cursor_y];
  int rowLen = (row) ? row->size : 0;
  if (E_Config.cursor_x > rowLen)
    E_Config.cursor_x = rowLen;
}

char *editorPrompt(char *prompt)
{
  size_t bufSize = 128;
  char *buf = malloc(bufSize);

  size_t bufLen = 0;
  buf[0] = '\0';

  while (1)
  {
    setStatusMessage(prompt, buf);
    refreshEditorScreen();

    int c = editorReadKey();
    if (c == '\r')
    {
      if (bufLen != 0)
      {
        setStatusMessage("");
        return buf;
      }
    }
    else if (c == '\x1b')
    {
      setStatusMessage("");
      free(buf);
      return NULL;
    }
    else if (!iscntrl(c) && c < 128)
    {
      if (bufLen == bufSize - 1)
      {
        bufSize *= 2;
        buf = realloc(buf, bufSize);
      }
      buf[bufLen++] = c;
      buf[bufLen] = '\0';
    }
  }
}

/*** Output Section ***/
void clearScreen(struct buffer *ab)
{
  if (ab == NULL)
    write(STDOUT_FILENO, "\x1b[2J", 4);
  else
    bufferAppend(ab, "\x1b[2J", 4);
}

void repositionCursor(struct buffer *ab)
{
  if (ab == NULL)
    write(STDOUT_FILENO, "\x1b[H", 3);
  else
    bufferAppend(ab, "\x1b[H", 3);
}

void refreshEditorScreen()
{
  editorScroll();
  struct buffer ab = BUFFER_INIT;
  repositionCursor(&ab);
  drawEditorRows(&ab);
  statusBar(&ab);
  statusMessage(&ab);
  repositionCursor(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E_Config.cursor_y - E_Config.row_offset) + 1, (E_Config.cursor_x - E_Config.col_offset) + 1);
  bufferAppend(&ab, buf, strlen(buf));

  write(STDOUT_FILENO, ab.buf, ab.len);
  bufFree(&ab);
}

void drawEditorRows(struct buffer *ab)
{
  for (int i = 0; i < E_Config.screenRows; i++)
  {
    int fileRow = i + E_Config.row_offset;
    if (fileRow >= E_Config.numRows)
    {
      if (E_Config.numRows == 0 && i == E_Config.screenRows / 3)
      {
        char welcome[64];
        int welcomeLen = snprintf(welcome, sizeof(welcome), "Welcome to TermTex Editor -- version %s", TERMTEX_VERSION);
        if (welcomeLen > E_Config.screenCols)
          welcomeLen = E_Config.screenCols;
        int padding = (E_Config.screenCols - welcomeLen) / 2;
        if (padding)
        {
          bufferAppend(ab, "~", 1);
          padding--;
        }
        while (padding)
        {
          bufferAppend(ab, " ", 1);
          padding--;
        }

        bufferAppend(ab, welcome, welcomeLen);
      }
      else
      {
        bufferAppend(ab, "~", 1);
      }
    }
    else
    {
      int len = E_Config.row[fileRow].size - E_Config.col_offset;
      if (len < 0)
        len = 0;
      if (len > E_Config.screenCols)
        len = E_Config.screenCols;
      bufferAppend(ab, &E_Config.row[fileRow].chars[E_Config.col_offset], len);
    }
    bufferAppend(ab, "\x1b[K", 3);
    bufferAppend(ab, "\r\n", 2);
  }
}

void editorScroll()
{
  if (E_Config.cursor_y < E_Config.row_offset)
  {
    E_Config.row_offset = E_Config.cursor_y;
  }

  if (E_Config.cursor_y >= E_Config.row_offset + E_Config.screenRows)
  {
    E_Config.row_offset = E_Config.cursor_y - E_Config.screenRows + 1;
  }

  if (E_Config.cursor_x < E_Config.col_offset)
  {
    E_Config.col_offset = E_Config.cursor_x;
  }

  if (E_Config.cursor_x >= E_Config.col_offset + E_Config.screenCols)
  {
    E_Config.col_offset = E_Config.cursor_x - E_Config.screenCols + 1;
  }
}

void statusBar(struct buffer *ab)
{
  bufferAppend(ab, "\x1b[7m", 4);
  char status[80];
  char cursorStatus[80];
  size_t len = snprintf(status, sizeof(status), " %.20s - %d Lines %s",
                        E_Config.filename ? E_Config.filename : "[No Name]", E_Config.numRows,
                        E_Config.dirty ? "(Modified)" : "");
  if (len > E_Config.screenCols)
    len = E_Config.screenCols;
  bufferAppend(ab, status, len);
  size_t cLen = snprintf(cursorStatus, sizeof(cursorStatus), "Ln %d,Col %d", E_Config.cursor_y + 1, E_Config.cursor_x + 1);
  while (len < E_Config.screenCols)
  {
    if (E_Config.screenCols - len == cLen)
    {
      bufferAppend(ab, cursorStatus, cLen);
      break;
    }
    else
    {
      bufferAppend(ab, " ", 1);
      len++;
    }
  }
  bufferAppend(ab, "\x1b[m", 3);
  bufferAppend(ab, "\r\n", 2);
}

void setStatusMessage(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E_Config.statusMsg, sizeof(E_Config.statusMsg), fmt, ap);
  va_end(ap);
  E_Config.status_msg_time = time(NULL);
}

void statusMessage(struct buffer *ab)
{
  bufferAppend(ab, "\x1b[K", 3);
  int msgLen = strlen(E_Config.statusMsg);
  if (msgLen > E_Config.screenCols)
    msgLen = E_Config.screenCols;
  if (msgLen && time(NULL) - E_Config.status_msg_time < 5)
    bufferAppend(ab, E_Config.statusMsg, msgLen);
}

/*** Row Operations Section ***/
void rowAppendAt(int at, char *line, size_t lineLen)
{
  if (at < 0 || at > E_Config.numRows)
    return;

  E_Config.row = realloc(E_Config.row, sizeof(eRow) * (E_Config.numRows + 1));
  memmove(&E_Config.row[at + 1], &E_Config.row[at], sizeof(eRow) * (E_Config.numRows - at));

  E_Config.row[at].size = lineLen;
  E_Config.row[at].chars = malloc(lineLen + 1);
  memcpy(E_Config.row[at].chars, line, lineLen);
  E_Config.row[at].chars[lineLen] = '\0';
  E_Config.numRows++;
}

void rowInsertChar(eRow *row, int at, char c)
{
  if (at < 0 || at > row->size)
    at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->chars[at] = c;
  row->size++;
  E_Config.dirty++;
}

void rowDelChar(eRow *row, int at)
{
  if (at < 0 || at > row->size)
    return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  E_Config.dirty++;
}

void rowAppendString(eRow *row, char *s, int len)
{
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
}

void rowDel(int at)
{
  if (at < 0 || at > E_Config.numRows)
    return;
  rowFree(&E_Config.row[at]);
  memmove(&E_Config.row[E_Config.cursor_y], &E_Config.row[E_Config.cursor_y + 1], (E_Config.numRows - at - 1) * sizeof(eRow));
  E_Config.numRows--;
  E_Config.dirty++;
}

void rowFree(eRow *row)
{
  free(row->chars);
}

/*** Editor Operations Section ***/
void editorInsertChar(int c)
{
  if (E_Config.cursor_y == E_Config.numRows)
  {
    rowAppendAt(E_Config.numRows, "", 0);
  }
  rowInsertChar(&E_Config.row[E_Config.cursor_y], E_Config.cursor_x, c);
  E_Config.cursor_x++;
}

void editorDelChar(int at)
{
  if (E_Config.cursor_x == 0 && E_Config.cursor_y == 0)
    return;
  if (E_Config.cursor_x > 0)
  {
    rowDelChar(&E_Config.row[E_Config.cursor_y], E_Config.cursor_x - 1);
    E_Config.cursor_x--;
  }
  else
  {
    E_Config.cursor_x = E_Config.row[E_Config.cursor_y - 1].size;
    rowAppendString(&E_Config.row[E_Config.cursor_y - 1], E_Config.row[E_Config.cursor_y].chars, E_Config.row[E_Config.cursor_y].size);
    rowDel(E_Config.cursor_y);
    E_Config.cursor_y--;
  }
}

void editorInserNewLine()
{
  if (E_Config.cursor_x == 0)
  {
    rowAppendAt(E_Config.cursor_y + 1, "", 0);
  }
  else
  {
    struct eRow *row = &E_Config.row[E_Config.cursor_y];
    rowAppendAt(E_Config.cursor_y + 1, &row->chars[E_Config.cursor_x], row->size - E_Config.cursor_x);
    row = &E_Config.row[E_Config.cursor_y];
    row->size = E_Config.cursor_x;
    row->chars[row->size] = '\0';
  }
  E_Config.cursor_x = 0;
  E_Config.cursor_y++;
  E_Config.dirty++;
}

/*** File I/O Section ***/
void editorOpen(char *filename)
{
  free(E_Config.filename);
  E_Config.filename = strdup(filename);

  FILE *fileP = fopen(filename, "r");
  if (!fileP)
    die("Error in File Opening");

  char *line = NULL;
  size_t lineCap = 0;
  ssize_t lineLen;
  while ((lineLen = getline(&line, &lineCap, fileP)) != -1)
  {
    while (lineLen > 0 && (line[lineLen - 1] == '\r' || line[lineLen - 1] == '\n'))
    {
      lineLen--;
    }
    rowAppendAt(E_Config.numRows, line, lineLen);
  }

  free(line);
  fclose(fileP);
  E_Config.dirty = 0;
}

char *editorRowsToString(int *len)
{
  int total_len = 0;
  for (int i = 0; i < E_Config.numRows; i++)
  {
    total_len += E_Config.row[i].size + 1;
  }
  total_len--;
  *len = total_len;

  char *buf = malloc(total_len);
  char *p = buf;
  for (int j = 0; j < E_Config.numRows; j++)
  {
    memcpy(p, E_Config.row[j].chars, E_Config.row[j].size);
    p += E_Config.row[j].size;
    if (j < E_Config.numRows - 1)
    {
      *p = '\n';
      p++;
    }
  }
  return buf;
}

void editorSaveFile()
{
  if (E_Config.filename == NULL)
    return;
  int len;
  char *buf = editorRowsToString(&len);

  int file = open(E_Config.filename, O_RDWR | O_CREAT, 0644);
  if (file != -1)
  {
    if (ftruncate(file, len) != -1)
    {
      if (write(file, buf, len) == len)
      {
        close(file);
        free(buf);
        E_Config.dirty = 0;
        setStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(file);
  }
  free(buf);
  setStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/*** init ***/
void initEditor()
{
  E_Config.cursor_x = 0;
  E_Config.cursor_y = 0;
  E_Config.row_offset = 0;
  E_Config.col_offset = 0;
  E_Config.numRows = 0;
  E_Config.dirty = 0;
  E_Config.filename = NULL;
  E_Config.statusMsg[0] = '\0';
  E_Config.status_msg_time = 0;
  E_Config.row = NULL;
  if (getWindowSize(&E_Config.screenRows, &E_Config.screenCols) == -1)
    die("Error in window size");
  E_Config.screenRows -= 2;
}

int main(int argc, char *argv[])
{
  enableRawMode();
  initEditor();
  if (argc >= 2)
  {
    editorOpen(argv[1]);
  }
  setStatusMessage("Help: Ctrl-Q = Quit | Ctrl-S = Save");
  while (1)
  {
    refreshEditorScreen();
    editorProcessKeyPress();
  }
  return 0;
}