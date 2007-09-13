#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>

#include "psp.h"
#include "image.h"
#include "video.h"
#include "ctrl.h"
#include "ui.h"
#include "init.h"
#include "util.h"
#include "fileio.h"

#include "neopop.h"
#include "system_rom.h"

#include "emumenu.h"
#include "emulate.h"

#define TAB_QUICKLOAD 0
#define TAB_STATE     9999
#define TAB_CONTROL   1
#define TAB_OPTION    2
#define TAB_SYSTEM    4
#define TAB_ABOUT     5
#define TAB_MAX       TAB_SYSTEM

#define OPTION_DISPLAY_MODE 1
#define OPTION_SYNC_FREQ    2
#define OPTION_FRAMESKIP    3
#define OPTION_VSYNC        4
#define OPTION_CLOCK_FREQ   5
#define OPTION_SHOW_FPS     6
#define OPTION_CONTROL_MODE 7
#define OPTION_ANIMATE      8

#define SYSTEM_SCRNSHOT     1
#define SYSTEM_RESET        2
#define SYSTEM_AUDIO				3

extern PspImage *Screen;

static const char *QuickloadFilter[] = { "NGP", '\0' },
	*ScreenshotDir = "screens",
	*SaveStateDir = "savedata",
	*ButtonConfigFile = "buttons",
	*OptionsFile = "neopop.ini",
	*TabLabel[] = { "Game", "Control", "Options", "System", "About" },
  ControlHelpText[] = "\026\250\020 Change mapping\t\026\001\020 Save to \271\t\026\243\020 Load defaults",
  PresentSlotText[] = "\026\244\020 Save\t\026\001\020 Load\t\026\243\020 Delete",
  EmptySlotText[] = "\026\244\020 Save";

char *GameName;
char *ScreenshotPath;
static char *SaveStatePath;
static char *GamePath;

static int TabIndex;
static int ResumeEmulation;
static PspImage *Background;
static PspImage *NoSaveIcon;

EmulatorOptions Options;

/* Define various menu options */
static const PspMenuOptionDef
  ToggleOptions[] = {
    MENU_OPTION("Disabled", 0),
    MENU_OPTION("Enabled",  1),
    MENU_END_OPTIONS
  },
  ScreenSizeOptions[] = {
    MENU_OPTION("Actual size",              DISPLAY_MODE_UNSCALED),
    MENU_OPTION("4:3 scaled (fit height)",  DISPLAY_MODE_FIT_HEIGHT),
    MENU_OPTION("16:9 scaled (fit screen)", DISPLAY_MODE_FILL_SCREEN),
    MENU_END_OPTIONS
  },
  FrameLimitOptions[] = {
    MENU_OPTION("Disabled",      0),
    MENU_OPTION("60 fps (NTSC)", 60),
    MENU_END_OPTIONS
  },
  FrameSkipOptions[] = {
    MENU_OPTION("No skipping",  0),
    MENU_OPTION("Skip 1 frame", 1),
    MENU_OPTION("Skip 2 frames",2),
    MENU_OPTION("Skip 3 frames",3),
    MENU_OPTION("Skip 4 frames",4),
    MENU_OPTION("Skip 5 frames",5),
    MENU_OPTION("Skip 6 frames",6),
    MENU_END_OPTIONS
  },
  PspClockFreqOptions[] = {
    MENU_OPTION("222 MHz", 222),
    MENU_OPTION("266 MHz", 266),
    MENU_OPTION("300 MHz", 300),
    MENU_OPTION("333 MHz", 333),
    MENU_END_OPTIONS
  },
  ControlModeOptions[] = {
    MENU_OPTION("\026\242\020 cancels, \026\241\020 confirms (US)",    0),
    MENU_OPTION("\026\241\020 cancels, \026\242\020 confirms (Japan)", 1),
    MENU_END_OPTIONS
  },
  ButtonMapOptions[] = {
    /* Unmapped */
    MENU_OPTION("None", 0),
    /* Buttons */
    MENU_OPTION("Up",      JOY|0x01),
    MENU_OPTION("Down",    JOY|0x02),
    MENU_OPTION("Left",    JOY|0x04),
    MENU_OPTION("Right",   JOY|0x08),
    MENU_OPTION("Button A",JOY|0x10),
    MENU_OPTION("Button B",JOY|0x20),
    MENU_OPTION("Option",  JOY|0x40),
    MENU_END_OPTIONS
  };

