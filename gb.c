#include "minigb_apu/minigb_apu.h"
#if PEANUT_FULL_GBC_SUPPORT
#include "peanut_gbc.h"
#else
#include "peanut_gb.h"
#endif

struct gb_s gb;

struct priv_t {
  uint8_t *rom;
  uint8_t *cart_ram;
  uint8_t *bootrom;
  uint16_t selected_palette[3][4];
  uint16_t fb[LCD_HEIGHT][LCD_WIDTH];
} priv;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-function-declaration"
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"

char title[16];
int selected_palette = 3;
unsigned char rom_bytes[8 * 1024 * 1024] = {};
unsigned char sav_bytes[32 * 1024] = {};
#define SAMPLE_BYTES 548 * 4
uint8_t audio_buffer[SAMPLE_BYTES];

int is_cgb_mode() {
#if PEANUT_FULL_GBC_SUPPORT
  if (gb.cgb.cgbMode) {
    return 1;
  } else {
#endif
    return 0;
#if PEANUT_FULL_GBC_SUPPORT
  }
#endif
}
void run_frame() { gb_run_frame(&gb); }
void reset() { gb_reset(&gb); }
void set_date(int sec, int min, int hour, int yday) {
  timeinfo.tm_sec = sec;
  timeinfo.tm_min = min;
  timeinfo.tm_hour = hour;
  timeinfo.tm_yday = yday;
}
void get_audio_callback() { audio_callback(NULL, audio_buffer, SAMPLE_BYTES); }
void load_sav(uint8_t sav, int i) { sav_bytes[i] = sav; }
void load_rom(uint8_t rom, int i) { rom_bytes[i] = rom; }
void repeat_a() { gb.direct.joypad_bits.a = ~gb.direct.joypad_bits.a; }
void repeat_b() { gb.direct.joypad_bits.b = ~gb.direct.joypad_bits.b; }
void press_a() { gb.direct.joypad_bits.a = 0; }
void release_a() { gb.direct.joypad_bits.a = 1; }
void press_b() { gb.direct.joypad_bits.b = 0; }
void release_b() { gb.direct.joypad_bits.b = 1; }
void press_start() { gb.direct.joypad_bits.start = 0; }
void release_start() { gb.direct.joypad_bits.start = 1; }
void press_select() { gb.direct.joypad_bits.select = 0; }
void release_select() { gb.direct.joypad_bits.select = 1; }
void press_up() { gb.direct.joypad_bits.up = 0; }
void release_up() { gb.direct.joypad_bits.up = 1; }
void press_down() { gb.direct.joypad_bits.down = 0; }
void release_down() { gb.direct.joypad_bits.down = 1; }
void press_left() { gb.direct.joypad_bits.left = 0; }
void release_left() { gb.direct.joypad_bits.left = 1; }
void press_right() { gb.direct.joypad_bits.right = 0; }
void release_right() { gb.direct.joypad_bits.right = 1; }

uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr) {
  const struct priv_t *const p = gb->direct.priv;
  return p->rom[addr];
}

uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr) {
  const struct priv_t *const p = gb->direct.priv;
  return p->cart_ram[addr];
}

void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr,
                       const uint8_t val) {
  const struct priv_t *const p = gb->direct.priv;
  p->cart_ram[addr] = val;
}

uint8_t gb_bootrom_read(struct gb_s *gb, const uint_fast16_t addr) {
  const struct priv_t *const p = gb->direct.priv;
  return p->bootrom[addr];
}

void gb_error(struct gb_s *gb, const enum gb_error_e gb_err,
              const uint16_t addr) {
  const char *gb_err_str[GB_INVALID_MAX] = {"UNKNOWN", "INVALID OPCODE",
                                            "INVALID READ", "INVALID WRITE",
                                            "HALT FOREVER"};
  uint8_t instr_byte = __gb_read(gb, addr);

  // TODO: see what happens on a real error, can call this.quit if needed
  save();

  if (addr >= 0x4000 && addr < 0x8000) {
    uint32_t rom_addr = (uint32_t)addr * (uint32_t)gb->selected_rom_bank;

    printf("Error: %s at 0x%04X (bank %d mode %d, file offset %u) with "
           "instruction %02X.\n"
           "Cart RAM saved to recovery.sav\n"
           "Exiting.\n",
           gb_err_str[gb_err], addr, gb->selected_rom_bank,
           gb->cart_mode_select, rom_addr, instr_byte);
  } else {
    printf("Error: %s at 0x%04X with "
           "instruction %02X.\n"
           "Cart RAM saved to recovery.sav\n"
           "Exiting.\n",
           gb_err_str[gb_err], addr, instr_byte);
  }

  alert("Error: check browser console");
}

