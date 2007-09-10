#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "neopop.h"

typedef struct
{
  char label[9];
  char string[256];
}
STRING_TAG;

/* TODO */
const char *FlashDirectory = ".";

static STRING_TAG string_tags[STRINGS_MAX] =
{
  //NOTE: This ordering must match the enumeration 'STRINGS' in neopop.h

  { "SDEFAULT",  "Are you sure you want to revert to the default control setup?" },
  { "ROMFILT",  "Rom Files (*.ngp,*.ngc,*.npc,*.zip)\0*.ngp;*.ngc;*.npc;*.zip\0\0" }, 
  { "STAFILT",  "State Files (*.ngs)\0*.ngs\0\0" },
  { "FLAFILT",  "Flash Memory Files (*.ngf)\0*.ngf\0\0" },
  { "BADFLASH",  "The flash data for this rom is from a different version of NeoPop, it will be destroyed soon." },
  { "POWER",  "The system has been signalled to power down. You must reset or load a new rom." },
  { "BADSTATE",  "State is from an unsupported version of NeoPop." },
  { "ERROR1",  "An error has occured creating the application window" },
  { "ERROR2",  "An error has occured initialising DirectDraw" },
  { "ERROR3",  "An error has occured initialising DirectInput" },
  { "TIMER",  "This system does not have a high resolution timer." },
  { "WRONGROM",  "This state is from a different rom, Ignoring." },
  { "EROMFIND",  "Cannot find ROM file" },
  { "EROMOPEN",  "Cannot open ROM file" },
  { "EZIPNONE", "No roms found" },
  { "EZIPBAD",  "Corrupted ZIP file" },
  { "EZIPFIND", "Cannot find ZIP file" },

  { "ABORT",  "Abort" },
  { "DISCON",  "Disconnect" },
  { "CONNEC",  "Connected." }, 
};

/*! Used to generate a critical message for the user. After the message
  has been displayed, the function should return. The message is not
  necessarily a fatal error. */
  
void system_message(char* vaMessage,...)
{
#ifdef PSP_DEBUG
  FILE *debug = fopen("message.txt", "a");

  va_list vl;
  va_start(vl, vaMessage);
  vfprintf(debug, vaMessage, vl);
  va_end(vl);

  fclose(debug);
#endif
}

/*! Get a string that may possibly be translated */

char* system_get_string(STRINGS string_id)
{
  if (string_id >= STRINGS_MAX)
    return "Unknown String";

  return string_tags[string_id].string;
}

/*! Reads a byte from the other system. If no data is available or no
  high-level communications have been established, then return FALSE.
  If buffer is NULL, then no data is read, only status is returned */

BOOL system_comms_read(_u8* buffer)
{
  return 0;
}

/*! Writes a byte from the other system. This function should block until
  the data is written. USE RELIABLE COMMS! Data cannot be re-requested. */

void system_comms_write(_u8 data)
{
  return;
}

/*! Peeks at any data from the other system. If no data is available or
  no high-level communications have been established, then return FALSE.
  If buffer is NULL, then no data is read, only status is returned */

BOOL system_comms_poll(_u8* buffer)
{
  return 0;
}

/*! Reads from the file specified by 'filename' into the given preallocated
  buffer. This is state data. */

BOOL system_io_state_read(char* filename, _u8* buffer, _u32 bufferLength)
{
  FILE* file;
  file = fopen(filename, "rb");

  if (file)
  {
    fread(buffer, bufferLength, sizeof(_u8), file);
    fclose(file);
    return TRUE;
  }

  return FALSE;
}

/*! Writes to the file specified by 'filename' from the given buffer.
  This is state data. */

BOOL system_io_state_write(char* filename, _u8* buffer, _u32 bufferLength)
{
  FILE* file;
  file = fopen(filename, "wb");

  if (file)
  {
    fwrite(buffer, bufferLength, sizeof(_u8), file);
    fclose(file);
    return TRUE;
  }

  return FALSE;
}