static const PspMenuItemDef
  OptionMenuDef[] = {
    MENU_HEADER("Video"),
    MENU_ITEM("Screen size", OPTION_DISPLAY_MODE, ScreenSizeOptions, -1,
      "\026\250\020 Change screen size"),
    MENU_HEADER("Performance"),
    MENU_ITEM("Frame limiter", OPTION_SYNC_FREQ, FrameLimitOptions, -1, 
      "\026\250\020 Change screen update frequency"),
    MENU_ITEM("Frame skipping", OPTION_FRAMESKIP, FrameSkipOptions, -1, 
      "\026\250\020 Change number of frames skipped per update"),
    MENU_ITEM("VSync", OPTION_VSYNC, ToggleOptions, -1,
      "\026\250\020 Enable to reduce tearing; disable to increase speed"),
    MENU_ITEM("PSP clock frequency", OPTION_CLOCK_FREQ, PspClockFreqOptions, -1,
      "\026\250\020 Larger values: faster emulation, faster battery depletion (default: 222MHz)"),
    MENU_ITEM("Show FPS counter",    OPTION_SHOW_FPS, ToggleOptions, -1,
      "\026\250\020 Show/hide the frames-per-second counter"),
    MENU_HEADER("Menu"),
    MENU_ITEM("Button mode", OPTION_CONTROL_MODE, ControlModeOptions,  -1, 
      "\026\250\020 Change OK and Cancel button mapping"),
    MENU_ITEM("Animate",    OPTION_ANIMATE, ToggleOptions, -1,
      "\026\250\020 Enable/disable in-menu animations"),
    MENU_END_ITEMS
  },
  ControlMenuDef[] = {
    MENU_ITEM(PSP_CHAR_ANALUP, MAP_ANALOG_UP, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_ANALDOWN, MAP_ANALOG_DOWN, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_ANALLEFT, MAP_ANALOG_LEFT, ButtonMapOptions, -1,
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_ANALRIGHT, MAP_ANALOG_RIGHT, ButtonMapOptions, -1,
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_UP, MAP_BUTTON_UP, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_DOWN, MAP_BUTTON_DOWN, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_LEFT, MAP_BUTTON_LEFT, ButtonMapOptions, -1,
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_RIGHT, MAP_BUTTON_RIGHT, ButtonMapOptions, -1,
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_SQUARE, MAP_BUTTON_SQUARE, ButtonMapOptions, -1,
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_CROSS, MAP_BUTTON_CROSS, ButtonMapOptions, -1,
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_CIRCLE, MAP_BUTTON_CIRCLE, ButtonMapOptions, -1,
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_TRIANGLE, MAP_BUTTON_TRIANGLE, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_LTRIGGER, MAP_BUTTON_LTRIGGER, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_RTRIGGER, MAP_BUTTON_RTRIGGER, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_SELECT, MAP_BUTTON_SELECT, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_START, MAP_BUTTON_START, ButtonMapOptions, -1, 
      ControlHelpText),
    MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_RTRIGGER, 
      MAP_BUTTON_LRTRIGGERS, ButtonMapOptions, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_START"+"PSP_CHAR_SELECT,
      MAP_BUTTON_STARTSELECT, ButtonMapOptions, -1, ControlHelpText),
    MENU_END_ITEMS
  },
  SystemMenuDef[] = {
    MENU_HEADER("Audio"),
    MENU_ITEM("Sound", SYSTEM_AUDIO, ToggleOptions, -1, 
      "\026\001\020 Enable/disable sound"),
    MENU_HEADER("System"),
    MENU_ITEM("Reset", SYSTEM_RESET, NULL, -1, "\026\001\020 Reset"),
    MENU_ITEM("Save screenshot",  SYSTEM_SCRNSHOT, NULL, -1, 
      "\026\001\020 Save screenshot"),
    MENU_END_ITEMS
  };

