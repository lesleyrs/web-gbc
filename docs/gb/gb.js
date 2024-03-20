class GameBoy {
  constructor() {
    this.rtc = true;
    this.fast_mode = 1;
    this.last_mode = 1;
    this.ff_speed = 4;
    this.turbo = false;
    this.repeat_keys = new Set();
    this.volume = 0.5;
    this.muted = false;
    this.audio_latency = 0.1;
  }

  async start({ wasmPath, canvasId, rom, sav }) {
    const canvas = document.getElementById(canvasId);
    this.ctx = canvas.getContext("2d", { alpha: false });
    if (!this.wasm) {
      this.wasm = await WebAssembly.instantiateStreaming(fetch(wasmPath), {
        env: make_environment(this)
      });
    }

    // console.log(this.wasm.instance);

    this.#load(rom, sav);

    const dragOver = (event) => {
      event.preventDefault();
    }

    const drop = async (event) => {
      event.preventDefault();
      this.quit_request = true;
      const files = event.dataTransfer.files;
      let rom, sav = undefined;

      for (const file of files) {
        const ext = file.name.substring(file.name.lastIndexOf('.') + 1);
        const name = file.name.substring(0, file.name.lastIndexOf('.'));
        if (ext == "gbc" || ext == "gb") {
          rom = file;
          sav = [...files].find(f => {
            const savExt = f.name.substring(f.name.lastIndexOf('.') + 1);
            const savName = f.name.substring(0, f.name.lastIndexOf('.'));
            return (savExt === "sav" || savExt === "sa2") && savName === name;
          });
          if (!sav) {
            rom = files[0];
          } else {
            break;
          }
        }
      }

      this.rom = rom.name;
      this.sav = sav ? sav.name : undefined;

      console.log(rom, sav);

      const romBytes = await rom.arrayBuffer();
      const savBytes = sav ? await sav.arrayBuffer() : undefined;

      this.#await_quit(romBytes, savBytes);
    }

    const resize = () => {
      console.log("GameBoy scale is set to " + get_scale());
    }

    const keyDown = async (event) => {
      switch (event.key) {
        case "P":
          if (!event.repeat) {
            if (event.shiftKey) {
              this.wasm.instance.exports.previous_palette();
              break;
            }
          }
        case "p":
          if (!event.repeat) {
            if (event.ctrlKey) {
              event.preventDefault();
              this.wasm.instance.exports.reset_palette();
              break;
            }
            if (event.altKey) {
              this.wasm.instance.exports.set_palette();
              break;
            }

            this.wasm.instance.exports.next_palette();
          }
          break;
        case "Enter":
          if (event.ctrlKey) {
            this.save(this.sav);
          } else {
            this.wasm.instance.exports.press_start();
          }
          break;
        case "Shift":
          if (!event.ctrlKey && event.location == KeyboardEvent.DOM_KEY_LOCATION_RIGHT) {
            this.wasm.instance.exports.press_select();
          }
          break;
        case "x":
          this.wasm.instance.exports.press_a();
          break;
        case "z":
          this.wasm.instance.exports.press_b();
          break;
        case "s":
        case "w":
          this.wasm.instance.exports.press_a();
          this.repeat_keys.add(event.code);
          break;
        case "a":
        case "q":
          this.wasm.instance.exports.press_b();
          this.repeat_keys.add(event.code);
          break;
        case "ArrowUp":
          if (!event.altKey) {
            event.preventDefault();
          }
          this.wasm.instance.exports.press_up();
          break;
        case "ArrowRight":
          if (!event.altKey) {
            event.preventDefault();
          }
          this.wasm.instance.exports.press_right();
          break;
        case "ArrowDown":
          if (!event.altKey) {
            event.preventDefault();
          }
          this.wasm.instance.exports.press_down();
          break;
        case "ArrowLeft":
          if (!event.altKey) {
            event.preventDefault();
          }
          this.wasm.instance.exports.press_left();
          break;
        case "e":
        case "d":
        case "c":
        case " ":
          if (!event.repeat && !this.turbo && !event.altKey) {
            this.startSec = audioCtx.currentTime + this.audio_latency;
            this.turbo = true;
            this.last_mode = this.fast_mode;
            console.log("Holding fast-forward key, Fast mode is " + (this.fast_mode = this.ff_speed));
          }
          break;
        case "1":
        case "2":
        case "3":
        case "4":
        // NOTE: past 4 it seems to make no difference
        case "5":
        case "6":
        case "7":
        case "8":
        case "9":
          if (!event.repeat) {
            if (event.altKey) {
              console.log("Default fast-forward speed is set to " + (this.ff_speed = event.key));
            } else {
              this.startSec = audioCtx.currentTime + this.audio_latency;
              console.log("Fast-forward mode is set to " + (this.fast_mode = event.key));
            }
          }
          break;
        case "0":
          if (!event.ctrlKey) {
            this.volume = 0.5;
            console.log("Volume reset: " + this.volume);
          }
          break;
        case "-":
          if (!event.ctrlKey) {
            this.volume -= 0.1;
            if (this.volume < 0 + Number.EPSILON) {
              this.volume = 0;
            }
            console.log("Volume: " + this.volume);
          }
          break;
        case "=":
          if (!event.ctrlKey) {
            this.volume += 0.1;
            if (this.volume > 1 - Number.EPSILON) {
              this.volume = 1;
            }
            console.log("Volume: " + this.volume);
          }
          break;
        case "m":
          this.muted = !this.muted;
          console.log("Audio", this.muted ? "muted" : "unmuted");
          break;
        case "r":
          if (!event.repeat && !event.ctrlKey) {
            this.wasm.instance.exports.reset();
          }
          break;
        case "Backspace":
          if (!event.repeat && !this.fetching) {
            await this.fetch("main.gb");
          }
          break;
      }
    }

    const keyUp = (event) => {
      switch (event.key) {
        case "Enter":
          this.wasm.instance.exports.release_start();
          break;
        case "Shift":
          this.wasm.instance.exports.release_select();
          break;
        case "x":
          this.wasm.instance.exports.release_a();
          break;
        case "z":
          this.wasm.instance.exports.release_b();
          break;
        case "s":
        case "w":
          this.wasm.instance.exports.release_a();
          this.repeat_keys.delete(event.code);
          break;
        case "a":
        case "q":
          this.wasm.instance.exports.release_b();
          this.repeat_keys.delete(event.code);
          break;
        case "ArrowUp":
          this.wasm.instance.exports.release_up();
          break;
        case "ArrowRight":
          this.wasm.instance.exports.release_right();
          break;
        case "ArrowDown":
          this.wasm.instance.exports.release_down();
          break;
        case "ArrowLeft":
          this.wasm.instance.exports.release_left();
          break;
        case "e":
        case "c":
        case "d":
        case " ":
          this.startSec = audioCtx.currentTime + this.audio_latency;
          this.turbo = false;
          this.fast_mode = this.last_mode;
          break;
      }
    }

    this.dragOver = dragOver.bind(this);
    this.drop = drop.bind(this);
    this.resize = resize.bind(this);
    this.keyDown = keyDown.bind(this);
    this.keyUp = keyUp.bind(this);

    window.addEventListener('dragover', this.dragOver);
    window.addEventListener('drop', this.drop);
    window.addEventListener('resize', this.resize);
    window.addEventListener('keydown', this.keyDown);
    window.addEventListener('keyup', this.keyUp);

    this.wasm.instance.exports.main() && this.exit();

    const memory = new Uint16Array(this.wasm.instance.exports.memory.buffer);
    const imageData = this.ctx.createImageData(canvas.width, canvas.height);
    const audioCtx = new AudioContext();

    const update = () => {
      if (this.quit_request) {
        this.quit = true;
        return;
      }

      this.wasm.instance.exports.run_frame();
      if (!this.muted) {
        // https://binji.github.io/posts/binjgb-on-the-web-part-2/
        this.wasm.instance.exports.get_audio_callback();

        const nowSec = audioCtx.currentTime;
        const nowPlusLatency = nowSec + this.audio_latency;
        this.startSec = (this.startSec || nowPlusLatency);
        if (this.startSec >= nowSec) {
          let buffer = audioCtx.createBuffer(2, 548, 32768);
          let channel0 = buffer.getChannelData(0);
          let channel1 = buffer.getChannelData(1);
          for (let i = 0; i < buffer.length; i++) {
            channel0[i] = (memory[this.audiobuffer_ptr / 2 + i * 2] << 16) * this.volume / 2147483648.0;
            channel1[i] = (memory[this.audiobuffer_ptr / 2 + i * 2 + 1] << 16) * this.volume / 2147483648.0;
          }
          var source = audioCtx.createBufferSource();
          source.buffer = buffer;
          source.connect(audioCtx.destination);
          source.start(this.startSec / this.fast_mode);

          const bufferSec = 548 / 32768;
          this.startSec += bufferSec;
        } else {
          console.log(
            'Resetting audio (' + this.startSec.toFixed(2) + ' < ' +
            nowSec.toFixed(2) + ')');
          this.startSec = nowPlusLatency;
        }
      }

      if (this.repeat_keys.has("KeyS") || this.repeat_keys.has("KeyW")) {
        this.wasm.instance.exports.repeat_a();
      }
      if (this.repeat_keys.has("KeyA") || this.repeat_keys.has("KeyQ")) {
        this.wasm.instance.exports.repeat_b();
      }

      for (let i = 0; i < canvas.width * canvas.height; i++) {
        const color = memory[this.framebuffer_ptr / 2 + i];
        const index = i * 4;
        imageData.data[index + 3] = 255;

        const red = (color >> 10) & 0x1F;
        const green = (color >> 5) & 0x1F;
        const blue = color & 0x1F;

        imageData.data[index] = (red * 255) / 31;
        imageData.data[index + 1] = (green * 255) / 31;
        imageData.data[index + 2] = (blue * 255) / 31;
        // imageData.data.fill(color & 0xFF, index, index + 3);
      }
      this.ctx.putImageData(imageData, 0, 0);

      // NOTE: Pauses when tab focus is lost at x1 speed due to using rAF, maybe expose a toggle.
      if (this.fast_mode > 1 && !navigator.userAgent.includes("Firefox")) {
        setTimeout(update, 1000 / (60 * this.fast_mode));
      } else {
        requestAnimationFrame(update);
      }
    }
    update();
  }

  exit() {
    this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
    this.quit_request = true;
  }

  save(sav) {
    let savBytes = new Uint8Array(this.wasm.instance.exports.memory.buffer, this.savebuffer_ptr, 32 * 1024);
    let blob = new Blob([savBytes], { type: 'application/octet-stream' });
    let url = window.URL.createObjectURL(blob);
    let a = document.createElement('a');
    a.href = url;
    if (sav === undefined) {
      sav = "recovery.sav";
    }
    a.download = sav;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    window.URL.revokeObjectURL(url);
  }

  async fetch(rom) {
    this.fetching = true;
    this.quit_request = true;
    const response = await fetch(this.rom = rom, { cache: 'no-cache' });
    const data = await response.arrayBuffer();

    this.sav = rom.substring(0, rom.lastIndexOf('.')) + ".sav";
    const saveResponse = await fetch(this.sav, { cache: 'no-cache' });
    const saveData = await saveResponse.arrayBuffer();

    this.#await_quit(data, saveData);
  }

  #await_quit(romBytes, savBytes) {
    if (this.quit) {
      this.#reset(romBytes, savBytes);
    } else {
      // NOTE: is there no better way to do this? 10 is magic
      setTimeout(() => this.#await_quit(romBytes, savBytes), 10);
    }
  }

  #load(rom, sav) {
    const romBytes = new Uint8Array(rom);
    const savBytes = new Uint8Array(sav);

    for (let i = 0; i < romBytes.length; i++) {
      this.wasm.instance.exports.load_rom(romBytes[i], i);
    }
    console.log("Received", romBytes.length, "rom bytes.");

    for (let i = 0; i < savBytes.length; i++) {
      this.wasm.instance.exports.load_sav(savBytes[i], i);
    }
    console.log("Received", savBytes.length, "sav bytes.");
  }

  #reset(romBytes, savBytes) {
    window.removeEventListener('dragOver', this.dragOver);
    window.removeEventListener('drop', this.drop);
    window.removeEventListener('resize', this.resize);
    window.removeEventListener('keydown', this.keyDown);
    window.removeEventListener('keyup', this.keyUp);
    this.drop = undefined;
    this.resize = undefined;
    this.keyDown = undefined;
    this.keyUp = undefined;
    this.quit_request = undefined;
    this.quit = undefined;
    this.fetching = undefined;
    this.startSec = undefined;
    this.start({ wasmPath: "gb.wasm", canvasId: "gb", rom: romBytes, sav: savBytes });
  }

  get_time() {
    this.wasm.instance.exports.set_time(this.rtc);
  }

  load_date() {
    // https://stackoverflow.com/questions/8619879/javascript-calculate-the-day-of-the-year-1-366
    const now = new Date();
    var start = new Date(now.getFullYear(), 0, 0);
    var diff = (now - start) + ((start.getTimezoneOffset() - now.getTimezoneOffset()) * 60 * 1000);
    var oneDay = 1000 * 60 * 60 * 24;
    var day = Math.floor(diff / oneDay);
    this.wasm.instance.exports.set_date(now.getSeconds, now.getMinutes, now.getHours, day);
  }

  get_savebuffer_ptr(arg) {
    this.savebuffer_ptr = arg;
  }

  get_audiobuffer_ptr(arg) {
    this.audiobuffer_ptr = arg;
  }

  get_framebuffer_ptr(arg) {
    this.framebuffer_ptr = arg;
  }

  // TODO: Why does changing printf name cause it to pass by value instead of ptr? Can't add proper logging function
  printf(formatPtr, ...args) {
    const memory = new Uint8Array(this.wasm.instance.exports.memory.buffer);
    let format = '';
    let i = formatPtr;
    while (memory[i] !== 0) {
      format += String.fromCharCode(memory[i++]);
    }

    const separators = ["%02X", "%04X", "%d", "%u", "%s"];

    let parts = [];
    let usedSeparators = [];
    let currentPart = '';

    for (let i = 0; i < format.length; i++) {
      let foundSeparator = false;
      for (let separator of separators) {
        if (format.startsWith(separator, i)) {
          parts.push(currentPart);
          usedSeparators.push(separator);
          currentPart = '';
          i += separator.length - 1;
          foundSeparator = true;
          break;
        }
      }
      if (!foundSeparator) {
        currentPart += format[i];
      }
    }
    parts.push(currentPart);

    // console.log(parts);
    // console.log(usedSeparators);

    let result = '';
    for (let [i, part] of parts.entries()) {
      if (part == parts[0]) {
        continue;
      }
      // NOTE: this works with all values that aren't 64 bits.
      const arg = memory[args[0]] | (memory[args[0] + 1] << 8) | (memory[args[0] + 2] << 16) | (memory[args[0] + 3] << 24);
      switch (usedSeparators[i - 1]) {
        case "%02X":
          result += arg.toString(16).toUpperCase().padStart(2, '0') + part;
          break;
        case "%04X":
          result += arg.toString(16).toUpperCase().padStart(4, '0') + part;
          break;
        case "%s":
          let format = '';
          let j = arg;
          while (memory[j] !== 0) {
            format += String.fromCharCode(memory[j++]);
          }
          result += format + part;
          break;
        // NOTE: %u only works with 16 bit values or smaller
        default:
          result += arg + part;
          break;
      }
      args[0] += 4;
    }
    console.log(parts[0] + result);
  }

  raw_print(arg) {
    console.log(arg);
  }

  alert() {
    alert(get_text(arguments, this.wasm.instance.exports.memory.buffer));
  }

  set_title() {
    document.title = get_text(arguments, this.wasm.instance.exports.memory.buffer);
  }
}

function cstrlen(mem, ptr) {
  let len = 0;
  while (mem[ptr] != 0) {
    len++;
    ptr++;
  }
  return len;
}

function cstr_by_ptr(mem_buffer, ptr) {
  const mem = new Uint8Array(mem_buffer);
  const len = cstrlen(mem, ptr);
  const bytes = new Uint8Array(mem_buffer, ptr, len);
  return new TextDecoder().decode(bytes);
}

function get_text(args, mem_buffer) {
  const buffer = mem_buffer;
  let text = "";
  for (let i = 0; i < args.length; i++) {
    text += cstr_by_ptr(buffer, args[i]);
  }
  return text;
}

function get_scale() {
  const scale = Math.round(window.devicePixelRatio * 100) / 100;
  return scale;
}

function make_environment(env) {
  return new Proxy(env, {
    get(_target, prop, _receiver) {
      if (env[prop] !== undefined) {
        return env[prop].bind(env);
      }
      return (...args) => {
        throw new Error(`NOT IMPLEMENTED: ${prop} ${args}`);
      }
    }
  });
}
