# Super Rockport Ball

A mod for Need for Speed: Most Wanted that turns you into a gigantic spinning ball

## Installation

- Make sure you have v1.3 of the game, as this is the only version this plugin is compatible with. (exe size of 6029312 bytes)
- Plop the files into your game folder.
- Enjoy, nya~ :3

## Disclaimer

Due to the current programming landscape, I feel that it's necessary to explicitly state that this project had zero assistance or any other kind of involvement from any sort of "AI agent" and it never will.  
This mod was entirely built by hand, by a human being, and I believe that any project that cannot also claim this about itself is not worth people's time.

## Building

Building is done on an Arch Linux system with CLion being used for the build process. 

Before you begin, clone [nya-common](https://github.com/gaycoderprincess/nya-common), [nya-common-nfsmw](https://github.com/gaycoderprincess/nya-common-nfsmw), and [CwoeeMenuLib](https://github.com/gaycoderprincess/CwoeeMenuLib) to folders next to this one, so they can be found.

Required packages: `mingw-w64-gcc vcpkg`

To install all dependencies, use:
```console
vcpkg install tomlplusplus:x86-mingw-static
```

To install the BASS audio library:

Download the Win32 version from [here](https://www.un4seen.com/bass.html) and extract it somewhere

Once that's done, copy `bass.lib` from the `c` folder into `nya-common/lib32`

You should be able to build the project now in CLion.