static void LoadOptions();
static int SaveOptions();

static void InitButtonConfig();
static int  SaveButtonConfig();
static int  LoadButtonConfig();

static int OnSplashButtonPress(const struct PspUiSplash *splash, 
  u32 button_mask);
static void OnSplashRender(const void *uiobject, const void *null);

static int OnGenericCancel(const void *uiobject, const void *param);
static void OnGenericRender(const void *uiobject, const void *item_obj);
static int OnGenericButtonPress(const PspUiFileBrowser *browser, 
  const char *path, u32 button_mask);

int OnQuickloadOk(const void *browser, const void *path);

static int OnMenuItemChanged(const struct PspUiMenu *uimenu, PspMenuItem* item, 
  const PspMenuOption* option);
static int OnMenuOk(const void *uimenu, const void* sel_item);
static int OnMenuButtonPress(const struct PspUiMenu *uimenu, 
  PspMenuItem* sel_item, u32 button_mask);

void OnSystemRender(const void *uiobject, const void *item_obj);

PspUiSplash SplashScreen =
{
  OnSplashRender,
  OnGenericCancel,
  OnSplashButtonPress,
  NULL
};

PspUiFileBrowser QuickloadBrowser = 
{
  OnGenericRender,
  OnQuickloadOk,
  OnGenericCancel,
  OnGenericButtonPress,
  QuickloadFilter,
  0
};

PspUiMenu ControlUiMenu =
{
  NULL,                  /* PspMenu */
  OnGenericRender,       /* OnRender() */
  OnMenuOk,              /* OnOk() */
  OnGenericCancel,       /* OnCancel() */
  OnMenuButtonPress,     /* OnButtonPress() */
  OnMenuItemChanged,     /* OnItemChanged() */
};

PspUiMenu OptionUiMenu =
{
  NULL,                  /* PspMenu */
  OnGenericRender,       /* OnRender() */
  OnMenuOk,              /* OnOk() */
  OnGenericCancel,       /* OnCancel() */
  OnMenuButtonPress,     /* OnButtonPress() */
  OnMenuItemChanged,     /* OnItemChanged() */
};

PspUiMenu SystemUiMenu =
{
  NULL,                  /* PspMenu */
  OnSystemRender,        /* OnRender() */
  OnMenuOk,              /* OnOk() */
  OnGenericCancel,       /* OnCancel() */
  OnMenuButtonPress,     /* OnButtonPress() */
  OnMenuItemChanged,     /* OnItemChanged() */
};

/* Game configuration (includes button maps) */
struct ButtonConfig ActiveConfig;

/* Default configuration */
struct ButtonConfig DefaultConfig =
{
  {
    JOY|0x01, /* Analog Up    */
    JOY|0x02, /* Analog Down  */
    JOY|0x04, /* Analog Left  */
    JOY|0x08, /* Analog Right */
    JOY|0x01, /* D-pad Up     */
    JOY|0x02, /* D-pad Down   */
    JOY|0x04, /* D-pad Left   */
    JOY|0x08, /* D-pad Right  */
    JOY|0x20, /* Square       */
    JOY|0x10, /* Cross        */
    0,        /* Circle       */
    0,        /* Triangle     */
    0,        /* L Trigger    */
    0,        /* R Trigger    */
    JOY|0x40, /* Select       */
    0,        /* Start        */
    SPC|SPC_MENU, /* L+R Triggers */
    0,            /* Start+Select */
  }
};

