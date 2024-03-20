gbc:
	clang --target=wasm32 -Wall -O3 -flto -nostdlib -Wl,--no-entry -Wl,--lto-O3 -Wl,--allow-undefined -Wl,--export-all -o gb.wasm gb.c minigb_apu/minigb_apu.c -DENABLE_SOUND -DENABLE_SOUND_MINIGB -DPEANUT_FULL_GBC_SUPPORT=1
	
gb:
	clang --target=wasm32 -Wall -O3 -flto -nostdlib -Wl,--no-entry -Wl,--lto-O3 -Wl,--allow-undefined -Wl,--export-all -o gb.wasm gb.c minigb_apu/minigb_apu.c -DENABLE_SOUND -DENABLE_SOUND_MINIGB

# NOTE: format C > make dist > upload to itch
# TODO: try github actions for artifacts instead of docs directory
dist: rom zip

GB = docs/gb/
GBC = docs/gbc/

zip: docs
    ifeq ($(OS),Windows_NT)
		pwsh -Command Compress-Archive -Path $(GB)index.html, $(GB)gb.js, $(GB)gb.wasm, $(GB)main.gb -DestinationPath web-gb.zip -Force
		pwsh -Command Compress-Archive -Path $(GBC)index.html, $(GBC)gb.js, $(GBC)gb.wasm, $(GBC)main.gb -DestinationPath web-gbc.zip -Force
    else
		zip web-gb.zip $(GB)index.html $(GB)gb.js $(GB)gb.wasm $(GB)main.gb
		zip web-gbc.zip $(GBC)index.html $(GBC)gb.js $(GBC)gb.wasm $(GBC)main.gb
    endif

docs: gb
	mkdir -p $(GB)
	cp index.html $(GB)index.html
	cp gb.js $(GB)gb.js
	cp gb.wasm $(GB)gb.wasm
	cp main.gb $(GB)main.gb
	$(MAKE) gbc
	mkdir -p $(GBC)
	cp index.html $(GBC)index.html
	cp gb.js $(GBC)gb.js
	cp gb.wasm $(GBC)gb.wasm
	cp main.gb $(GBC)main.gb

rom:
	../gbdk/bin/lcc -o main.gb gbdk/main.c
