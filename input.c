#include "termtex.h"

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
    if (c == DEL_KEY)
      updateCursor(RIGHT_ARROW);
    editorDelChar();
    break;

  case CTRL_KEY('h'):
  {
    if (E_Config.row[E_Config.cursor_y].chars[E_Config.cursor_x - 1] == ' ')
    {
      editorDelChar();
      break;
    }
    while (E_Config.row[E_Config.cursor_y].chars[E_Config.cursor_x - 1] != ' ')
    {
      editorDelChar();
    }
  }
  break;

  case CTRL_KEY('l'):
    E_Config.panes[E_Config.active_pane].active = 0;
    E_Config.active_pane = (E_Config.active_pane + 1) % 4;
    E_Config.panes[E_Config.active_pane].active = 1;
    break;
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
  pane *p = &E_Config.panes[E_Config.active_pane];
  eRow *row = (p->cursor_y >= p->numRows) ? NULL : &E_Config.row[p->row_buffer_start + p->cursor_y];
  switch (c)
  {
  case UP_ARROW:
    if (p->cursor_y != p->base_row - 1)
      p->cursor_y--;
    break;
  case DOWN_ARROW:
    if (p->cursor_y < (p->base_row - 1) + (p->numRows - 1))
      p->cursor_y++;
    break;
  case RIGHT_ARROW:
    if (row && p->cursor_x < row->size)
    {
      p->cursor_x++;
    }
    else if (p->cursor_y < p->numRows - 1)
    {
      p->cursor_y++;
      p->cursor_x = 0;
    }
    break;
  case LEFT_ARROW:
    if (p->cursor_x != 0)
    {
      p->cursor_x--;
    }
    else if (p->cursor_x == 0 && p->cursor_y > p->base_row - 1)
    {
      p->cursor_y--;
      p->cursor_x = E_Config.row[p->row_buffer_start + p->cursor_y].size;
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
    if (c == BACKSPACE || c == CTRL_KEY('h') || c == DEL_KEY)
    {
      if (bufLen != 0)
        buf[--bufLen] = '\0';
    }
    else if (c == '\r')
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