/* Button masks */
const u64 ButtonMask[] = 
{
  PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER, 
  PSP_CTRL_START    | PSP_CTRL_SELECT,
  PSP_CTRL_ANALUP,    PSP_CTRL_ANALDOWN,
  PSP_CTRL_ANALLEFT,  PSP_CTRL_ANALRIGHT,
  PSP_CTRL_UP,        PSP_CTRL_DOWN,
  PSP_CTRL_LEFT,      PSP_CTRL_RIGHT,
  PSP_CTRL_SQUARE,    PSP_CTRL_CROSS,
  PSP_CTRL_CIRCLE,    PSP_CTRL_TRIANGLE,
  PSP_CTRL_LTRIGGER,  PSP_CTRL_RTRIGGER,
  PSP_CTRL_SELECT,    PSP_CTRL_START,
  0 /* End */
};

/* Button map ID's */
const int ButtonMapId[] = 
{
  MAP_BUTTON_LRTRIGGERS, 
  MAP_BUTTON_STARTSELECT,
  MAP_ANALOG_UP,       MAP_ANALOG_DOWN,
  MAP_ANALOG_LEFT,     MAP_ANALOG_RIGHT,
  MAP_BUTTON_UP,       MAP_BUTTON_DOWN,
  MAP_BUTTON_LEFT,     MAP_BUTTON_RIGHT,
  MAP_BUTTON_SQUARE,   MAP_BUTTON_CROSS,
  MAP_BUTTON_CIRCLE,   MAP_BUTTON_TRIANGLE,
  MAP_BUTTON_LTRIGGER, MAP_BUTTON_RTRIGGER,
  MAP_BUTTON_SELECT,   MAP_BUTTON_START,
  -1 /* End */
};

int InitMenu()
{
  /* Reset variables */
  TabIndex = TAB_ABOUT;
  Background = NULL;
  GameName = NULL;
  GamePath = NULL;

  /* Initialize options */
  LoadOptions();

  /* Initialize emulation engine */
  if (!InitEmulation())
    return 0;

  /* Load the background image */
  Background = pspImageLoadPng("background.png");

  /* Init NoSaveState icon image */
  NoSaveIcon=pspImageCreate(136, 114, PSP_IMAGE_16BPP);
  pspImageClear(NoSaveIcon, RGB(0x0c,0,0x3f));

  /* Initialize control menu */
  ControlUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(ControlUiMenu.Menu, ControlMenuDef);

  /* Initialize options menu */
  OptionUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(OptionUiMenu.Menu, OptionMenuDef);

  /* Initialize system menu */
  SystemUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(SystemUiMenu.Menu, SystemMenuDef);

  /* Initialize paths */
  SaveStatePath = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) 
    + strlen(SaveStateDir) + 2));
  sprintf(SaveStatePath, "%s%s/", pspGetAppDirectory(), SaveStateDir);
  ScreenshotPath = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) 
    + strlen(ScreenshotDir) + 2));
  sprintf(ScreenshotPath, "%s%s/", pspGetAppDirectory(), ScreenshotDir);

  /* Load default configuration */
  LoadButtonConfig();

  /* Initialize UI components */
  UiMetric.Background = Background;
  UiMetric.Font = &PspStockFont;
  UiMetric.Left = 8;
  UiMetric.Top = 24;
  UiMetric.Right = 472;
  UiMetric.Bottom = 250;
  UiMetric.OkButton = (!Options.ControlMode) ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
  UiMetric.CancelButton = (!Options.ControlMode) ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
  UiMetric.ScrollbarColor = PSP_COLOR_GRAY;
  UiMetric.ScrollbarBgColor = 0x44ffffff;
  UiMetric.ScrollbarWidth = 10;
  UiMetric.TextColor = PSP_COLOR_GRAY;
  UiMetric.SelectedColor = PSP_COLOR_YELLOW;
  UiMetric.SelectedBgColor = COLOR(0xff,0xff,0xff,0x44);
  UiMetric.StatusBarColor = PSP_COLOR_WHITE;
  UiMetric.BrowserFileColor = PSP_COLOR_GRAY;
  UiMetric.BrowserDirectoryColor = PSP_COLOR_YELLOW;
  UiMetric.GalleryIconsPerRow = 5;
  UiMetric.GalleryIconMarginWidth = 8;
  UiMetric.MenuItemMargin = 20;
  UiMetric.MenuSelOptionBg = PSP_COLOR_BLACK;
  UiMetric.MenuOptionBoxColor = PSP_COLOR_GRAY;
  UiMetric.MenuOptionBoxBg = COLOR(0, 0, 33, 0xBB);
  UiMetric.MenuDecorColor = PSP_COLOR_YELLOW;
  UiMetric.DialogFogColor = COLOR(0, 0, 0, 88);
  UiMetric.TitlePadding = 4;
  UiMetric.TitleColor = PSP_COLOR_WHITE;
  UiMetric.MenuFps = 30;
  UiMetric.TabBgColor = COLOR(0x74,0x74,0xbe,0xff);

  return 1;
}