/*! Reads the "appropriate" (system specific) flash data into the given
  preallocated buffer. The emulation core doesn't care where from. */

BOOL system_io_flash_read(_u8* buffer, _u32 bufferLength)
{
  char path[256];
  FILE* file;

  //Build a path to the flash file
  sprintf(path, "%s\\%s.ngf", FlashDirectory, rom.filename);

  file = fopen(path, "rb");

  if (file)
  {
    fread(buffer, bufferLength, sizeof(_u8), file);
    fclose(file);
    return TRUE;
  }

  return FALSE;
}

/*! Writes the given flash data into an "appropriate" (system specific)
  place. The emulation core doesn't care where to. */

BOOL system_io_flash_write(_u8* buffer, _u32 bufferLength)
{
  char path[256];
  FILE* file;

  //Build a path for the flash file
  sprintf(path, "%s\\%s.ngf", FlashDirectory, rom.filename);
  
  file = fopen(path, "wb");

  if (file)
  {
    fwrite(buffer, bufferLength, sizeof(_u8), file);
    fclose(file);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

/*! Reads as much of the file specified by 'filename' into the given, 
  preallocated buffer. This is rom data */

BOOL system_io_rom_read(char* filename, _u8* buffer, _u32 bufferLength)
{
  FILE* file;
  file = fopen(filename, "rb");

  if (file)
  {
    fread(buffer, bufferLength, sizeof(_u8), file);
    fclose(file);
    return TRUE;
  }

  return FALSE;
}

/*! Callback for "sound_init" with the system sound frequency */
  
void system_sound_chipreset()
{
  /* Initialises sound chips, matching frequncies */
  sound_init(CHIP_FREQUENCY);
}

/* Dummy code starts here */

/*! Clears the sound output. */
  
void system_sound_silence()
{
/*
  BYTE  *ppvAudioPtr1, *ppvAudioPtr2;
  DWORD  pdwAudioBytes1, pdwAudioBytes2;

  chipWrite = UNDEFINED;
  dacWrite = UNDEFINED; 

  if (chipBuffer)
  {
    IDirectSoundBuffer_Stop(chipBuffer);

    // Fill the sound buffer
    if SUCCEEDED(IDirectSoundBuffer_Lock(chipBuffer, 0, 0, 
      &ppvAudioPtr1, &pdwAudioBytes1, 
      &ppvAudioPtr2, &pdwAudioBytes2, DSBLOCK_ENTIREBUFFER))
    {
      if (ppvAudioPtr1 && pdwAudioBytes1)
        memset(ppvAudioPtr1, 0, pdwAudioBytes1);

      if (ppvAudioPtr2 && pdwAudioBytes2)
        memset(ppvAudioPtr2, 0, pdwAudioBytes2);
      
      IDirectSoundBuffer_Unlock(chipBuffer, 
        ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
    }

    //Start playing
    if (mute == FALSE)
      IDirectSoundBuffer_Play(chipBuffer, 0,0, DSBPLAY_LOOPING );
  }

  if (dacBuffer)
  {
    IDirectSoundBuffer_Stop(dacBuffer);

    // Fill the sound buffer
    if SUCCEEDED(IDirectSoundBuffer_Lock(dacBuffer, 0, 0, 
      &ppvAudioPtr1, &pdwAudioBytes1, 
      &ppvAudioPtr2, &pdwAudioBytes2, DSBLOCK_ENTIREBUFFER))
    {
      if (ppvAudioPtr1 && pdwAudioBytes1)
        memset(ppvAudioPtr1, 0x80, pdwAudioBytes1);

      if (ppvAudioPtr2 && pdwAudioBytes2)
        memset(ppvAudioPtr2, 0x80, pdwAudioBytes2);
      
      IDirectSoundBuffer_Unlock(dacBuffer, 
        ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
    }

    //Start playing
    if (mute == FALSE)
      IDirectSoundBuffer_Play(dacBuffer, 0,0, DSBPLAY_LOOPING );
  }
*/
}


