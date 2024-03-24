# [Peanut-GB](https://github.com/deltabeard/Peanut-GB) webassembly front-end
Small non-emscripten build, supports fast forward and rapid a+b. Inspired by dev videos of [raylib.js](https://github.com/tsoding/zozlib.js).

## Controls
Search for `keyDown` in [gb.js](gb.js). To load a rom you can drag and drop onto the canvas (multiple files work, for saves). The initial canvas will be 160x144, zoom in to scale it up. For extra vertical screen space, the bookmark bar can be toggled with `Ctrl-Shift-B`. Extra options are available through the browser console:

+ `gb.fetch("path/to/rom")`: Fetches a rom and will also try to get a save file with same name + path. You can fetch from disk using relative path if hosted locally or from external sources like https://hh.gbdev.io/ but it will need the CORS header set.

+ `gb.rtc = false`: Resets the RTC in some games, then drag & drop or fetch the game again.

## Build
Requires `clang` compiler for wasm32 target, call `make` or just copy/paste the command from makefile. Then run a webserver such as `py -m http.server` for Windows or `python3 -m http.server` otherwise.

> [!NOTE]
Optional: To embed a rom use `xxd -i rom.gbc > rom.h`, include the header in [gb.c](gb.c) and pass it to priv.rom to set the game. `#embed` could be used in the future when clang supports it. If you are doing this you can also remove the allocated memory for `rom_bytes` and other rom loading code.

## To do
Mobile webpage layout, touch and controller support, fix fast-forward audio (and this.always_run).

## Limitations
+ Peanut-GB `cgb` branch is being used for GBC games support but it's not flawless, there are some graphical glitches and CGB boot roms can't be used for now (https://github.com/LIJI32/SameBoy/tree/master/BootROMs).

+ Fast forward is disabled on Firefox as setTimeout does not work correctly so it uses requestAnimationFrame exclusively, but does work on Chrome, Edge and presumably all other chromium-based browsers.

+ Link cable is enabled by default and doesn't use any networking but duplicates sent data back to you, this allows you to trade with yourself etc. Linking causes some games to break at the moment like tetris, so you can disable it with `gb.link_cable = false`.