/* Load options */
void LoadOptions()
{
  char *path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(OptionsFile) + 1));
  sprintf(path, "%s%s", pspGetAppDirectory(), OptionsFile);

  /* Initialize INI structure */
  PspInit *init = pspInitCreate();

  /* Read the file */
  pspInitLoad(init, path);

  /* Load values */
  Options.DisplayMode = pspInitGetInt(init, "Video", "Display Mode", DISPLAY_MODE_UNSCALED);
  Options.UpdateFreq = pspInitGetInt(init, "Video", "Update Frequency", 60);
  Options.Frameskip = pspInitGetInt(init, "Video", "Frameskip", 1);
  Options.VSync = pspInitGetInt(init, "Video", "VSync", 0);
  Options.ClockFreq = pspInitGetInt(init, "Video", "PSP Clock Frequency", 222);
  Options.ShowFps = pspInitGetInt(init, "Video", "Show FPS", 0);

  Options.ControlMode = pspInitGetInt(init, "Menu", "Control Mode", 0);
  UiMetric.Animate = pspInitGetInt(init, "Menu", "Animate", 1);

  Options.SoundOn = pspInitGetInt(init, "System", "Sound", 1);

  if (GamePath) free(GamePath);
  GamePath = pspInitGetString(init, "File", "Game Path", NULL);

  /* Clean up */
  free(path);
  pspInitDestroy(init);
}

/* Save options */
int SaveOptions()
{
  char *path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) + strlen(OptionsFile) + 1));
  sprintf(path, "%s%s", pspGetAppDirectory(), OptionsFile);

  /* Initialize INI structure */
  PspInit *init = pspInitCreate();

  /* Set values */
  pspInitSetInt(init, "Video", "Display Mode", Options.DisplayMode);
  pspInitSetInt(init, "Video", "Update Frequency", Options.UpdateFreq);
  pspInitSetInt(init, "Video", "Frameskip", Options.Frameskip);
  pspInitSetInt(init, "Video", "VSync", Options.VSync);
  pspInitSetInt(init, "Video", "PSP Clock Frequency",Options.ClockFreq);
  pspInitSetInt(init, "Video", "Show FPS", Options.ShowFps);

  pspInitSetInt(init, "Menu", "Control Mode", Options.ControlMode);
  pspInitSetInt(init, "Menu", "Animate", UiMetric.Animate);

  pspInitSetInt(init, "System", "Sound", Options.SoundOn);

  if (GamePath) pspInitSetString(init, "File", "Game Path", GamePath);

  /* Save INI file */
  int status = pspInitSave(init, path);

  /* Clean up */
  pspInitDestroy(init);
  free(path);

  return status;
}

