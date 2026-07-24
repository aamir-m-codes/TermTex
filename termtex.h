#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#ifndef TERMTEX_H
#define TERMTEX_H

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
#define TOTAL_PANES 4

#define K_K_START "[K_K_START]"
#define K_K_END "[K_K_END]"
#define K_U_START "[K_U_START]"
#define K_U_END "[K_U_END]"
#define U_K_START "[U_K_START]"
#define U_K_END "[U_K_END]"
#define U_U_START "[U_U_START]"
#define U_U_END "[U_U_END]"

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

typedef struct eRow eRow;
struct editorConfig;
struct buffer;
typedef struct editorPane pane;
typedef struct fileBlock fb;

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
void editorDelChar();
void rowAppendString(eRow *row, char *s, int len);
void rowDel(int at);
void rowFree(eRow *row);
void editorInserNewLine();
char *editorRowsToString(int *len);
void editorSaveFile();
char *editorPrompt(char *prompt);
void loadPanesRows(fb *bl, int at, char *line, size_t lineLen);
void drawPane(struct buffer *ab, int p);

struct eRow
{
  int size;
  char *chars;
};

struct fileBlock
{
  int active;
  char **start;
  char **end;
};

#define INIT_FB ((fb){                                               \
    .active = -1,                                                    \
    .start = (char *[]){K_K_START, K_U_START, U_K_START, U_U_START}, \
    .end = (char *[]){K_K_END, K_U_END, U_K_END, U_U_END},           \
})

struct editorPane
{
  int base_row;
  int base_col;
  int row_bound;
  int col_bound;
  int cursor_x;
  int cursor_y;
  int row_offset;
  int col_offset;
  int paneRows;
  int paneCols;
  int numRows;
  int row_buffer_start;
  int row_buffer_end;
  int active;
};

struct editorConfig
{
  int cursor_x;
  int cursor_y;
  int row_offset;
  int col_offset;
  int screenRows;
  int screenCols;
  int rowMid;
  int colMid;
  int numRows;
  int dirty;
  int active_pane;
  char *filename;
  char statusMsg[80];
  time_t status_msg_time;
  eRow *row;
  pane panes[TOTAL_PANES];
  struct termios original_term_attr;
};

extern struct editorConfig E_Config;

struct buffer
{
  char *buf;
  int len;
};

#define BUFFER_INIT {NULL, 0}

#endif