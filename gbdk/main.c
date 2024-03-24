// #include <gb/gb.h>
#include <stdio.h>

void main(void) {
  // BGP_REG = 0x27U;
  // BGP_REG = DMG_PALETTE(DMG_BLACK, DMG_DARK_GRAY, DMG_LITE_GRAY, DMG_WHITE);

  puts("    HOW TO USE:");
  puts("DRAG>DROP ROM+SAVE");
  puts("");
  puts("A+B=X+Z DPAD=ARROWS");
  printf("STA=ENTER SEL=RSHIFT");
  puts("TURBO=1-4 QUIT=BACK");
  puts("RESET=CTRL-R");
  puts("SAVE=CTRL-ENTER");
  puts("");
  puts("PAL=P/CTRL-P/ALT-P");
  puts("VOLUME=0/-/= MUTE=M");
  puts("RAPID A+B=W+Q/S+A");
  printf("HELD TURBO=E/D/C/\" \"");
  puts("SET TURBO=ALT+1-[4]");
  puts("");
  printf("GB.FETCH(\"ROM/PATH\")");
  puts("GB.RTC=[TRUE]FALSE");
  printf("GB.LINK=[TRUE]FALSE");

  // while (1) {
  //   vsync();
  // }
}
