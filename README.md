# steam_get

Tool to get your achievements. Free and open-source, we bypass using Valve's headers by loading the functions manually.


Heavily based on VVVVVV's Steam API implementation.

## User's Guide

### NOTE:
**If the program complains about a missing libsteam_api.so on Linux, change into the directory where the build is run it like so: `LD_LIBRARY_PATH=$PWD ./steam_get` Make sure to copy over the SO from the repo if you have placed your build somewhere else!**


By default it will give you all achievements for Spacewar, Valve's dummy application.

Change `steam_appid.txt`'s contents to the Steam app ID of the app you intend to 100%. You can find this on [SteamDB](https://steamdb.info/).

Does not support stats, I was planning to but I realised you may not want your stats to be ruined.

Now run the program. When the program stops outputting text, feel free to close the Command Prompt window (or end the process if you are on \*nix) and check Steam for your piping-hot achievements.

## Compiling

Prequisites are SDL2 headers (for memory safe operations) and the POSIX Threads headerfile (to update Steam and get achivements at roughly the same time). Other than that only standard headers are used.

I made it as easy as possible. Run `build.bat` on Windows or `build.sh` on \*nix. Or you can compile it manually:

### Linux (used for Linux builds)
```
gcc steam_get.c -o steam_get -lSDL2
```
### Linux (MinGW) (used for Windows builds)
```
x86_64-w64-mingw32-gcc -mconsole steam_get.c -o steam_get -lSDL2 -lpthread
```
### Windows (MinGW only)
```
gcc -mconsole steam_get.c -o steam_get -lSDL2 -lpthread
```

This will output either a `steam_get` ELF or a `steam_get.exe` Windows Executable.
