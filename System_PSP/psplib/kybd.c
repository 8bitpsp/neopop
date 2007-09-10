/** PSP helper library ***************************************/
/**                                                         **/
/**                          kybd.c                         **/
/**                                                         **/
/** This file contains virtual keyboard implementation      **/
/** routines                                                **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include <time.h>
#include <psptypes.h>
#include <psprtc.h>
#include <malloc.h>
#include <pspgu.h>
#include <string.h>
#include <pspkernel.h>

#include "video.h"
#include "font.h"
#include "kybd.h"

#define PSP_KYBD_BUTTONS   6
#define PSP_KYBD_DELAY     400
#define PSP_KYBD_THRESHOLD 50

#define PSP_KYBD_BUTTON_BG        COLOR(0, 0, 0, 0x88)
#define PSP_KYBD_BUTTON_BORDER    PSP_COLOR_WHITE
#define PSP_KYBD_FONT_COLOR       COLOR(0xFF, 0xFF, 0xFF, 0xFF)
#define PSP_KYBD_STUCK_COLOR      COLOR(0xFF, 0, 0, 0x33)
#define PSP_KYBD_SELECTED_COLOR   COLOR(0xFF, 0xFF, 0, 0x88)

static u64 PushTime[PSP_KYBD_BUTTONS];
static const int ButtonMap[PSP_KYBD_BUTTONS] = 
{
  PSP_CTRL_UP,
  PSP_CTRL_DOWN,
  PSP_CTRL_LEFT,
  PSP_CTRL_RIGHT,
  PSP_CTRL_CIRCLE,
  PSP_CTRL_TRIANGLE
};

void _pspKybdFilterRepeats(SceCtrlData *pad);
void _pspKybdRenderKeyboard(PspKeyboardLayout *layout);

void pspKybdReinit(PspKeyboardLayout *layout)
{
  int i;
  for (i = 0; i < PSP_KYBD_BUTTONS; i++) PushTime[i] = 0;
  layout->HeldDown = 0;

  for (i = 0; i < layout->StickyCount; i++)
    if (layout->ReadCallback)
      layout->StickyKeys[i].Status = 
        layout->ReadCallback(layout->StickyKeys[i].Code);
}

void pspKybdRender(const PspKeyboardLayout *layout)
{
  int off_x, off_y, i, j, fh;
  const struct PspKeyboardButton *button;

  /* Render the virtual keyboard */
  pspVideoCallList(layout->CallList);

  fh = pspFontGetLineHeight(&PspStockFont);
  off_x = SCR_WIDTH / 2 - layout->MatrixWidth / 2;
  off_y = SCR_HEIGHT / 2 - layout->MatrixHeight / 2;

  /* Highlight sticky buttons */
  for (i = 0; i < layout->StickyCount; i++)
  {
    if (layout->StickyKeys[i].Status)
    {
      for (j = 0; j < layout->StickyKeys[i].IndexCount; j++)
      {
        button = &(layout->Keys[layout->StickyKeys[i].KeyIndex[j]]);
        pspVideoFillRect(off_x + button->X + 1, off_y + button->Y + 1, 
          off_x + button->X + button->W, off_y + button->Y + button->H,
          PSP_KYBD_STUCK_COLOR);

        pspVideoPrint(&PspStockFont, (off_x + button->X + button->W / 2) - button->TextWidth / 2, 
          (off_y + button->Y + button->H / 2) - button->TextHeight / 2,
          button->Caption, PSP_KYBD_FONT_COLOR);
      }
    }
  }

  /* Highlight selected button */
  button = &(layout->Keys[layout->Selected]);
  pspVideoFillRect(off_x + button->X + 1, 
    off_y + button->Y + 1, 
    off_x + button->X + button->W, 
    off_y + button->Y + button->H,
    PSP_KYBD_SELECTED_COLOR);
  pspVideoPrint(&PspStockFont, (off_x + button->X + button->W / 2) - button->TextWidth / 2, 
    (off_y + button->Y + button->H / 2) - button->TextHeight / 2,
    button->Caption, PSP_KYBD_FONT_COLOR);
}

