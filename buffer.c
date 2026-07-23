#include "termtex.h"

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
