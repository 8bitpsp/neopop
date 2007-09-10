#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"

#include "menu.h"

PSP_MODULE_INFO(PSP_APP_NAME, 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

static void ExitCallback(void* arg)
{
  ExitPSP = 1;
}
int main(int argc, char **argv)
{
  /* Initialize PSP */
  pspInit(argv[0]);
  pspAudioInit(1024);
  pspCtrlInit();
  pspVideoInit();

#ifdef PSP_DEBUG
  FILE *debug = fopen("message.txt", "w");
  fclose(debug);
#endif

  /* Initialize callbacks */
  pspRegisterCallback(PSP_EXIT_CALLBACK, ExitCallback, NULL);
  pspStartCallbackThread();

  /* Show the menu */
  if (InitMenu())
  {
    DisplayMenu();
    TrashMenu();
  }

  /* Release PSP resources */
  pspAudioShutdown();
  pspVideoShutdown();
  pspShutdown();

  return(0);
}

