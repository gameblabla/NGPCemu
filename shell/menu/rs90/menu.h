#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include "video_blit.h"

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

#define RGB565(r,g,b) ((r << 8) | (g << 3) | (b >> 3))

extern uint_fast8_t emulator_state;
extern uint_fast8_t done;

extern void Menu();
extern void Init_Configuration();

#endif