#if ENABLE_LCD
void lcd_draw_line(struct gb_s *gb, const uint8_t pixels[LCD_WIDTH],
                   const uint_least8_t line) {
  struct priv_t *priv = gb->direct.priv;
#if PEANUT_FULL_GBC_SUPPORT
  if (gb->cgb.cgbMode) {
    for (unsigned int x = 0; x < LCD_WIDTH; x++) {
      priv->fb[line][x] = gb->cgb.fixPalette[pixels[x]];
    }
  } else {
#endif
      for (unsigned int x = 0; x < LCD_WIDTH; x++) {
        priv->fb[line][x] =
            priv->selected_palette[(pixels[x] & LCD_PALETTE_ALL) >> 4]
                                  [pixels[x] & 3];
      }
      // const uint32_t palette[] = {0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000};
      // for (unsigned int x = 0; x < LCD_WIDTH; x++)
      //   priv->fb[line][x] = palette[pixels[x] & 3];
#if PEANUT_FULL_GBC_SUPPORT
  }
#endif
}
#endif

void auto_assign_palette(struct priv_t *priv, uint8_t game_checksum) {
  size_t palette_bytes = 3 * 4 * sizeof(uint16_t);

  switch (game_checksum) {
  /* Balloon Kid and Tetris Blast */
  case 0x71:
  case 0xFF: {
    const uint16_t palette[3][4] = {
        {0x7FFF, 0x7E60, 0x7C00, 0x0000}, /* OBJ0 */
        {0x7FFF, 0x7E60, 0x7C00, 0x0000}, /* OBJ1 */
        {0x7FFF, 0x7E60, 0x7C00, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Pokemon Yellow and Tetris */
  case 0x15:
  case 0xDB:
  case 0x95: /* Not officially */
  {
    const uint16_t palette[3][4] = {
        {0x7FFF, 0x7FE0, 0x7C00, 0x0000}, /* OBJ0 */
        {0x7FFF, 0x7FE0, 0x7C00, 0x0000}, /* OBJ1 */
        {0x7FFF, 0x7FE0, 0x7C00, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Donkey Kong */
  case 0x19: {
    const uint16_t palette[3][4] = {
        {0x7FFF, 0x7E10, 0x48E7, 0x0000}, /* OBJ0 */
        {0x7FFF, 0x7E10, 0x48E7, 0x0000}, /* OBJ1 */
        {0x7FFF, 0x7E60, 0x7C00, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Pokemon Blue */
  case 0x61:
  case 0x45:

  /* Pokemon Blue Star */
  case 0xD8: {
    const uint16_t palette[3][4] = {
        {0x7FFF, 0x7E10, 0x48E7, 0x0000}, /* OBJ0 */
        {0x7FFF, 0x329F, 0x001F, 0x0000}, /* OBJ1 */
        {0x7FFF, 0x329F, 0x001F, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Pokemon Red */
  case 0x14: {
    const uint16_t palette[3][4] = {
        {0x7FFF, 0x3FE6, 0x0200, 0x0000}, /* OBJ0 */
        {0x7FFF, 0x7E10, 0x48E7, 0x0000}, /* OBJ1 */
        {0x7FFF, 0x7E10, 0x48E7, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Pokemon Red Star */
  case 0x8B: {
    const uint16_t palette[3][4] = {
        {0x7FFF, 0x7E10, 0x48E7, 0x0000}, /* OBJ0 */
        {0x7FFF, 0x329F, 0x001F, 0x0000}, /* OBJ1 */
        {0x7FFF, 0x3FE6, 0x0200, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Kirby */
  case 0x27:
  case 0x49:
  case 0x5C:
  case 0xB3: {
    const uint16_t palette[3][4] = {
        {0x7D8A, 0x6800, 0x3000, 0x0000}, /* OBJ0 */
        {0x001F, 0x7FFF, 0x7FEF, 0x021F}, /* OBJ1 */
        {0x527F, 0x7FE0, 0x0180, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Donkey Kong Land [1/2/III] */
  case 0x18:
  case 0x6A:
  case 0x4B:
  case 0x6B: {
    const uint16_t palette[3][4] = {
        {0x7F08, 0x7F40, 0x48E0, 0x2400}, /* OBJ0 */
        {0x7FFF, 0x2EFF, 0x7C00, 0x001F}, /* OBJ1 */
        {0x7FFF, 0x463B, 0x2951, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Link's Awakening */
  case 0x70: {
    const uint16_t palette[3][4] = {
        {0x7FFF, 0x03E0, 0x1A00, 0x0120}, /* OBJ0 */
        {0x7FFF, 0x329F, 0x001F, 0x001F}, /* OBJ1 */
        {0x7FFF, 0x7E10, 0x48E7, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* Mega Man [1/2/3] & others I don't care about. */
  case 0x01:
  case 0x10:
  case 0x29:
  case 0x52:
  case 0x5D:
  case 0x68:
  case 0x6D:
  case 0xF6: {
    const uint16_t palette[3][4] = {
        {0x7FFF, 0x329F, 0x001F, 0x0000}, /* OBJ0 */
        {0x7FFF, 0x3FE6, 0x0200, 0x0000}, /* OBJ1 */
        {0x7FFF, 0x7EAC, 0x40C0, 0x0000}  /* BG */
    };
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  default: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x5294, 0x294A, 0x0000},
                                    {0x7FFF, 0x5294, 0x294A, 0x0000},
                                    {0x7FFF, 0x5294, 0x294A, 0x0000}};
    printf("No palette found for 0x%02X.", game_checksum);
    memcpy(priv->selected_palette, palette, palette_bytes);
  }
  }
}

void manual_assign_palette(struct priv_t *priv, uint8_t selection) {
#define NUMBER_OF_PALETTES 12
  size_t palette_bytes = 3 * 4 * sizeof(uint16_t);

  switch (selection) {
  /* 0x05 (Right) */
  case 0: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x2BE0, 0x7D00, 0x0000},
                                    {0x7FFF, 0x2BE0, 0x7D00, 0x0000},
                                    {0x7FFF, 0x2BE0, 0x7D00, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x07 (A + Down) */
  case 1: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x7FE0, 0x7C00, 0x0000},
                                    {0x7FFF, 0x7FE0, 0x7C00, 0x0000},
                                    {0x7FFF, 0x7FE0, 0x7C00, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x12 (Up) */
  case 2: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                                    {0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                                    {0x7FFF, 0x7EAC, 0x40C0, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x13 (B + Right) */
  case 3: {
    const uint16_t palette[3][4] = {{0x0000, 0x0210, 0x7F60, 0x7FFF},
                                    {0x0000, 0x0210, 0x7F60, 0x7FFF},
                                    {0x0000, 0x0210, 0x7F60, 0x7FFF}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x16 (B + Left, DMG Palette) */
  default:
  case 4: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x5294, 0x294A, 0x0000},
                                    {0x7FFF, 0x5294, 0x294A, 0x0000},
                                    {0x7FFF, 0x5294, 0x294A, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x17 (Down) */
  case 5: {
    const uint16_t palette[3][4] = {{0x7FF4, 0x7E52, 0x4A5F, 0x0000},
                                    {0x7FF4, 0x7E52, 0x4A5F, 0x0000},
                                    {0x7FF4, 0x7E52, 0x4A5F, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x19 (B + Up) */
  case 6: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                                    {0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                                    {0x7F98, 0x6670, 0x41A5, 0x2CC1}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x1C (A + Right) */
  case 7: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                                    {0x7FFF, 0x7E10, 0x48E7, 0x0000},
                                    {0x7FFF, 0x3FE6, 0x0198, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x0D (A + Left) */
  case 8: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                                    {0x7FFF, 0x7EAC, 0x40C0, 0x0000},
                                    {0x7FFF, 0x463B, 0x2951, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x10 (A + Up) */
  case 9: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x3FE6, 0x0200, 0x0000},
                                    {0x7FFF, 0x329F, 0x001F, 0x0000},
                                    {0x7FFF, 0x7E10, 0x48E7, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x18 (Left) */
  case 10: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x7E10, 0x48E7, 0x0000},
                                    {0x7FFF, 0x3FE6, 0x0200, 0x0000},
                                    {0x7FFF, 0x329F, 0x001F, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }

  /* 0x1A (B + Down) */
  case 11: {
    const uint16_t palette[3][4] = {{0x7FFF, 0x329F, 0x001F, 0x0000},
                                    {0x7FFF, 0x3FE6, 0x0200, 0x0000},
                                    {0x7FFF, 0x7FE0, 0x3D20, 0x0000}};
    memcpy(priv->selected_palette, palette, palette_bytes);
    break;
  }
  }

  return;
}

void next_palette() {
  if (!is_cgb_mode()) {
    if (++selected_palette == NUMBER_OF_PALETTES) {
      selected_palette = 0;
    }

    manual_assign_palette(&priv, selected_palette);
    printf("Now using palette %d", selected_palette + 1);
  }
}

void previous_palette() {
  if (!is_cgb_mode()) {
    if (--selected_palette == -1) {
      selected_palette = NUMBER_OF_PALETTES - 1;
    }

    manual_assign_palette(&priv, selected_palette);
    printf("Now using palette %d", selected_palette + 1);
  }
}

void set_palette() {
  if (!is_cgb_mode()) {
    manual_assign_palette(&priv, selected_palette);
    printf("Now using palette %d", selected_palette + 1);
  }
}

void reset_palette() {
  if (!is_cgb_mode()) {
    auto_assign_palette(&priv, gb_colour_hash(&gb));
    printf("Now using the default palette");
  }
}

void set_time(uint8_t rtc) {
  if (rtc) {
    load_date();
    gb_set_rtc(&gb, &timeinfo);
  } else {
    memset(&gb.rtc_real, 0xFF, sizeof(gb.rtc_real));
  }
}

int main() {
  priv.rom = rom_bytes;
  priv.cart_ram = sav_bytes;

  enum gb_init_error_e gb_ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read,
                                        &gb_cart_ram_write, &gb_error, &priv);

  switch (gb_ret) {
  case GB_INIT_NO_ERROR:
    break;

  case GB_INIT_CARTRIDGE_UNSUPPORTED: {
    const char err_cart[] = "Unsupported cartridge.";
    printf(err_cart);
    alert(err_cart);
    break;
  }
  case GB_INIT_INVALID_CHECKSUM: {
    const char err_sum[] = "Invalid ROM: Checksum failure.";
    printf(err_sum);
    alert(err_sum);
    break;
  }
  default: {
    printf("Unknown error: %d", gb_ret);
    alert("Unknown error: check browser console");
    break;
  }
  }

  // TODO: Place behind define as Peanut-GB does not support the CGB variant
  // priv.bootrom = dmg_boot_bin;
  // gb_set_bootrom(&gb, gb_bootrom_read);
  // gb_reset(&gb);
  // printf("No dmg_boot.bin file found; disabling boot ROM");
  // printf("boot ROM enabled");

  get_time();

  get_savebuffer_ptr(sav_bytes);

  get_framebuffer_ptr(priv.fb);

  get_audiobuffer_ptr(audio_buffer);

  set_title(gb_get_rom_name(&gb, title));

#if defined(ENABLE_SOUND) && defined(ENABLE_SOUND_MINIGB)
  audio_init();
#endif

#if ENABLE_LCD
  gb_init_lcd(&gb, &lcd_draw_line);
#endif

  if (!is_cgb_mode()) {
    auto_assign_palette(&priv, gb_colour_hash(&gb));
  }

  return gb_ret;
}

#pragma clang diagnostic pop