void _pspKybdFilterRepeats(SceCtrlData *pad)
{
  SceCtrlData p = *pad;
  int i;
  u64 tick;
  u32 tick_res;

  /* Get current tick count */
  sceRtcGetCurrentTick(&tick);
  tick_res = sceRtcGetTickResolution();

  /* Check each button */
  for (i = 0; i < PSP_KYBD_BUTTONS; i++)
  {
    if (p.Buttons & ButtonMap[i])
    {
      if (!PushTime[i] || tick >= PushTime[i])
      {
        /* Button was pushed for the first time, or time to repeat */
        pad->Buttons |= ButtonMap[i];
        /* Compute next press time */
        PushTime[i] = tick + ((PushTime[i]) ? PSP_KYBD_THRESHOLD : PSP_KYBD_DELAY)
          * (tick_res / 1000);
      }
      else
      {
        /* No need to repeat yet */
        pad->Buttons &= ~ButtonMap[i];
      }
    }
    else
    {
      /* Button was released */
      pad->Buttons &= ~ButtonMap[i];
      PushTime[i] = 0;        
    }
  }
}

void pspKybdReleaseAll(PspKeyboardLayout *layout)
{
  /* Release 'held down' key */
  if (layout->HeldDown && layout->WriteCallback)
  {
    layout->WriteCallback(layout->HeldDown, 0);
    layout->HeldDown = 0;
  }

  /* Unset all sticky keys */
  int i;
  for (i = 0; i < layout->StickyCount; i++)
  {
    layout->StickyKeys[i].Status = 0;
    if (layout->WriteCallback) 
      layout->WriteCallback(layout->StickyKeys[i].Code, 0);
  }
}

void pspKybdNavigate(PspKeyboardLayout *layout, SceCtrlData *pad)
{
  int i;

  _pspKybdFilterRepeats(pad);

  if ((pad->Buttons & PSP_CTRL_SQUARE)
    && layout->WriteCallback && !layout->HeldDown)
  {
    /* Button pressed */
    layout->HeldDown = layout->Keys[layout->Selected].Code;
    layout->WriteCallback(layout->HeldDown, 1);

    /* Unstick if the key is stuck */
    if (layout->Keys[layout->Selected].IsSticky)
    {
      for (i = 0; i < layout->StickyCount; i++)
      {
        /* Active sticky key; toggle status */
        if (layout->StickyKeys[i].Status && 
          layout->Keys[layout->Selected].Code == layout->StickyKeys[i].Code)
        {
          layout->StickyKeys[i].Status = !layout->StickyKeys[i].Status;
          break;
        }
      }
    }
  }
  else if (!(pad->Buttons & PSP_CTRL_SQUARE) 
    && layout->WriteCallback && layout->HeldDown)
  {
    /* Button released */
    layout->WriteCallback(layout->HeldDown, 0);
    layout->HeldDown = 0;
  }

  if (pad->Buttons & PSP_CTRL_RIGHT)
  {
    if (layout->Selected + 1 < layout->KeyCount
      && layout->Keys[layout->Selected].Y == layout->Keys[layout->Selected + 1].Y)
        layout->Selected++;
  }
  else if (pad->Buttons & PSP_CTRL_LEFT)
  {
    if (layout->Selected > 0 
      && layout->Keys[layout->Selected].Y == layout->Keys[layout->Selected - 1].Y)
        layout->Selected--; 
  }
  else if (pad->Buttons & PSP_CTRL_DOWN)
  {
    /* Find first button on the next row */
    for (i = layout->Selected + 1; i < layout->KeyCount && layout->Keys[i].Y == layout->Keys[layout->Selected].Y; i++);

    if (i < layout->KeyCount)
    {
      /* Find button below the current one */
      int r = i;
      for (; i < layout->KeyCount && layout->Keys[i].Y == layout->Keys[r].Y; i++)
        if (layout->Keys[i].X + layout->Keys[i].W / 2 >= layout->Keys[layout->Selected].X + layout->Keys[layout->Selected].W / 2) 
          break;

      layout->Selected = (i < layout->KeyCount && layout->Keys[r].Y == layout->Keys[i].Y) ? i : i - 1;
    }
  }
  else if (pad->Buttons & PSP_CTRL_UP)
  {
    /* Find first button on the previous row */
    for (i = layout->Selected - 1; i >= 0 && layout->Keys[i].Y == layout->Keys[layout->Selected].Y; i--);

    if (i >= 0)
    {
      /* Find button above the current one */
      int r = i;
      for (; i >= 0 && layout->Keys[i].Y == layout->Keys[r].Y; i--)
        if (layout->Keys[i].X + layout->Keys[i].W / 2 <= layout->Keys[layout->Selected].X + layout->Keys[layout->Selected].W / 2) 
          break;

      layout->Selected = (i >= 0 && layout->Keys[r].Y == layout->Keys[i].Y) ? i : i + 1;
    }
  }

  if (layout->WriteCallback)
  {
	  if (pad->Buttons & PSP_CTRL_CIRCLE && layout->Keys[layout->Selected].IsSticky)
	  {
	    for (i = 0; i < layout->StickyCount; i++)
	    {
	      /* Sticky key; toggle status */
	      if (layout->Keys[layout->Selected].Code == layout->StickyKeys[i].Code)
	      {
	        layout->StickyKeys[i].Status = !layout->StickyKeys[i].Status;
	        layout->WriteCallback(layout->StickyKeys[i].Code, layout->StickyKeys[i].Status);
	        break;
	      }
	    }
	  }
	  else if (pad->Buttons & PSP_CTRL_TRIANGLE)
	  {
	    /* Unset all sticky keys */
	    for (i = 0; i < layout->StickyCount; i++)
	    {
	      layout->StickyKeys[i].Status = 0;
	      layout->WriteCallback(layout->StickyKeys[i].Code, 0);
	    }
	  }
  }

  /* Unset used buttons */
  for (i = 0; i < PSP_KYBD_BUTTONS; i++) pad->Buttons &= ~ButtonMap[i];
}

