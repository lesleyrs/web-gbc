# [Peanut-GB](https://github.com/deltabeard/Peanut-GB) webassembly front-end
Small non-emscripten build, supports fast forward and rapid a+b. Inspired by dev videos of [raylib.js](https://github.com/tsoding/zozlib.js).

## Controls
Search `keyDown` in [gb.js](gb.js). You can drag & drop a rom on the canvas to load it (multiple files work, for saves). The initial canvas will be 160x144, zoom in to scale it up. Use `Ctrl-Shift-B` to toggle the bookmark bar for more vertical screen space. Other browser console options are:

+ `gb.fetch("path/to/rom")`: Fetch rom and save with same name. You can fetch from disk using relative path if hosted locally or from a link like https://hh.gbdev.io/ if it has the correct headers.

+ `gb.rtc = false`: Resets the RTC in some games, then drag & drop or fetch the game again.

+ `gb.link = false`: Disable link cable if it breaks your game[^1], then drag & drop or fetch the game again.

## Build
Requires `clang` compiler for wasm32 target, call `make` or just copy/paste the command from makefile. Then run a webserver such as `py -m http.server` for Windows or `python3 -m http.server` otherwise.

> [!NOTE]
Optional: To embed a rom use `xxd -i rom.gbc > rom.h`, include the header in [gb.c](gb.c) and pass it to priv.rom to set the game. `#embed` could be used in the future when clang supports it. If you are doing this you can also remove the allocated memory for `rom_bytes` and other rom loading code.

## To do
Mobile webpage layout, touch and controller support, fix fast-forward audio (and this.always_run+toggle).

## Limitations
+ Peanut-GB `cgb` branch is being used for GBC games support but it's not flawless, there are some graphical glitches and CGB boot roms can't be used for now (https://github.com/LIJI32/SameBoy/tree/master/BootROMs).

+ setTimeout doesn't work well on Firefox so you can't use fast-forward. Firefox on localhost seems to have vsync issues too?

[^1]: Link cable is enabled by default and doesn't use any networking but duplicates sent data back to you, this allows you to trade with yourself etc. Linking causes some games to break at the moment like tetris.
