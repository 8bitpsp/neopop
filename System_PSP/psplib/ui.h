/** PSP helper library ***************************************/
/**                                                         **/
/**                           ui.h                          **/
/**                                                         **/
/** This file contains declarations for a simple GUI        **/
/** rendering library                                       **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PSP_UI_H
#define _PSP_UI_H

#include "video.h"
#include "menu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PspUiMetric
{
  const PspImage *Background;
  const PspFont *Font;
  u64 CancelButton;
  u64 OkButton;
  int Left;
  int Top;
  int Right;
  int Bottom;
  u32 ScrollbarColor;
  u32 ScrollbarBgColor;
  int ScrollbarWidth;
  u32 TextColor;
  u32 SelectedColor;
  u32 SelectedBgColor;
  u32 StatusBarColor;
  int MenuFps;

  u32 DialogFogColor;

  u32 BrowserFileColor;
  u32 BrowserDirectoryColor;

  int GalleryIconsPerRow;
  int GalleryIconMarginWidth;

  int MenuItemMargin;
  u32 MenuSelOptionBg;
  u32 MenuOptionBoxColor;
  u32 MenuOptionBoxBg;
  u32 MenuDecorColor;

  int TitlePadding;
  u32 TitleColor;
  u32 TabBgColor;
  int Animate;
} PspUiMetric;

typedef struct PspUiFileBrowser
{
  void (*OnRender)(const void *browser, const void *path);
  int  (*OnOk)(const void *browser, const void *file);
  int  (*OnCancel)(const void *gallery, const void *parent_dir);
  int  (*OnButtonPress)(const struct PspUiFileBrowser* browser, 
         const char *selected, u32 button_mask);
  const char **Filter;
  void *Userdata;
} PspUiFileBrowser;

typedef struct PspUiMenu
{
  PspMenu *Menu;
  void (*OnRender)(const void *uimenu, const void *item);
  int  (*OnOk)(const void *menu, const void *item);
  int  (*OnCancel)(const void *menu, const void *item);
  int  (*OnButtonPress)(const struct PspUiMenu *menu, PspMenuItem* item, 
         u32 button_mask);
  int  (*OnItemChanged)(const struct PspUiMenu *menu, PspMenuItem* item, 
         const PspMenuOption* option);
} PspUiMenu;

typedef struct PspUiGallery
{
  PspMenu *Menu;
  void (*OnRender)(const void *gallery, const void *item);
  int  (*OnOk)(const void *gallery, const void *item);
  int  (*OnCancel)(const void *gallery, const void *item);
  int  (*OnButtonPress)(const struct PspUiGallery *gallery, PspMenuItem* item, 
          u32 button_mask);
  void *Userdata;
} PspUiGallery;

typedef struct PspUiSplash
{
  void (*OnRender)(const void *splash, const void *null);
  int  (*OnCancel)(const void *splash, const void *null);
  int  (*OnButtonPress)(const struct PspUiSplash *splash, u32 button_mask);
  const char* (*OnGetStatusBarText)(const struct PspUiSplash *splash);
} PspUiSplash;

#define PSP_UI_YES     2
#define PSP_UI_NO      1
#define PSP_UI_CANCEL  0

#define PSP_UI_CONFIRM 1

char pspUiGetButtonIcon(u32 button_mask);

void pspUiOpenBrowser(PspUiFileBrowser *browser, const char *start_path);
void pspUiOpenGallery(const PspUiGallery *gallery, const char *title);
void pspUiOpenMenu(const PspUiMenu *uimenu, const char *title);
void pspUiSplashScreen(PspUiSplash *splash);

int  pspUiConfirm(const char *message);
int  pspUiYesNoCancel(const char *message);
void pspUiAlert(const char *message);
void pspUiFlashMessage(const char *message);
const PspMenuItem* pspUiSelect(const char *title, const PspMenu *menu);

void pspUiFadeout();

PspUiMetric UiMetric;

#ifdef __cplusplus
}
#endif

#endif  // _PSP_UI_H
