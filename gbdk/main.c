// #include <gb/gb.h>
#include <stdio.h>

void main(void) {
  // BGP_REG = 0x27U;
  // BGP_REG = DMG_PALETTE(DMG_BLACK, DMG_DARK_GRAY, DMG_LITE_GRAY, DMG_WHITE);

  puts("");
  puts("    HOW TO USE:");
  puts("");
  puts("DRAG>DROP ROM+SAVE");
  puts("");
  puts("A+B=X+Z DPAD=ARROWS");
  printf("STA=ENTER SEL=RSHIFT");
  puts("TURBO=1-4 RESET=R");
  puts("QUIT=BACK MUTE=M");
  puts("SAVE=CTRL+ENTER");
  puts("");
  puts("VOLUME=0/-/=");
  printf("PALETTES=p/P/C-p/A-p");
  puts("RAPID A+B=W+Q/S+A");
  printf("HELD TURBO=E/D/C/\" \"");
  printf("HELD SPEED=ALT+1-[4]");
  puts("");
  printf("GB.FETCH(\"ROM/PATH\")");
  printf("GB.RTC=[TRUE]FALSE");

  // while (1) {
  //   vsync();
  // }
}
