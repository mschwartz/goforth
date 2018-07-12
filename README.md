# goforth (and conquer!)

pForth ported to Odroid Go.

## Description

pForth is a portable FORTH implementation written by Phil Burk (and others?).  This is a port of pForth to the Odroid Go
ESP32 based device.

Phil originally wrote JForth for the Amiga, one of the slickest macro assemblers (masqueradeda as Forth!) ever
conceived.  Many of the niceties of JForth found their way into pForth, making it one of the more fun Forth environments
to program for.

Ported to Odroid Go by Mike Schwartz (mike@moduscreate.com)

* http://www.softsynth.com/pforth/
* https://github.com/philburk/pforth

## PForth Features
* ANS standard support for Core, Core Extensions, File-Access, Floating-Point, Locals, Programming-Tools, Strings word sets.
* Compiles from simple ANSI 'C' code with no special pre-processing needed. Also compiles under C++.
* INCLUDE reads source from normal files, not BLOCKs.
* Precompiled dictionaries can be saved and reloaded.
* Custom 'C' code can be easily linked with pForth.
* Handy words like ANEW  INCLUDE? SEE  WORDS.LIKE  FILE?
* Single Step Debugger
* Smart conditionals.  10 0 DO I . LOOP works in outer interpreter.
* Conditional compilation.  [IF]   [ELSE]   [THEN]
* Local variables using { }
* 'C' like structure defining words.
* Vectored execution using DEFER
* Can be compiled without any stdlib calls for embedded systems. Only needs custom KEY and EMIT equivalents in 'C'.

## Known issues
1) I was unable (so far) to add Odroid Go build support in Phil's pforth repository structure.  This was made difficult
by the ESP-IDF build system and Makefiles, which have their own opinion about project directory structure.

## Other notes
This is not a traditional embedded pForth.  Odroid Go features an SD card reader/writer and basic posix style file
system access.  Odroid Go also features posix-like putchar/getchar/printf functions to access the USB/Serial console.

Editing Forth programs is TBD.  One of pForth's features is real File I/O and includes.

