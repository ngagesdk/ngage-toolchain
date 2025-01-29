/* @file celeste_SDL3.c
 *
 * A C source port of the original Celeste game,
 * highly optimized for the Nokia N-Gage.
 *
 * Original game by Maddy Makes Games.
 * C source port by lemon32767.
 *
 * https://github.com/lemon32767/ccleste
 *
 */

#ifndef CELESTE_SDL3_H
#define CELESTE_SDL3_H

#include <SDL3/SDL.h>
#include "celeste.h"

#define NGAGE_W 176
#define NGAGE_H 208

#define PICO8_W 128
#define PICO8_H 128

#define SCALE 1

void LoadData(void);
void RefreshPalette(void);
void ResetPalette(void);

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);

#endif // CELESTE_SDL3_H
