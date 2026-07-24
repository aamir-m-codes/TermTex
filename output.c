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
  drawDivider(&ab);
  for (int i = 0; i < TOTAL_PANES; i++)
  {
    drawPaneRows(&ab, i);
  }
  char resetXY[10];
  int xyLen = snprintf(resetXY, sizeof(resetXY), "\x1b[%d;0H", E_Config.screenRows + 1);
  bufferAppend(&ab, resetXY, xyLen);
  statusBar(&ab);
  statusMessage(&ab);
  repositionCursor(&ab);

  pane *p = &E_Config.panes[E_Config.active_pane];
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (p->base_row + p->cursor_y - p->row_offset), (p->base_col + p->cursor_x - p->col_offset));
  bufferAppend(&ab, buf, strlen(buf));

  write(STDOUT_FILENO, ab.buf, ab.len);
  bufFree(&ab);
}

void drawDivider(struct buffer *ab)
{
  for (int i = 0; i < E_Config.screenRows + 1; i++)
  {
    if (i + 1 == E_Config.rowMid)
    {
      for (int j = 0; j < E_Config.screenCols; j++)
      {
        bufferAppend(ab, "\u2500", 4);
      }
    }
    bufferAppend(ab, "\x1b[K", 3);
    char buf[10];
    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", i + 1, E_Config.colMid);
    bufferAppend(ab, buf, len);
    if (i + 1 == E_Config.rowMid)
      bufferAppend(ab, "\u253c", 4);
    else
      bufferAppend(ab, "\u2502", 4);
    bufferAppend(ab, "\r\n", 2);
  }
}

void editorScroll()
{
  pane *p = &E_Config.panes[E_Config.active_pane];
  if (p->cursor_y < p->row_offset)
  {
    p->row_offset = p->cursor_y;
  }

  if (p->cursor_y >= p->row_offset + p->paneRows)
  {
    p->row_offset = p->cursor_y - p->paneRows + 1;
  }

  if (p->cursor_x < p->col_offset)
  {
    p->col_offset = p->cursor_x;
  }

  if (p->cursor_x >= p->col_offset + p->paneCols)
  {
    p->col_offset = p->cursor_x - p->paneCols + 1;
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
  size_t cLen = snprintf(cursorStatus, sizeof(cursorStatus), "Pane %d,Ln %d,Col %d", E_Config.active_pane + 1, E_Config.cursor_y + 1, E_Config.cursor_x + 1);
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

void drawPaneRows(struct buffer *ab, int p)
{
  if (p >= TOTAL_PANES)
    return;

  pane *pPane = &E_Config.panes[p];
  int y = pPane->base_row;
  int rowPrinted = 0;
  for (int r = 0; r < pPane->paneRows; r++)
  {
    int prefix = r + pPane->row_offset;
    int paneRow;
    if (prefix <= pPane->numRows)
    {
      paneRow = prefix + pPane->row_buffer_start;
    }
    char coords[10];
    int coordLen = snprintf(coords, sizeof(coords), "\x1b[%d;%dH", y, pPane->base_col);
    bufferAppend(ab, coords, coordLen);
    if (prefix >= pPane->numRows)
    {
      bufferAppend(ab, "~", 1);
    }
    else
    {
      int len = E_Config.row[paneRow].size - pPane->col_offset;
      if (len < 0)
        len = 0;
      if (len > pPane->paneCols)
        len = pPane->paneCols;
      bufferAppend(ab, &E_Config.row[paneRow].chars[pPane->col_offset], len);
      rowPrinted++;
    }
    y++;
    bufferAppend(ab, "\r\n", 2);
  }
}
