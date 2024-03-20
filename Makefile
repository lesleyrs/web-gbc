gbc:
	clang --target=wasm32 -Wall -O3 -flto -nostdlib -Wl,--no-entry -Wl,--lto-O3 -Wl,--allow-undefined -Wl,--export-all -o gb.wasm gb.c minigb_apu/minigb_apu.c -DENABLE_SOUND -DENABLE_SOUND_MINIGB -DPEANUT_FULL_GBC_SUPPORT=1
	
gb:
	clang --target=wasm32 -Wall -O3 -flto -nostdlib -Wl,--no-entry -Wl,--lto-O3 -Wl,--allow-undefined -Wl,--export-all -o gb.wasm gb.c minigb_apu/minigb_apu.c -DENABLE_SOUND -DENABLE_SOUND_MINIGB

dist: rom docs

GB = docs/gb/
GBC = docs/gbc/

docs: gb
    ifeq ($(OS),Windows_NT)
		pwsh -Command Compress-Archive -Path index.html, gb.js, gb.wasm, main.gb -DestinationPath web-gb.zip -Force
    else
		zip web-gb.zip index.html gb.js gb.wasm main.gb
    endif
	mkdir -p $(GB)
	cp index.html $(GB)index.html
	cp gb.js $(GB)gb.js
	cp gb.wasm $(GB)gb.wasm
	cp main.gb $(GB)main.gb

	$(MAKE) gbc

    ifeq ($(OS),Windows_NT)
		pwsh -Command Compress-Archive -Path index.html, gb.js, gb.wasm, main.gb -DestinationPath web-gbc.zip -Force
    else
		zip web-gbc.zip index.html gb.js gb.wasm main.gb
    endif
	mkdir -p $(GBC)
	cp index.html $(GBC)index.html
	cp gb.js $(GBC)gb.js
	cp gb.wasm $(GBC)gb.wasm
	cp main.gb $(GBC)main.gb

rom:
	../gbdk/bin/lcc -o main.gb gbdk/main.c
