#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>

#include "menu.h"
#include "emulate.h"

int InitMenu()
{
  if (!InitEmulation())
    return 0;
}

void DisplayMenu()
{
  RunEmulation();
}

void TrashMenu()
{
  /* Free emulation-specific resources */
  TrashEmulation();
}