void DisplayMenu()
{
  int i;
  PspMenuItem *item;

  /* Menu loop */
  do
  {
    ResumeEmulation = 0;

    /* Set normal clock frequency */
    pspSetClockFrequency(222);
    /* Set buttons to autorepeat */
    pspCtrlSetPollingMode(PSP_CTRL_AUTOREPEAT);

    /* Display appropriate tab */
    switch (TabIndex)
    {
    case TAB_QUICKLOAD:
      pspUiOpenBrowser(&QuickloadBrowser, (GameName) ? GameName : GamePath);
      break;
    case TAB_CONTROL:
      /* Load current button mappings */
      for (item = ControlUiMenu.Menu->First, i = 0; item; item = item->Next, i++)
        pspMenuSelectOptionByValue(item, (void*)ActiveConfig.ButtonMap[i]);
      pspUiOpenMenu(&ControlUiMenu, NULL);
      break;
    case TAB_OPTION:
      /* Init menu options */
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_DISPLAY_MODE);
      pspMenuSelectOptionByValue(item, (void*)Options.DisplayMode);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_SYNC_FREQ);
      pspMenuSelectOptionByValue(item, (void*)Options.UpdateFreq);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_FRAMESKIP);
      pspMenuSelectOptionByValue(item, (void*)(int)Options.Frameskip);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_VSYNC);
      pspMenuSelectOptionByValue(item, (void*)Options.VSync);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_CLOCK_FREQ);
      pspMenuSelectOptionByValue(item, (void*)Options.ClockFreq);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_SHOW_FPS);
      pspMenuSelectOptionByValue(item, (void*)Options.ShowFps);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_CONTROL_MODE);
      pspMenuSelectOptionByValue(item, (void*)Options.ControlMode);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_ANIMATE);
      pspMenuSelectOptionByValue(item, (void*)UiMetric.Animate);
      pspUiOpenMenu(&OptionUiMenu, NULL);
      break;
    case TAB_SYSTEM:
      item = pspMenuFindItemById(OptionUiMenu.Menu, SYSTEM_AUDIO);
      pspMenuSelectOptionByValue(item, (void*)Options.SoundOn);
      pspUiOpenMenu(&SystemUiMenu, NULL);
      break;
    case TAB_ABOUT:
      pspUiSplashScreen(&SplashScreen);
      break;
    }

    if (!ExitPSP)
    {
      /* Set clock frequency during emulation */
      pspSetClockFrequency(Options.ClockFreq);
      /* Set buttons to normal mode */
      pspCtrlSetPollingMode(PSP_CTRL_NORMAL);

      /* Resume emulation */
      if (ResumeEmulation)
      {
        if (UiMetric.Animate) pspUiFadeout();
        RunEmulation();
        if (UiMetric.Animate) pspUiFadeout();
      }
    }
  } while (!ExitPSP);
}

void OnSplashRender(const void *splash, const void *null)
{
  int fh, i, x, y, height;
  const char *lines[] = 
  { 
    PSP_APP_NAME" version "PSP_APP_VER" ("__DATE__")",
    "\026http://psp.akop.org/smsplus",
    " ",
    "2007 Akop Karapetyan (port)",
    "1998-2004 Charles MacDonald (emulation)",
    NULL
  };

  fh = pspFontGetLineHeight(UiMetric.Font);

  for (i = 0; lines[i]; i++);
  height = fh * (i - 1);

  /* Render lines */
  for (i = 0, y = SCR_HEIGHT / 2 - height / 2; lines[i]; i++, y += fh)
  {
    x = SCR_WIDTH / 2 - pspFontGetTextWidth(UiMetric.Font, lines[i]) / 2;
    pspVideoPrint(UiMetric.Font, x, y, lines[i], PSP_COLOR_GRAY);
  }

  /* Render PSP status */
  OnGenericRender(splash, null);
}

int OnSplashButtonPress(const struct PspUiSplash *splash, 
  u32 button_mask)
{
  return OnGenericButtonPress(NULL, NULL, button_mask);
}

/* Handles drawing of generic items */
void OnGenericRender(const void *uiobject, const void *item_obj)
{
  /* Draw tabs */
  int i, x, width, height = pspFontGetLineHeight(UiMetric.Font);
  for (i = 0, x = 5; i <= TAB_MAX; i++, x += width + 10)
  {
    width = -10;

    if (!GameName && (i == TAB_STATE || i == TAB_SYSTEM))
      continue;

    /* Determine width of text */
    width = pspFontGetTextWidth(UiMetric.Font, TabLabel[i]);

    /* Draw background of active tab */
    if (i == TabIndex)
      pspVideoFillRect(x - 5, 0, x + width + 5, height + 1, UiMetric.TabBgColor);

    /* Draw name of tab */
    pspVideoPrint(UiMetric.Font, x, 0, TabLabel[i], PSP_COLOR_WHITE);
  }
}

