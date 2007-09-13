#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspgu.h>
#include <psptypes.h>
#include <psprtc.h>

#include "neopop.h"

#include "emulate.h"
#include "emumenu.h"

#include "audio.h"
#include "image.h"
#include "psp.h"
#include "video.h"
#include "perf.h"
#include "ctrl.h"
#include "util.h"

PspImage *Screen;

static PspFpsCounter FpsCounter;
static int ScreenX, ScreenY, ScreenW, ScreenH;
static int ClearScreen;
static int TicksPerUpdate;
static u32 TicksPerSecond;
static u64 LastTick;
static u64 CurrentTick;
static unsigned char SoundReady, ReturnToMenu;

extern char *GameName;
extern EmulatorOptions Options;
extern const u64 ButtonMask[];
extern const int ButtonMapId[];
extern struct ButtonConfig ActiveConfig;
extern char *ScreenshotPath;

static void AudioCallback(void* buf, unsigned int *length, void *userdata);

_u8 system_frameskip_key;

/* Initialize emulation */
int InitEmulation()
{
	/* Initialize BIOS */
	if (!bios_install()) return 0;

  /* Basic emulator initialization */
	language_english = TRUE;
	system_colour = COLOURMODE_AUTO;

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

  /* Clear the buffer first, if necessary */
  if (ClearScreen >= 0)
  {
    ClearScreen--;
    pspVideoClearScreen();
  }

  /* Blit screen */
  pspVideoPutImage(Screen, ScreenX, ScreenY, ScreenW, ScreenH);

  /* Show FPS counter */
  if (Options.ShowFps)
  {
	  static char fps_display[64];
	  sprintf(fps_display, " %3.02f ", pspPerfGetFps(&FpsCounter));

	  int width = pspFontGetTextWidth(&PspStockFont, fps_display);
	  int height = pspFontGetLineHeight(&PspStockFont);

	  pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_COLOR_BLACK);
	  pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_COLOR_WHITE);
	}

  pspVideoEnd();

  pspVideoSwapBuffers();
}

void system_input_update()
{
  _u8 *input_ptr = &ram[0x6F82];

  /* Reset input */
  *input_ptr = 0;

  static SceCtrlData pad;

  /* Check the input */
  if (pspCtrlPollControls(&pad))
  {
#ifdef PSP_DEBUG
    if ((pad.Buttons & (PSP_CTRL_SELECT | PSP_CTRL_START))
      == (PSP_CTRL_SELECT | PSP_CTRL_START))
        pspUtilSaveVramSeq(ScreenshotPath, "game");
#endif

    /* Parse input */
    int i, on, code;
    for (i = 0; ButtonMapId[i] >= 0; i++)
    {
      code = ActiveConfig.ButtonMap[ButtonMapId[i]];
      on = (pad.Buttons & ButtonMask[i]) == ButtonMask[i];

      /* Check to see if a button set is pressed. If so, unset it, so it */
      /* doesn't trigger any other combination presses. */
      if (on) pad.Buttons &= ~ButtonMask[i];

      if (code & JOY)
      {
        if (on) *input_ptr |= CODE_MASK(code);
      }
      else if (code & SPC)
      {
        switch (CODE_MASK(code))
        {
        case SPC_MENU:
          ReturnToMenu = on; break;
        }
      }
    }
  }
}

/*! Called at the start of the vertical blanking period */
void system_VBL(void)
{
	/* Update Graphics */
  system_graphics_update();

	/* Update Input */
	system_input_update();

	/* Actual sound update is done in the callback */
	SoundReady = 1;

  /* Wait if needed */
  if (Options.UpdateFreq)
  {
    do { sceRtcGetCurrentTick(&CurrentTick); }
    while (CurrentTick - LastTick < TicksPerUpdate);
    LastTick = CurrentTick;
  }

  /* Wait for VSync signal */
  if (Options.VSync) pspVideoWaitVSync();
}

/* Run emulation */
void RunEmulation()
{
  float ratio;

  /* Recompute screen size/position */
  switch (Options.DisplayMode)
  {
  default:
  case DISPLAY_MODE_UNSCALED:
    ScreenW = Screen->Viewport.Width;
    ScreenH = Screen->Viewport.Height;
    break;
  case DISPLAY_MODE_FIT_HEIGHT:
    ratio = (float)SCR_HEIGHT / (float)Screen->Viewport.Height;
    ScreenW = (float)Screen->Viewport.Width * ratio - 2;
    ScreenH = SCR_HEIGHT;
    break;
  case DISPLAY_MODE_FILL_SCREEN:
    ScreenW = SCR_WIDTH - 3;
    ScreenH = SCR_HEIGHT;
    break;
  }

  ScreenX = (SCR_WIDTH / 2) - (ScreenW / 2);
  ScreenY = (SCR_HEIGHT / 2) - (ScreenH / 2);

  /* Initialize performance counter */
  pspPerfInitFps(&FpsCounter);
  pspImageClear(Screen, 0);

  /* Recompute update frequency */
  TicksPerSecond = sceRtcGetTickResolution();
  if (Options.UpdateFreq)
  {
    TicksPerUpdate = TicksPerSecond
      / (Options.UpdateFreq / (Options.Frameskip + 1));
    sceRtcGetCurrentTick(&LastTick);
  }

 	mute = Options.SoundOn;
	system_frameskip_key = Options.Frameskip + 1; /* 1 - 7 */
	SoundReady = 0;
  ReturnToMenu = 0;
  ClearScreen = 1;

  sceGuDisable(GU_BLEND); /* Disable alpha blending */

  if (!mute) pspAudioSetChannelCallback(0, AudioCallback, 0);

  /* Wait for V. refresh */
  pspVideoWaitVSync();

  while (!ExitPSP && !ReturnToMenu)
    emulate();

  if (!mute) pspAudioSetChannelCallback(0, NULL, 0);

  sceGuEnable(GU_BLEND); /* Re-enable alpha blending */
}

void AudioCallback(void* buf, unsigned int *length, void *userdata)
{
  int length_bytes = *length << 2; /* 4 bytes per stereo sample */

  /* If the sound buffer's not ready, render silence */
  if (!SoundReady) memset(buf, 0, length_bytes);
	else sound_update_stereo((_u16*)buf, length_bytes);
	SoundReady = 0;
}

/* Release emulation resources */
void TrashEmulation()
{
  pspImageDestroy(Screen);
}

