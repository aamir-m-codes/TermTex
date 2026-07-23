#include "termtex.h"

struct editorConfig E_Config;

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