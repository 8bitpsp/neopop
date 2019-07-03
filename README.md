NeoPop PSP
=========

NeoGeo Pocket and Pocket Color emulator

&copy; 2007 Akop Karapetyan  
&copy; 2002 neopop_uk

New Features
------------

#### Version 0.71.15 (September 30 2007)

*   Fixed crash caused by having many long filenames in the same directory

Installation
------------

Unzip `neopop.zip` into `/PSP/GAME/` folder on the memory stick.

Game ROM’s can reside anywhere (the GAMES subdirectory is recommended, but not necessary). NeoPop PSP supports ROM loading from ZIP files.

Controls
--------

During emulation:

| PSP controls                    | Emulated controls            |
| ------------------------------- | ---------------------------- |
| Analog stick up/down/left/right | D-pad up/down/left/right     |
| D-pad up/down/left/right        | D-pad up/down/left/right     |
| [ ] (square)                    | Button A                     |
| X (cross)                       | Button B                     |
| Select                          | Option                       |
| [L] + [R]                       | Return to the emulator menu  |

By default, button configuration changes are not retained after button mapping is modified. To save changes, press X (cross) after desired mapping is configured. To load the default mapping press ^ (triangle).

Known Issues
------------

* NeoPop has been superseded by [RACE!](https://github.com/8bitpsp/race)

Compiling
---------

To compile, ensure that [zlib](svn://svn.pspdev.org/psp/trunk/zlib) and [libpng](svn://svn.pspdev.org/psp/trunk/libpng) are installed, and run make:

`make -f Makefile.psp`

Version History
---------------

#### 0.71.1 (September 18 2007)

*   Initial release

Credits
-------

_neopop\_uk_ (NeoPop)  
_PSMonkey_ (RISC fixes)
