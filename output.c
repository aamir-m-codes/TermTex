#include "termtex.h"

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
  if ((int)len > E_Config.screenCols)
    len = E_Config.screenCols;
  bufferAppend(ab, status, len);
  size_t cLen = snprintf(cursorStatus, sizeof(cursorStatus), "Ln %d,Col %d", E_Config.cursor_y + 1, E_Config.cursor_x + 1);
  while ((int)len < E_Config.screenCols)
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