int OnGenericButtonPress(const PspUiFileBrowser *browser, 
  const char *path, u32 button_mask)
{
  int tab_index;

  /* If L or R are pressed, switch tabs */
  if (button_mask & PSP_CTRL_LTRIGGER)
  {
    TabIndex--;
    do
    {
      tab_index = TabIndex;
      if (!GameName && (TabIndex == TAB_STATE || TabIndex == TAB_SYSTEM)) TabIndex--;
      if (TabIndex < 0) TabIndex = TAB_MAX;
    } while (tab_index != TabIndex);
  }
  else if (button_mask & PSP_CTRL_RTRIGGER)
  {
    TabIndex++;
    do
    {
      tab_index = TabIndex;
      if (!GameName && (TabIndex == TAB_STATE || TabIndex == TAB_SYSTEM)) TabIndex++;
      if (TabIndex > TAB_MAX) TabIndex = 0;
    } while (tab_index != TabIndex);
  }
  else if ((button_mask & (PSP_CTRL_START | PSP_CTRL_SELECT)) 
    == (PSP_CTRL_START | PSP_CTRL_SELECT))
  {
    if (pspUtilSaveVramSeq(ScreenshotPath, "ui"))
      pspUiAlert("Saved successfully");
    else
      pspUiAlert("ERROR: Not saved");
    return 0;
  }
  else return 0;

  return 1;
}

int OnGenericCancel(const void *uiobject, const void* param)
{
  if (GameName) ResumeEmulation = 1;
  return 1;
}

int OnQuickloadOk(const void *browser, const void *path)
{
  if (!system_load_rom((char*)path))
  {
    pspUiAlert("Error loading cartridge");
    return 0;
  }

  if (GameName) free(GameName);
  GameName = strdup(path);

  if (GamePath) free(GamePath);
  GamePath = pspFileIoGetParentDirectory(GameName);

  reset();

  ResumeEmulation = 1;
  return 1;
}

int OnMenuItemChanged(const struct PspUiMenu *uimenu, PspMenuItem* item, 
  const PspMenuOption* option)
{
  if (uimenu == &ControlUiMenu)
  {
    unsigned int value = (unsigned int)option->Value;
    ActiveConfig.ButtonMap[item->ID] = value;
  }
  else if (uimenu == &SystemUiMenu)
  {
    Options.SoundOn = (int)option->Value;
  }
  else if (uimenu == &OptionUiMenu)
  {
    int value = (int)option->Value;
    switch(item->ID)
    {
    case OPTION_DISPLAY_MODE:
      Options.DisplayMode = value; break;
    case OPTION_SYNC_FREQ:
      Options.UpdateFreq = value; break;
    case OPTION_FRAMESKIP:
      Options.Frameskip = value; break;
    case OPTION_VSYNC:
      Options.VSync = value; break;
    case OPTION_CLOCK_FREQ:
      Options.ClockFreq = value; break;
    case OPTION_SHOW_FPS:
      Options.ShowFps = value; break;
      break;
    case OPTION_CONTROL_MODE:
      Options.ControlMode = value;
      UiMetric.OkButton = (!value) ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
      UiMetric.CancelButton = (!value) ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
      break;
    case OPTION_ANIMATE:
      UiMetric.Animate = value; break;
    }
  }

  return 1;
}

int OnMenuOk(const void *uimenu, const void* sel_item)
{
  if (uimenu == &ControlUiMenu)
  {
    /* Save to MS */
    if (SaveButtonConfig())
      pspUiAlert("Changes saved");
    else
      pspUiAlert("ERROR: Changes not saved");
  }
  else if (uimenu == &SystemUiMenu)
  {
    switch (((const PspMenuItem*)sel_item)->ID)
    {
    case SYSTEM_RESET:

      /* Reset system */
      if (pspUiConfirm("Reset the system?"))
      {
        ResumeEmulation = 1;
        reset();
        return 1;
      }
      break;

    case SYSTEM_SCRNSHOT:

      /* Save screenshot */
      if (!pspUtilSavePngSeq(ScreenshotPath, pspFileIoGetFilename(GameName), Screen))
        pspUiAlert("ERROR: Screenshot not saved");
      else
        pspUiAlert("Screenshot saved successfully");
      break;
    }
  }

  return 0;
}