void pspKybdDestroyLayout(PspKeyboardLayout *layout)
{
  int i;

  /* Destroy sticky keys */
  for (i = 0; i < layout->StickyCount; i++)
    free(layout->StickyKeys[i].KeyIndex);
  free(layout->StickyKeys);

  /* Destroy individual keys */
  for (i = 0; i < layout->KeyCount; i++)
    free(layout->Keys[i].Caption);

  /* Destroy layout loaded in pspKybdLoadLayout */
  free(layout->Keys);
  free(layout);
}

void _pspKybdRenderKeyboard(PspKeyboardLayout *layout)
{
  /* Render the virtual keyboard to a call list */
  memset(layout->CallList, 0, sizeof(layout->CallList));

  sceGuStart(GU_CALL, layout->CallList);

  const struct PspKeyboardButton *button;
  int off_x, off_y, fh;
  int sx, sy, color, i;

  fh = pspFontGetLineHeight(&PspStockFont);
  off_x = SCR_WIDTH / 2 - layout->MatrixWidth / 2;
  off_y = SCR_HEIGHT / 2 - layout->MatrixHeight / 2;

  for (i = 0; i < layout->KeyCount; i++)
  {
    button = &(layout->Keys[i]);

    /* Draw the button background */
    pspVideoFillRect(off_x + button->X, off_y + button->Y,
      off_x + button->X + button->W, off_y + button->Y + button->H,
      PSP_KYBD_BUTTON_BG);

    /* Draw the button border */
    pspVideoDrawRect(off_x + button->X, off_y + button->Y,
      off_x + button->X + button->W, off_y + button->Y + button->H,
      PSP_KYBD_BUTTON_BORDER);
    pspVideoDrawRect(off_x + button->X + 2, off_y + button->Y + 2,
      off_x + button->X + button->W - 2, off_y + button->Y + button->H - 2,
      PSP_KYBD_BUTTON_BORDER);

    /* Draw the button text */
    sx = (off_x + button->X + button->W / 2) - button->TextWidth / 2;
    sy = (off_y + button->Y + button->H / 2) - button->TextHeight / 2;
    color = PSP_KYBD_FONT_COLOR;

    pspVideoPrint(&PspStockFont, sx, sy, button->Caption, color);
    if (button->IsSticky)
      pspVideoDrawLine(sx, sy + fh, sx + button->TextWidth, sy + fh, color);
  }

  /* Print button legend */
/*
  char buttons[] = " \026\244\020 Press button\t\026\242\020 Stick button\t\026\243\020 Unstick all ";
  int txt_w = pspFontGetTextWidth(&PspStockFont, buttons);
  off_x = SCR_WIDTH / 2 - txt_w / 2;
  off_y = SCR_HEIGHT - fh;
  pspVideoFillRect(off_x, off_y, off_x + txt_w, off_y + fh,PSP_KYBD_BUTTON_BG);
  pspVideoPrint(&PspStockFont, off_x, off_y, buttons, PSP_COLOR_WHITE);
*/
  sceGuFinish();
}

