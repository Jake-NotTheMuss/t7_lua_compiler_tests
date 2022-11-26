T7 Lua Compiler Interface
=========================

Performs compilation of Lua scripts using the Black Ops III Havok Script
compiler.

I use this for testing and comparing compiler output with my in-house compiler
that I am working on which will match the features and capabilities of the Call
of Duty Lua compiler.

## How To Build
Download the source to a folder within `mods` in the root Black Ops III folder,
then run `build.bat`. It will build the mod and the DLL. The script requires a
64-bit Visual C++ compiler to be installed on the machine.

## How To Use
Add your Lua scripts to the folder `tests`. The mod will find any files ending
with `.lua` within that directory tree and compile them.

Once the mod is loaded in-game, press `RUN LUA COMPILER TESTS` to compile all
test files.

Compiler errors are printed to `error.log`.