int OnMenuButtonPress(const struct PspUiMenu *uimenu, PspMenuItem* sel_item, 
  u32 button_mask)
{
  if (uimenu == &ControlUiMenu)
  {
    if (button_mask & PSP_CTRL_TRIANGLE)
    {
      PspMenuItem *item;
      int i;

      /* Load default mapping */
      InitButtonConfig();

      /* Modify the menu */
      for (item = ControlUiMenu.Menu->First, i = 0; item; item = item->Next, i++)
        pspMenuSelectOptionByValue(item, (void*)DefaultConfig.ButtonMap[i]);

      return 0;
    }
  }

  return OnGenericButtonPress(NULL, NULL, button_mask);
}

/* Initialize game configuration */
static void InitButtonConfig()
{
  memcpy(&ActiveConfig, &DefaultConfig, sizeof(struct ButtonConfig));
}

/* Load game configuration */
static int LoadButtonConfig()
{
  char *path;
  if (!(path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) 
    + strlen(ButtonConfigFile) + 6)))) return 0;
  sprintf(path, "%s%s.cnf", pspGetAppDirectory(), ButtonConfigFile);

  /* Open file for reading */
  FILE *file = fopen(path, "r");
  free(path);

  /* If no configuration, load defaults */
  if (!file)
  {
    InitButtonConfig();
    return 1;
  }

  /* Read contents of struct */
  int nread = fread(&ActiveConfig, sizeof(struct ButtonConfig), 1, file);
  fclose(file);

  if (nread != 1)
  {
    InitButtonConfig();
    return 0;
  }

  return 1;
}

/* Save game configuration */
static int SaveButtonConfig()
{
  char *path;
  if (!(path = (char*)malloc(sizeof(char) * (strlen(pspGetAppDirectory()) 
    + strlen(ButtonConfigFile) + 6)))) return 0;
  sprintf(path, "%s%s.cnf", pspGetAppDirectory(), ButtonConfigFile);

  /* Open file for writing */
  FILE *file = fopen(path, "w");
  free(path);
  if (!file) return 0;

  /* Write contents of struct */
  int nwritten = fwrite(&ActiveConfig, sizeof(struct ButtonConfig), 1, file);
  fclose(file);

  return (nwritten == 1);
}

/* Handles any special drawing for the system menu */
void OnSystemRender(const void *uiobject, const void *item_obj)
{
  int w, h, x, y;
  w = Screen->Viewport.Width;
  h = Screen->Viewport.Height;
  x = SCR_WIDTH - w - 8;
  y = SCR_HEIGHT - h - 80;

  /* Draw a small representation of the screen */
  pspVideoShadowRect(x, y, x + w - 1, y + h - 1, PSP_COLOR_BLACK, 3);
  pspVideoPutImage(Screen, x, y, w, h);
  pspVideoDrawRect(x, y, x + w - 1, y + h - 1, PSP_COLOR_GRAY);

  OnGenericRender(uiobject, item_obj);
}

void TrashMenu()
{
  /* Free emulation-specific resources */
  TrashEmulation();

  /* Save options */
  SaveOptions();

 /* Free local resources */
  pspImageDestroy(Background);
  pspImageDestroy(NoSaveIcon);

  pspMenuDestroy(ControlUiMenu.Menu);
  pspMenuDestroy(OptionUiMenu.Menu);
  pspMenuDestroy(SystemUiMenu.Menu);

  if (GameName) free(GameName);
  if (GamePath) free(GamePath);

  free(SaveStatePath);
  free(ScreenshotPath);
}