PspKeyboardLayout* pspKybdLoadLayout(const char *path, 
  int(*read_callback)(unsigned int), void(*write_callback)(unsigned int, int))
{
  /* Reset physical button status */
  int i, j;
  for (i = 0; i < PSP_KYBD_BUTTONS; i++)
    PushTime[i] = 0;

  /* Create layout struct */
  PspKeyboardLayout *layout;
  if (!(layout = (PspKeyboardLayout*)malloc(sizeof(PspKeyboardLayout))))
    return NULL;

  /* Initialize */
  layout->KeyCount = layout->StickyCount = 0;
  layout->Keys = NULL;
  layout->StickyKeys = NULL;
  layout->MatrixWidth = layout->MatrixHeight = 0;
  layout->Selected = 0;
  layout->HeldDown = 0;

  /* Init callbacks */
  layout->ReadCallback = read_callback;
  layout->WriteCallback = write_callback;

  FILE *file;
  char label[16];
  struct PspKeyboardButton *button;

  /* Try opening file */
  if (!(file = fopen(path, "r")))
  {
    free(layout);
    return NULL;
  }

  /* Read the count */
  if (fscanf(file, "keys %hi\n", &(layout->KeyCount)) < 1)
  {
    free(layout);
    return NULL;
  }

  layout->Keys = (struct PspKeyboardButton*)
    malloc(sizeof(struct PspKeyboardButton) * layout->KeyCount);

  /* Parse */
  int c, count, code;
  for (i = 0; !feof(file) && i < layout->KeyCount; i++)
  {
    button = &layout->Keys[i];

    if (fgetc(file) != '>') break;
    for (j = 0; j < 15; j++)
    {
      if ((c = fgetc(file)) < 0) break;
      else if (c == '\t') { label[j] = '\0'; break; }
      else label[j] = (char)c;
    }

    /* Scan a single line */
    fscanf(file, "0x%x\t%hi\t%hi\t%hi\t%hi\n", 
      &code, &(button->X), &(button->Y), &(button->W), &(button->H));

    button->Code = code & 0xffff;
    button->Caption = strdup(label);
    button->IsSticky = 0;

    /* Adjust keymatrix bounds */
    if (button->X + button->W > layout->MatrixWidth)
      layout->MatrixWidth = button->X + button->W;
    if (button->Y + button->H > layout->MatrixHeight)
      layout->MatrixHeight = button->Y + button->H;

    /* Determine key label dimensions */
    button->TextWidth = pspFontGetTextWidth(&PspStockFont, button->Caption);
    button->TextHeight = pspFontGetTextHeight(&PspStockFont, button->Caption);
  }

  /* Read the sticky count */
  if (!feof(file))
  {
    if (fscanf(file, "sticky %hi\n", &(layout->StickyCount)) > 0 
      && layout->StickyCount > 0)
    {
      /* Initialize sticky key structure */
      layout->StickyKeys = (struct PspStickyButton*)
        malloc(sizeof(struct PspStickyButton) * layout->StickyCount);

      struct PspStickyButton *sticky;
      for (i = 0; !feof(file) && i < layout->StickyCount; i++)
      {
        sticky = &layout->StickyKeys[i];

        fscanf(file, "0x%x\t", &code);
        sticky->Code = code & 0xffff;
        sticky->Status = 0;
        sticky->IndexCount = 0;
        sticky->KeyIndex = NULL;

        for (j = 0; j < layout->KeyCount; j++)
        {
          /* Mark sticky status */
          if (layout->Keys[j].Code == sticky->Code)
            layout->Keys[j].IsSticky = 1;

          /* Determine number of matching sticky keys */
          if (layout->Keys[j].Code == sticky->Code) sticky->IndexCount++;
        }

        /* Allocate space */
        if (sticky->IndexCount > 0)
          sticky->KeyIndex = (unsigned short*)malloc(sizeof(unsigned short) 
            * sticky->IndexCount);

        /* Save indices */
        for (j = 0, count = 0; j < layout->KeyCount; j++)
          if (layout->Keys[j].Code == sticky->Code)
            sticky->KeyIndex[count++] = j;
      }
    }
  }

  /* Close the file */
  fclose(file);

  /* Render the keyboard to a display list */
  _pspKybdRenderKeyboard(layout);

  return layout;
}
