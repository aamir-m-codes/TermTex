#include "termtex.h"

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

void editorDelChar()
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

  fb bl = INIT_FB;
  char *line = NULL;
  size_t lineCap = 0;
  ssize_t lineLen;
  while ((lineLen = getline(&line, &lineCap, fileP)) != -1)
  {
    while (lineLen > 0 && (line[lineLen - 1] == '\r' || line[lineLen - 1] == '\n'))
    {
      lineLen--;
    }
    loadPanesRows(&bl, E_Config.numRows, line, lineLen);
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
  {
    E_Config.filename = editorPrompt("Saved as: %s (ESC to cancel)");
    if (E_Config.filename == NULL)
    {
      setStatusMessage("Save aborted");
      return;
    }
  }

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

void loadPanesRows(fb *bl, int at, char *line, size_t lineLen)
{
  int i = 0;
  for (; i < TOTAL_PANES; i++)
  {
    if (strncmp(line, bl->start[i], lineLen) == 0)
    {
      E_Config.panes[i].row_buffer_start = at;
      bl->active = i;
      return;
    }
  }
  int j = 0;
  for (; j < TOTAL_PANES; j++)
  {
    if (strncmp(line, bl->end[j], lineLen) == 0)
    {
      E_Config.panes[j].row_buffer_end = at;
      bl->active = -1;
      return;
    }
  }
  if (bl->active != -1)
  {
    rowAppendAt(E_Config.numRows, line, lineLen);
    E_Config.panes[bl->active].numRows++;
  }
}

/*** init ***/
void initEditor()
{
  E_Config.cursor_x = 0;
  E_Config.cursor_y = 0;
  E_Config.row_offset = 0;
  E_Config.col_offset = 0;
  E_Config.rowMid = 0;
  E_Config.colMid = 0;
  E_Config.numRows = 0;
  E_Config.dirty = 0;
  E_Config.filename = NULL;
  E_Config.statusMsg[0] = '\0';
  E_Config.status_msg_time = 0;
  E_Config.row = NULL;
  for (int i = 0; i < TOTAL_PANES; i++)
  {
    E_Config.panes[i].base_row = 0;
    E_Config.panes[i].base_col = 0;
    E_Config.panes[i].row_bound = 0;
    E_Config.panes[i].col_bound = 0;
    E_Config.panes[i].cursor_x = 0;
    E_Config.panes[i].cursor_y = 0;
    E_Config.panes[i].row_offset = 0;
    E_Config.panes[i].col_offset = 0;
    E_Config.panes[i].paneRows = 0;
    E_Config.panes[i].paneCols = 0;
    E_Config.panes[i].numRows = 0;
    E_Config.panes[i].row_buffer_start = 0;
    E_Config.panes[i].row_buffer_end = 0;
    E_Config.panes[i].active = (i == 0);
  }
  E_Config.active_pane = 0;
  if (getWindowSize(&E_Config.screenRows, &E_Config.screenCols) == -1)
    die("Error in window size");
  E_Config.screenRows -= 2;
  E_Config.rowMid = E_Config.screenRows / 2;
  E_Config.colMid = E_Config.screenCols / 2 + 1;
  E_Config.screenRows--;
  E_Config.screenCols--;
  E_Config.panes[0].base_row = 0;
  E_Config.panes[1].base_row = 0;
  E_Config.panes[2].base_row = E_Config.rowMid + 1;
  E_Config.panes[3].base_row = E_Config.rowMid + 1;
  E_Config.panes[0].base_col = 0;
  E_Config.panes[1].base_col = E_Config.colMid + 1;
  E_Config.panes[2].base_col = 0;
  E_Config.panes[3].base_col = E_Config.colMid + 1;
  E_Config.panes[0].row_bound = E_Config.rowMid - 1;
  E_Config.panes[1].row_bound = E_Config.rowMid - 1;
  E_Config.panes[2].row_bound = E_Config.screenRows;
  E_Config.panes[3].row_bound = E_Config.screenRows;
  E_Config.panes[0].col_bound = E_Config.colMid - 1;
  E_Config.panes[1].col_bound = E_Config.screenCols;
  E_Config.panes[2].col_bound = E_Config.colMid - 1;
  E_Config.panes[3].col_bound = E_Config.screenCols;
  E_Config.panes[0].paneRows = E_Config.rowMid - 1;
  E_Config.panes[1].paneRows = E_Config.rowMid - 1;
  E_Config.panes[2].paneRows = E_Config.screenRows - E_Config.rowMid;
  E_Config.panes[3].paneRows = E_Config.screenRows - E_Config.rowMid;
}
