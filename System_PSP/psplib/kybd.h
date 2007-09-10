/** PSP helper library ***************************************/
/**                                                         **/
/**                          kybd.h                         **/
/**                                                         **/
/** This file contains declarations for virtual keyboard    **/
/** implementation routines                                 **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PSP_KYBD_H
#define _PSP_KYBD_H

#include "ctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PspKeyboardButton
{
  char *Caption;
  unsigned short Code;
  unsigned short X;
  unsigned short Y;
  unsigned short W;
  unsigned short H;
  unsigned short TextWidth;
  unsigned short TextHeight;
  unsigned char IsSticky;
};

struct PspStickyButton
{
  unsigned short Code;
  unsigned char Status;
  unsigned short *KeyIndex;
  unsigned char IndexCount;
};

struct PspKeyboardLayout
{
  unsigned short KeyCount;
  struct PspKeyboardButton *Keys;
  unsigned short StickyCount;
  struct PspStickyButton *StickyKeys;
  unsigned short MatrixWidth;
  unsigned short MatrixHeight;
  unsigned short Selected;
  unsigned short HeldDown;
  unsigned int __attribute__((aligned(16))) CallList[262144];

  int(*ReadCallback)(unsigned int code);
  void(*WriteCallback)(unsigned int code, int status);
};

typedef struct PspKeyboardLayout PspKeyboardLayout;

void pspKybdReinit(PspKeyboardLayout *layout);
void pspKybdRender(const PspKeyboardLayout *layout);
void pspKybdNavigate(PspKeyboardLayout *layout, SceCtrlData *pad);
void pspKybdReleaseAll(PspKeyboardLayout *layout);

PspKeyboardLayout* pspKybdLoadLayout(const char *path, 
  int(*read_callback)(unsigned int), void(*write_callback)(unsigned int, int));
void pspKybdDestroyLayout(PspKeyboardLayout *layout);

#ifdef __cplusplus
}
#endif

#endif  // _PSP_KYBD_H
