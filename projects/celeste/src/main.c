/* @file main.c
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

#include <assert.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "celeste.h"

#define NGAGE_W 176
#define NGAGE_H 208

#define PICO8_W 128
#define PICO8_H 128

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Surface*  SDL_screen;
static SDL_Texture*  SDL_screen_tex;

SDL_Surface* screen;
SDL_Surface* gfx;
SDL_Surface* font;
static int scale = 1;
static const SDL_Color base_palette[16] =
{
    { 0x00, 0x00, 0x00 },
    { 0x1d, 0x2b, 0x53 },
    { 0x7e, 0x25, 0x53 },
    { 0x00, 0x87, 0x51 },
    { 0xab, 0x52, 0x36 },
    { 0x5f, 0x57, 0x4f },
    { 0xc2, 0xc3, 0xc7 },
    { 0xff, 0xf1, 0xe8 },
    { 0xff, 0x00, 0x4d },
    { 0xff, 0xa3, 0x00 },
    { 0xff, 0xec, 0x27 },
    { 0x00, 0xe4, 0x36 },
    { 0x29, 0xad, 0xff },
    { 0x83, 0x76, 0x9c },
    { 0xff, 0x77, 0xa8 },
    { 0xff, 0xcc, 0xaa }
};

static SDL_Color palette[16];
static Uint32 map[16];

#define getcolor(col) map[col % 16]

static void SetPaletteEntry(unsigned char idx, unsigned char base_idx)
{
    palette[idx] = base_palette[base_idx];

    SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(screen->format);
    map[idx] = SDL_MapRGB(details, NULL, palette[idx].r, palette[idx].g, palette[idx].b);
}

static void RefreshPalette(void)
{
    int i;
    SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(screen->format);

    for (i = 0; i < SDL_arraysize(map); i++)
    {
        map[i] = SDL_MapRGB(details, NULL, palette[i].r, palette[i].g, palette[i].b);
    }
}

static void ResetPalette(void)
{
    SDL_memcpy(palette, base_palette, sizeof palette);
    RefreshPalette();
}

static Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = SDL_BYTESPERPIXEL(surface->format);
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp)
    {
        case 1:
            return *p;
        case 2:
            return *(Uint16 *)p;
        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
                return p[0] << 16 | p[1] << 8 | p[2];
            }
            else
            {
                return p[0] | p[1] << 8 | p[2] << 16;
            }
        case 4:
            return *(Uint32 *)p;
        default:
            return 0;
    }
}

static void loadbmpscale(char* filename, SDL_Surface** s)
{
    SDL_Surface* surf = *s;
    SDL_Surface* bmp;
    SDL_Palette pal;
    int w, h;
    unsigned char* data;
    int y;
    char file_path[256];

    if (surf)
    {
        SDL_DestroySurface(surf), surf = *s = NULL;
    }

    SDL_snprintf(file_path, sizeof(file_path), "%s%s", SDL_GetBasePath(), filename);
    bmp = SDL_LoadBMP(file_path);
    if (! bmp)
    {
        SDL_Log("error loading bmp '%s': %s\n", filename, SDL_GetError());
        return;
    }

    w = bmp->w;
    h = bmp->h;

    surf = SDL_CreateSurface(w*scale, h*scale, screen->format);
    assert(surf != NULL);
    data = surf->pixels;

    for (y = 0; y < h; y++)
    {
        int x;
        for (x = 0; x < w; x++)
        {
            unsigned char pix = getpixel(bmp, x, y);
            int i;
            for (i = 0; i < scale; i++)
            {
                int j;
                for (j = 0; j < scale; j++)
                {
                    data[x*scale+i + (y*scale+j)*w*scale] = pix;
                }
            }
        }
    }
    SDL_DestroySurface(bmp);

    pal.colors = (SDL_Color*)base_palette;
    pal.ncolors = 16;

    SDL_SetSurfacePalette(surf, &pal);
    SDL_SetSurfaceColorKey(surf, true, 0);

    *s = surf;
}

#define LOGLOAD(w) SDL_Log("loading %s...", w)
#define LOGDONE() SDL_Log("done")

static void LoadData(void)
{
    LOGLOAD("gfx.bmp");
    loadbmpscale("gfx.bmp", &gfx);
    LOGDONE();

    LOGLOAD("font.bmp");
    loadbmpscale("font.bmp", &font);
    LOGDONE();
}

#include "tilemap.h"

static Uint16 buttons_state = 0;

static void p8_rectfill(int x0, int y0, int x1, int y1, int col);
static void p8_print(const char* str, int x, int y, int col);

static char osd_text[200] = "";
static int  osd_timer = 0;

static void OSDset(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(osd_text, sizeof osd_text, fmt, ap);
    osd_text[sizeof osd_text - 1] = '\0'; //make sure to add NUL terminator in case of truncation
    SDL_Log("%s", osd_text);
    osd_timer = 30;
    va_end(ap);
}

static void OSDdraw(void)
{
    if (osd_timer > 0)
    {
        --osd_timer;
    }

    if (osd_timer > 0)
    {
        const int x = 4;
        const int y = 120 + (osd_timer < 10 ? 10-osd_timer : 0); //disappear by going below the screen
        p8_rectfill(x-2, y-2, x+4*(int)SDL_strlen(osd_text), y+6, 6); //outline
        p8_rectfill(x-1, y-1, x+4*(int)SDL_strlen(osd_text)-1, y+5, 0);
        p8_print(osd_text, x, y, 7);
    }
}

static void Flip(SDL_Surface* screen)
{
    SDL_Rect source = { 0, 0, 128, 128 };
    SDL_Rect dest   = { 24, 40, 128, 128 };

    SDL_UpdateTexture(SDL_screen_tex, NULL, screen->pixels, screen->pitch);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderTexture(renderer, SDL_screen_tex, &source, &dest);
    SDL_RenderPresent(renderer);
}

static bool enable_screenshake = true;
static bool paused = false;
static bool running = true;
static void* initial_game_state = NULL;
static void* game_state = NULL;
static void  mainLoop(void);
static FILE* TAS = NULL;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("N-Gage SDK example", "1.0", "com.example.ngagesdk");

    int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("ngage", NGAGE_W, NGAGE_H, 0, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_PixelFormat format = SDL_GetWindowPixelFormat(window);
    SDL_screen_tex = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, NGAGE_W, NGAGE_H);
    if (!SDL_screen_tex)
    {
        SDL_Log("Couldn't create screen texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    screen = SDL_CreateSurface(PICO8_W, PICO8_H, format);

    ResetPalette();
    SDL_HideCursor();

    SDL_Log("game state size %gkb", Celeste_P8_get_state_size()/1024.);
    SDL_Log("now loading...");

    {
        const unsigned char loading_bmp[] = {
            0x42, 0x4d, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x00,
            0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x09, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00,
            0x00, 0x00, 0x23, 0x2e, 0x00, 0x00, 0x23, 0x2e, 0x00, 0x00, 0x02, 0x00,
            0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x42, 0x47, 0x52, 0x73, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
            0x00, 0x00, 0x66, 0x3e, 0xf1, 0x24, 0xf0, 0x00, 0x00, 0x00, 0x49, 0x44,
            0x92, 0x24, 0x90, 0x00, 0x00, 0x00, 0x49, 0x3c, 0x92, 0x24, 0x90, 0x00,
            0x00, 0x00, 0x49, 0x04, 0x92, 0x24, 0x90, 0x00, 0x00, 0x00, 0x46, 0x38,
            0xf0, 0x3c, 0xf0, 0x00, 0x00, 0x00, 0x40, 0x00, 0x12, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xc0, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        SDL_IOStream* rw = SDL_IOFromConstMem(loading_bmp, sizeof loading_bmp);
        SDL_Surface* loading = SDL_LoadBMP_IO(rw, true);
        SDL_Rect rc = {60, 60};

        if (!loading)
        {
            goto skip_load;
        }

        SDL_BlitSurface(loading, NULL, screen, &rc);
        Flip(screen);
        SDL_DestroySurface(loading);
    }
skip_load:

    LoadData();

    Celeste_P8_set_call_func(pico8emu);

    //for reset
    initial_game_state = SDL_malloc(Celeste_P8_get_state_size());
    if (initial_game_state)
    {
        Celeste_P8_save_state(initial_game_state);
    }

    if (TAS)
    {
        // a consistent seed for tas playback
        Celeste_P8_set_rndseed(8);
    }
    else
    {
        Celeste_P8_set_rndseed((unsigned)(time(NULL) + SDL_GetTicks()));
    }

    Celeste_P8_init();

    return SDL_APP_CONTINUE;
}

// This function runs when a new event (Keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_EVENT_KEY_DOWN:
        {
            switch (event->key.key)
            {
                case SDLK_7:
                case SDLK_UP:
                case SDLK_RIGHT:
                case SDLK_DOWN:
                case SDLK_LEFT:
                    break;
                case SDLK_SOFTLEFT:
                case SDLK_SOFTRIGHT:
                    return SDL_APP_SUCCESS;
                default:
                    return SDL_APP_CONTINUE;
            }
            break;
        }
    }
}

// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

// This function runs once at shutdown.
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (SDL_screen_tex)
    {
        SDL_DestroyTexture(SDL_screen_tex);
    }

    // SDL will clean up the window/renderer for us.
}
