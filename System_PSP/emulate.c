#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspgu.h>
#include <psptypes.h>
#include <psprtc.h>

#include "neopop.h"
#include "system_rom.h"

#include "emulate.h"

#include "audio.h"
#include "image.h"
#include "psp.h"
#include "video.h"
#include "perf.h"
#include "ctrl.h"

PspImage *Screen;

static PspFpsCounter FpsCounter;
static int ScreenX, ScreenY, ScreenW, ScreenH;
static unsigned char SoundReady;

void AudioCallback(void* buf, unsigned int *length, void *userdata);

_u8 system_frameskip_key;

/* Initialize emulation */
int InitEmulation()
{
  system_message("Initializing... ");

	/* Initialize BIOS */
	if (!bios_install()) return 0;

  /* Basic emulator initialization */
	language_english = TRUE;
	system_colour = COLOURMODE_AUTO;

  /* Load ROM */
  if (!system_load_rom("flack.ngp")) return 0;

  /* Reset */
	reset();

  system_message("OK\n");

  /* Create screen buffer */
  if (!(Screen = pspImageCreateVram(256, 256, PSP_IMAGE_16BPP)))
    	return 0;

  Screen->Viewport.Width = SCREEN_WIDTH;
  Screen->Viewport.Height = SCREEN_HEIGHT;
  Screen->TextureFormat = GU_PSM_4444; /* Override default 5551 */
  cfb = Screen->Pixels;

  return 1;
}

void system_graphics_update()
{
  pspVideoBegin();

  /* Perf counter */
  static char fps_display[64];
  sprintf(fps_display, " %3.02f ", pspPerfGetFps(&FpsCounter));

  int width = pspFontGetTextWidth(&PspStockFont, fps_display);
  int height = pspFontGetLineHeight(&PspStockFont);

  pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_COLOR_BLACK);
  pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_COLOR_WHITE);
  /* End */

  /* Blit screen */
  pspVideoPutImage(Screen, ScreenX, ScreenY, ScreenW, ScreenH);
  pspVideoEnd();

  pspVideoSwapBuffers();
}

void system_input_update()
{
  static SceCtrlData pad;
  int up, down, left, right, button_a, button_b, option;

  pspCtrlPollControls(&pad);

  up = (pad.Buttons & PSP_CTRL_UP) ? 1 : 0;
  down = (pad.Buttons & PSP_CTRL_DOWN) ? 1 : 0;
  left = (pad.Buttons & PSP_CTRL_LEFT) ? 1 : 0;
  right = (pad.Buttons & PSP_CTRL_RIGHT) ? 1 : 0;
  button_a = (pad.Buttons & PSP_CTRL_CROSS) ? 1 : 0;
  button_b = (pad.Buttons & PSP_CTRL_SQUARE) ? 1 : 0;
  option = (pad.Buttons & PSP_CTRL_SELECT) ? 1 : 0;

	/* Write the controller status to memory */
	ram[0x6F82] = up | (down << 1) | (left << 2) | (right << 3) | 
		(button_a << 4) | (button_b << 5) | (option << 6);
}

/*! Called at the start of the vertical blanking period, this function is
	designed to perform many of the critical hardware interface updates
	Here is a list of recommended actions to take:
	
	- The frame buffer should be copied to the screen.
	- The frame rate should be throttled to 59.95hz
	- The sound chips should be polled for the next chunk of data
	- Input should be polled and the current status written to "ram[0x6F82]" */
void system_VBL(void)
{
	/* Update Graphics */
  system_graphics_update();

	/* Update Input */
	system_input_update();

	/* Actual sound update is done in the callback */
	SoundReady = 1;

  /* TODO: Throttle framerate */
}

/* Run emulation */
void RunEmulation()
{
  system_message("Entering emulation loop\n");

  /* Initialize performance counter */
  pspPerfInitFps(&FpsCounter);

  ScreenX = 0;
  ScreenY = 0;
  ScreenW = Screen->Viewport.Width;
  ScreenH = Screen->Viewport.Height;

 	mute = FALSE;
	system_frameskip_key = 4;		/* 1 - 7 */
	SoundReady = 0;

  sceGuDisable(GU_BLEND); /* Disable alpha blending */

  pspSetClockFrequency(333);

  if (!mute) pspAudioSetChannelCallback(0, AudioCallback, 0);

  while (!ExitPSP)
  {
    emulate();
  }

  if (!mute) pspAudioSetChannelCallback(0, NULL, 0);

  pspSetClockFrequency(222);

  sceGuEnable(GU_BLEND); /* Re-enable alpha blending */

  system_message("Exiting emulation\n");
}

void AudioCallback(void* buf, unsigned int *length, void *userdata)
{
  int length_bytes = *length << 2;
  if (!SoundReady) memset(buf, 0, length_bytes);
	else sound_update_stereo((_u16*)buf, length_bytes);
	SoundReady = 0;
}

/* Release emulation resources */
void TrashEmulation()
{
  system_message("Releasing resources... ");

  pspImageDestroy(Screen);

  system_message("done\n");
}
