//---------------------------------------------------------------------------
// NEOPOP : Emulator as in Dreamland
//
// Copyright (c) 2001-2002 by neopop_uk
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See also the license.txt file for
//  additional informations.
//---------------------------------------------------------------------------

/*
//---------------------------------------------------------------------------
//=========================================================================

  system_rom.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

21 JUL 2002 - neopop_uk
=======================================
- Created this file to remove some of the mess from 'system_main.c' and
to make rom loading more abstracted, primarily for ports to systems with
alternate methods of loading roms.
- Simplified 'LoadRomFile' using the 'system_io_read' function. It's now
more memory efficient too.
- Simplified 'LoadRomZip', also more efficient and the error messages
should be more obvious.
- For simplicity, I have made ZIP support optional

26 JUL 2002 - neopop_uk
=======================================
- I have made 'unload rom' do a more complete job of changing the Windows
  state - the code is incorporated. from the unload menu option.

03 AUG 2002 - neopop_uk
=======================================
- I've moved the general purpose rom management to the common code-base.

18 AUG 2002 - neopop_uk
=======================================
- Converted to used strings.

//---------------------------------------------------------------------------
*/

//=============================================================================

#define ZIPSUPPORT  //Comment this line to remove zip support

//=============================================================================

#include "neopop.h"
#include <sys/stat.h>
#include <ctype.h>

#include "system_rom.h"

//=============================================================================

#ifdef ZIPSUPPORT
#include "zLIB/unzip.h"
static BOOL LoadRomZip(char* filename);
#endif

static BOOL LoadRomFile(char* filename);

//=============================================================================

void _strlwr(char *string)
{
  char *c;
  for (c = string; *c; c++) *c = (char)tolower(*c);
}

//-----------------------------------------------------------------------------
// system_load_rom()
//-----------------------------------------------------------------------------
BOOL system_load_rom(char* filename)
{
  BOOL ret;
  char copyFNAME[256];
  int i;

  /* Kill the old rom */
  rom_unload();

  system_message("Loading %s...", filename);

  //Store File Name
  memset(rom.filename, 0, 256);
  for (i = strlen(filename); i > 0; i--)
    if (filename[i] == '/' || filename[i] == '\\')
      break;
  strncpy(rom.filename, filename + i + 1, strlen(filename) - i - 5);

  //Load the file

#ifdef ZIPSUPPORT
  strcpy(copyFNAME, filename);
  _strlwr(copyFNAME);
  if (strstr(copyFNAME, ".zip"))
    ret = LoadRomZip(copyFNAME);    // Load Zip
  else
#endif

  ret = LoadRomFile(filename);  // Load raw file

  //Success?
  if (ret)
  {
    system_message("OK\n");
    rom_loaded();      //Perform independent actions
    return TRUE;
  }
  else
  {
    system_message("Error\n");
    system_unload_rom();
    return FALSE;
  }
}

//-----------------------------------------------------------------------------
// system_unload_rom()
//-----------------------------------------------------------------------------
void system_unload_rom(void)
{
  rom_unload();
}

//=============================================================================

static BOOL LoadRomFile(char* filename)
{
  struct stat statbuffer;

  //Get file length
  if (stat(filename, &statbuffer) == -1)
  {
    system_message("%s\n%s", system_get_string(IDS_EROMFIND), filename);
    return FALSE;
  }

  rom.length = statbuffer.st_size;
  rom.data = (_u8*)calloc(rom.length, sizeof(_u8));
    
  if (system_io_rom_read(filename, rom.data, rom.length))
  {
    //Success!
    return TRUE;
  }
  else
  {
    //Failed.
    system_message("%s\n%s", system_get_string(IDS_EROMOPEN), filename);
    return FALSE;
  }
}

//=============================================================================

#ifdef ZIPSUPPORT
static BOOL LoadRomZip(char* filename)
{
  unzFile zip = NULL;
  char currentZipFileName[256];
  unz_file_info fileInfo;

  if ((zip = unzOpen(filename)) == NULL)
  {
    system_message("%s\n%s", system_get_string(IDS_EZIPFIND), filename);
    return FALSE;
  }

  //Scan for the file list
  if (unzGoToFirstFile(zip) != UNZ_OK)
  {
    unzClose(zip);
    system_message("%s\n%s", system_get_string(IDS_EZIPBAD), filename);
    return FALSE;
  }
    
  while (unzGetCurrentFileInfo(zip, &fileInfo, currentZipFileName, 
      256, NULL, 0, NULL, 0) == UNZ_OK)
  {
    _strlwr(currentZipFileName);

    //Find a rom with the correct extension
    if (strstr(currentZipFileName, ".ngp") == NULL &&
      strstr(currentZipFileName, ".ngc") == NULL &&
      strstr(currentZipFileName, ".npc") == NULL)
    {
      //Last one?
      if (unzGoToNextFile(zip) == UNZ_END_OF_LIST_OF_FILE)
        break;
      else
        continue;  //Try the next...
    }

    //Extract It
    rom.length = fileInfo.uncompressed_size;

    //Open the file
    if(unzOpenCurrentFile(zip) == UNZ_OK)
    {
      rom.length = fileInfo.uncompressed_size;

      //Reserve the space required
      rom.data = (_u8*)calloc(rom.length, 1);
                  
      //Read the File
      if(unzReadCurrentFile(zip, rom.data, rom.length) >= 0)
      {
        //Load complete
        unzCloseCurrentFile(zip);
        unzClose(zip);
        return TRUE;  //success!
      }
      else
      {
        system_message("%s\n%s", system_get_string(IDS_EZIPBAD), filename);
        unzCloseCurrentFile(zip);
        unzClose(zip);
        return FALSE;
      }
    }
    else
    {
      system_message("%s\n%s", system_get_string(IDS_EZIPBAD), filename);
      unzClose(zip);
      return FALSE;
    }
  }

  unzClose(zip);
  system_message("%s\n%s", system_get_string(IDS_EZIPNONE), filename);
  return FALSE;
}
#endif

//=============================================================================

