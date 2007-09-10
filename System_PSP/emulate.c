#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psptypes.h>
#include <psprtc.h>

#include "neopop.h"
#include "system_rom.h"

#include "emulate.h"

#include "image.h"
#include "psp.h"
#include "video.h"
#include "perf.h"
#include "ctrl.h"

PspImage *Screen;

static PspFpsCounter FpsCounter;
static int ScreenX, ScreenY, ScreenW, ScreenH;

_u8 system_frameskip_key;

/* Initialize emulation */
int InitEmulation()
{
  system_message("Initializing... ");

	/* Fill the bios buffer however it needs to be... */
	if (!bios_install()) return 0;

	mute = FALSE;
	system_colour = COLOURMODE_AUTO;
	language_english = TRUE;
	system_frameskip_key = 8;		/* 1 - 7 */

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

  return 1;
}

void CopyBuffer()
{
	int x, y;
	_u16 *row;
	int r,g,b;

	_u16 *frame16;
	_u32 w = Screen->Width;

	row = cfb;
	frame16 = Screen->Pixels;

	for (y = 0; y < SCREEN_HEIGHT; y++)
	{
		for (x = 0; x < SCREEN_WIDTH; x++)
		{
			r = ((row[x] & 0x00F) << 4) | (row[x] & 0x00F);
			g = ((row[x] & 0x0F0)) | ((row[x] & 0x0F0) >> 4);
			b = ((row[x] & 0xF00) >> 4) | ((row[x] & 0xF00) >> 8);

      frame16[x] = RGB(r, g, b);
		}

		frame16 += w;
		row += SCREEN_WIDTH;
	}
}

void system_graphics_update()
{
  pspVideoBegin();

  CopyBuffer();  

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

void system_sound_update()
{
/*
	int		pdwAudioBytes1, pdwAudioBytes2, count;
	int		Write, LengthBytes;
	_u16	*chipPtr1, *chipPtr2, *src16, *dest16;
	_u8		*dacPtr1, *dacPtr2, *src8, *dest8;

	IDirectSoundBuffer_GetCurrentPosition(chipBuffer, NULL, &Write);

	// UNDEFINED write cursors
	if (chipWrite == UNDEFINED || dacWrite == UNDEFINED)
	{
		lastChipWrite = chipWrite = Write;

		// Get DAC position too.
		IDirectSoundBuffer_GetCurrentPosition(dacBuffer, NULL, &Write);
		lastDacWrite = dacWrite = Write;

		return; //Wait a frame to accumulate length.
	}

	//Chip -> Direct Sound
	//====================

	if (Write < lastChipWrite)	//Wrap?
		lastChipWrite -= CHIPBUFFERLENGTH;

	LengthBytes = Write - lastChipWrite;
	lastChipWrite = Write;
*/
//	sound_update((_u16*)block, LengthBytes>>1);	//Get sound data
/*
	if SUCCEEDED(IDirectSoundBuffer_Lock(chipBuffer, 
		chipWrite, LengthBytes, &chipPtr1, &pdwAudioBytes1, 
		&chipPtr2, &pdwAudioBytes2, 0))
	{
		src16 = (_u16*)block;	//Copy from this buffer

		dest16 = chipPtr1;
		count = pdwAudioBytes1 >> 2;
		while(count)
		{ 
			*dest16++ = *src16;
			*dest16++ = *src16++; count--; 
		}

		//Buffer Wrap?
		if (chipPtr2)
		{
			dest16 = chipPtr2;
			count = pdwAudioBytes2 >> 2;
			while(count)
			{ 
				*dest16++ = *src16;
				*dest16++ = *src16++; count--; 
			}
		}
		
		IDirectSoundBuffer_Unlock(chipBuffer, 
			chipPtr1, pdwAudioBytes1, chipPtr2, pdwAudioBytes2);

		chipWrite += LengthBytes;
		if (chipWrite > CHIPBUFFERLENGTH)
			chipWrite -= CHIPBUFFERLENGTH;
	}
	else
	{
		DWORD status;
		chipWrite = UNDEFINED;
		IDirectSoundBuffer_GetStatus(chipBuffer, &status);
		if (status & DSBSTATUS_BUFFERLOST)
		{
			if (IDirectSoundBuffer_Restore(chipBuffer) != DS_OK) return;
			if (!mute) IDirectSoundBuffer_Play(chipBuffer, 0, 0, DSBPLAY_LOOPING);
		}
	}

	//DAC -> Direct Sound
	//===================
	IDirectSoundBuffer_GetCurrentPosition(dacBuffer, NULL, &Write);

	if (Write < lastDacWrite)	//Wrap?
		lastDacWrite -= DACBUFFERLENGTH;

	LengthBytes = (Write - lastDacWrite); 
	lastDacWrite = Write;
*/
//	dac_update(block, LengthBytes);	//Get DAC data
/*
	if SUCCEEDED(IDirectSoundBuffer_Lock(dacBuffer, 
		dacWrite, LengthBytes, &dacPtr1, &pdwAudioBytes1, 
		&dacPtr2, &pdwAudioBytes2, 0))
	{
		src8 = block;	//Copy from this buffer

		dest8 = dacPtr1;
		count = pdwAudioBytes1;
		while(count)
		{ 
		   *dest8++ = *src8++; 
		   count--;
		}

		//Buffer Wrap?
		if (dacPtr2)
		{
		   dest8 = dacPtr2;
		   count = pdwAudioBytes2;
		   while(count)
		   { 
			   *dest8++ = *src8++; 
			   count--;
		   }
		}
		
		IDirectSoundBuffer_Unlock(dacBuffer, 
			dacPtr1, pdwAudioBytes1, dacPtr2, pdwAudioBytes2);

		dacWrite += LengthBytes;
		if (dacWrite >= DACBUFFERLENGTH)
			dacWrite -= DACBUFFERLENGTH;
	}
	else
	{
		DWORD status;
		dacWrite = UNDEFINED;
		IDirectSoundBuffer_GetStatus(dacBuffer, &status);
		if (status & DSBSTATUS_BUFFERLOST)
		{
			if (IDirectSoundBuffer_Restore(dacBuffer) != DS_OK) return;
			if (!mute) IDirectSoundBuffer_Play(dacBuffer, 0, 0, DSBPLAY_LOOPING);
		}
	}
*/
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

	/* Update Sound */
	system_sound_update();
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

  pspSetClockFrequency(333);

  while (!ExitPSP)
  {
    emulate();
  }

  pspSetClockFrequency(222);

  system_message("Exiting emulation\n");
}

/* Release emulation resources */
void TrashEmulation()
{
  system_message("Releasing resources... ");

  pspImageDestroy(Screen);

  system_message("done\n");
}
