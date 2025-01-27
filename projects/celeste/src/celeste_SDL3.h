/* @file celeste_SDL3.h
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
#include <stdio.h>

#define NGAGE_W 176
#define NGAGE_H 208

#define PICO8_W 128
#define PICO8_H 128

#define getcolor(col) map[col % 16]

extern SDL_Renderer *renderer;
extern SDL_Texture* SDL_screen_tex;
extern SDL_Surface* screen;
extern Uint16 buttons_state;
extern FILE* TAS;

void ResetPalette(void);
void Flip(SDL_Surface* screen);
void LoadData(void);
void OSDdraw(void);

void p8_rectfill(int x0, int y0, int x1, int y1, int col);
void p8_print(const char* str, int x, int y, int col);

#endif // CELESTE_SDL3_H