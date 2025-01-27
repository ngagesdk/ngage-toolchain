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
#include <time.h>

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

    const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(screen->format);
    map[idx] = SDL_MapRGB(details, NULL, palette[idx].r, palette[idx].g, palette[idx].b);
}

static void RefreshPalette(void)
{
    int i;
    const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(screen->format);

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
    SDL_FRect source = { 0, 0, 128, 128 };
    SDL_FRect dest   = { 24, 40, 128, 128 };

    SDL_UpdateTexture(SDL_screen_tex, NULL, screen->pixels, screen->pitch);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, SDL_screen_tex, &source, &dest);
    SDL_RenderPresent(renderer);
}

static bool enable_screenshake = true;
static bool paused = false;
static bool running = true;
static void* initial_game_state = NULL;
static void* game_state = NULL;
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
    SDL_screen_tex = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, PICO8_W, PICO8_H);
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
    //const bool* kbstate = SDL_GetKeyboardState(NULL);
    static int reset_input_timer  = 0;
    Uint16 prev_buttons_state = buttons_state;

    //if (initial_game_state != NULL)
    //{
    //    reset_input_timer++;
    //    if (reset_input_timer >= 30)
    //    {
    //        reset_input_timer=0;
    //        //reset
    //        OSDset("reset");
    //        paused = 0;
    //        Celeste_P8_load_state(initial_game_state);
    //        Celeste_P8_set_rndseed((unsigned)(time(NULL) + SDL_GetTicks()));
    //        Celeste_P8_init();
    //    }
    //}
    //else
    //{
    //    reset_input_timer = 0;
    //}

    prev_buttons_state = buttons_state;
    buttons_state = 0;

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
    if (paused)
    {
        const int x0 = PICO8_W/2-3*4, y0 = 8;

        p8_rectfill(x0-1,y0-1, 6*4+x0+1,6+y0+1, 6);
        p8_rectfill(x0,y0, 6*4+x0,6+y0, 0);
        p8_print("paused", x0+1, y0+1, 7);
    }
    else
    {
        Celeste_P8_update();
        Celeste_P8_draw();
    }
    OSDdraw();
    Flip(screen);

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

static int gettileflag(int tile, int flag);
static void p8_line(int x0, int y0, int x1, int y1, unsigned char color);

//lots of code from https://github.com/SDL-mirror/SDL/blob/bc59d0d4a2c814900a506d097a381077b9310509/src/video/SDL_surface.c#L625
//coordinates should be scaled already
static inline void Xblit(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect, int color, int flipx, int flipy)
{
    SDL_Rect fulldst;
    int srcx, srcy, w, h;

    assert(src && dst && !src->locked && !dst->locked);

    int src_bpp = SDL_BYTESPERPIXEL(src->format);
    int dst_bpp = SDL_BYTESPERPIXEL(dst->format);

    assert(src_bpp == 8);
    assert(dst_bpp == 2 || dst_bpp == 4);
    /* If the destination rectangle is NULL, use the entire dest surface */
    if (!dstrect)
    {
        dstrect = (fulldst = (SDL_Rect){ 0,0,dst->w,dst->h }, &fulldst);
    }

    /* clip the source rectangle to the source surface */
    if (srcrect)
    {
        int maxw, maxh;

        srcx = srcrect->x;
        w = srcrect->w;
        if (srcx < 0)
        {
            w += srcx;
            dstrect->x -= srcx;
            srcx = 0;
        }
        maxw = src->w - srcx;
        if (maxw < w)
        {
            w = maxw;
        }

        srcy = srcrect->y;
        h = srcrect->h;
        if (srcy < 0)
        {
            h += srcy;
            dstrect->y -= srcy;
            srcy = 0;
        }
        maxh = src->h - srcy;
        if (maxh < h)
        {
            h = maxh;
        }

    }
    else
    {
        srcx = srcy = 0;
        w = src->w;
        h = src->h;
    }

    /* clip the destination rectangle against the clip rectangle */
    {
        SDL_Rect clip;
        clip.x = 0;
        clip.y = 0;
        clip.w = dst->w;
        clip.h = dst->h;

        int dx, dy;

        dx = clip.x - dstrect->x;
        if (dx > 0)
        {
            w -= dx;
            dstrect->x += dx;
            srcx += dx;
        }
        dx = dstrect->x + w - clip.x - clip.w;
        if (dx > 0)
        {
            w -= dx;
        }

        dy = clip.y - dstrect->y;
        if (dy > 0)
        {
            h -= dy;
            dstrect->y += dy;
            srcy += dy;
        }
        dy = dstrect->y + h - clip.y - clip.h;
        if (dy > 0)
        {
            h -= dy;
        }
    }

    if (w && h)
    {
        unsigned char* srcpix = src->pixels;
        int srcpitch = src->pitch;
        int x, y;

#define _blitter(dsttype, dp, xflip) do { \
            dsttype* dstpix = dst->pixels; \
            for (y = 0; y < h; y++) \
            { \
                for (x = 0; x < w; x++) \
                { \
                    unsigned char p = srcpix[!xflip ? srcx+x+(srcy+y)*srcpitch : srcx+(w-x-1)+(srcy+y)*srcpitch]; \
                    if (p) dstpix[dstrect->x+x + (dstrect->y+y)*dst->w] = getcolor(dp); \
                } \
            } \
            } while(0)

        int screen_bpp = SDL_BYTESPERPIXEL(screen->format);
        if (screen_bpp == 2 && color && flipx)
        {
            _blitter(Uint16, color, 1);
        }
        else if (screen_bpp == 2 && !color && flipx)
        {
            _blitter(Uint16, p, 1);
        }
        else if (screen_bpp == 2 && color && !flipx)
        {
            _blitter(Uint16, color, 0);
        }
        else if (screen_bpp == 2 && !color && !flipx)
        {
            _blitter(Uint16, p, 0);
        }
        else if (screen_bpp == 4 && color && flipx)
        {
            _blitter(Uint32, color, 1);
        }
        else if (screen_bpp == 4 && !color && flipx)
        {
            _blitter(Uint32, p, 1);
        }
        else if (screen_bpp == 4 && color && !flipx)
        {
            _blitter(Uint32, color, 0);
        }
        else if (screen_bpp == 4 && !color && !flipx)
        {
            _blitter(Uint32, p, 0);
        }
#undef _blitter
    }
}

static void p8_rectfill(int x0, int y0, int x1, int y1, int col)
{
    int w = (x1 - x0 + 1) * scale;
    int h = (y1 - y0 + 1) * scale;
    if (w > 0 && h > 0)
    {
        SDL_Rect rc = { x0 * scale,y0 * scale, w,h };
        SDL_FillSurfaceRect(screen, &rc, getcolor(col));
    }
}

static void p8_print(const char* str, int x, int y, int col)
{
    char c;
    for (c = *str; c; c = *(++str))
    {
        SDL_Rect srcrc;
        SDL_Rect dstrc;
        c &= 0x7F;
        srcrc.x = 8 * (c % 16);
        srcrc.y = 8 * (c / 16);
        srcrc.x *= scale;
        srcrc.y *= scale;
        srcrc.w = srcrc.h = 8 * scale;

        dstrc.x = x * scale;
        dstrc.y = y * scale;
        dstrc.w = scale;
        dstrc.h = scale;

        Xblit(font, &srcrc, screen, &dstrc, col, 0, 0);
        x += 4;
    }
}

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...)
{
    static int camera_x = 0, camera_y = 0;
    va_list    args;
    int        ret = 0;

    if (!enable_screenshake)
    {
        camera_x = camera_y = 0;
    }

    va_start(args, call);

#define INT_ARG() va_arg(args, int)
#define BOOL_ARG() (bool)va_arg(args, int)
#define RET_INT(_i) do {ret = (_i); goto end;} while (0)
#define RET_BOOL(_b) RET_INT(!!(_b))

    switch (call)
    {
        case CELESTE_P8_SPR: //spr(sprite,x,y,cols,rows,flipx,flipy)
        {
            int sprite = INT_ARG();
            int x = INT_ARG();
            int y = INT_ARG();
            int cols = INT_ARG();
            int rows = INT_ARG();
            int flipx = BOOL_ARG();
            int flipy = BOOL_ARG();

            (void)cols;
            (void)rows;

            assert(rows == 1 && cols == 1);

            if (sprite >= 0)
            {
                SDL_Rect srcrc = {
                    8 * (sprite % 16),
                    8 * (sprite / 16)
                };
                SDL_Rect dstrc =
                {
                (x - camera_x) * scale, (y - camera_y) * scale,
                scale, scale
                };
                srcrc.x *= scale;
                srcrc.y *= scale;
                srcrc.w = srcrc.h = scale * 8;
                Xblit(gfx, &srcrc, screen, &dstrc, 0, flipx, flipy);
            }
            break;
        }
        case CELESTE_P8_BTN: //btn(b)
        {
            int b = INT_ARG();
            assert(b >= 0 && b <= 5);
            RET_BOOL(buttons_state & (1 << b));
            break;
        }
        case CELESTE_P8_PAL: //pal(a,b)
        {
            int a = INT_ARG();
            int b = INT_ARG();
            if (a >= 0 && a < 16 && b >= 0 && b < 16)
            {
                //swap palette colors
                SetPaletteEntry(a, b);
            }
            break;
        }
        case CELESTE_P8_PAL_RESET: //pal()
        {
            ResetPalette();
            break;
        }
        case CELESTE_P8_CIRCFILL: //circfill(x,y,r,col)
        {
            int cx = INT_ARG() - camera_x;
            int cy = INT_ARG() - camera_y;
            int r = INT_ARG();
            int col = INT_ARG();
            int realcolor = getcolor(col);

            if (r <= 1)
            {
                SDL_Rect rect_a = { scale * (cx - 1), scale * cy, scale * 3, scale };
                SDL_Rect rect_b = { scale * cx, scale * (cy - 1), scale, scale * 3 };

                SDL_FillSurfaceRect(screen, &rect_a, realcolor);
                SDL_FillSurfaceRect(screen, &rect_b, realcolor);
            }
            else if (r <= 2)
            {
                SDL_Rect rect_a = { scale * (cx - 2), scale * (cy - 1), scale * 5, scale * 3 };
                SDL_Rect rect_b = { scale * (cx - 1), scale * (cy - 2), scale * 3, scale * 5 };

                SDL_FillSurfaceRect(screen, &rect_a, realcolor);
                SDL_FillSurfaceRect(screen, &rect_b, realcolor);
            }
            else if (r <= 3)
            {
                SDL_Rect rect_a = { scale * (cx - 3), scale * (cy - 1), scale * 7, scale * 3 };
                SDL_Rect rect_b = { scale * (cx - 1), scale * (cy - 3), scale * 3, scale * 7 };
                SDL_Rect rect_c = { scale * (cx - 2), scale * (cy - 2), scale * 5, scale * 5 };

                SDL_FillSurfaceRect(screen, &rect_a, realcolor);
                SDL_FillSurfaceRect(screen, &rect_b, realcolor);
                SDL_FillSurfaceRect(screen, &rect_c, realcolor);
            }
            else  //i dont think the game uses this
            {
                int f = 1 - r; //used to track the progress of the drawn circle (since its semi-recursive)
                int ddFx = 1;   //step x
                int ddFy = -2 * r; //step y
                int x = 0;
                int y = r;

                //this algorithm doesn't account for the diameters
                //so we have to set them manually
                p8_line(cx, cy - y, cx, cy + r, col);
                p8_line(cx + r, cy, cx - r, cy, col);

                while (x < y)
                {
                    if (f >= 0)
                    {
                        y--;
                        ddFy += 2;
                        f += ddFy;
                    }
                    x++;
                    ddFx += 2;
                    f += ddFx;

                    //build our current arc
                    p8_line(cx + x, cy + y, cx - x, cy + y, col);
                    p8_line(cx + x, cy - y, cx - x, cy - y, col);
                    p8_line(cx + y, cy + x, cx - y, cy + x, col);
                    p8_line(cx + y, cy - x, cx - y, cy - x, col);
                }
            }
            break;
        }
        case CELESTE_P8_PRINT: //print(str,x,y,col)
        {
            const char* str = va_arg(args, const char*);
            int x = INT_ARG() - camera_x;
            int y = INT_ARG() - camera_y;
            int col = INT_ARG() % 16;

            p8_print(str, x, y, col);
            break;
        }
        case CELESTE_P8_RECTFILL: //rectfill(x0,y0,x1,y1,col)
        {
            int x0 = INT_ARG() - camera_x;
            int y0 = INT_ARG() - camera_y;
            int x1 = INT_ARG() - camera_x;
            int y1 = INT_ARG() - camera_y;
            int col = INT_ARG();

            p8_rectfill(x0, y0, x1, y1, col);
            break;
        }
        case CELESTE_P8_LINE: //line(x0,y0,x1,y1,col)
        {
            int x0 = INT_ARG() - camera_x;
            int y0 = INT_ARG() - camera_y;
            int x1 = INT_ARG() - camera_x;
            int y1 = INT_ARG() - camera_y;
            int col = INT_ARG();

            p8_line(x0, y0, x1, y1, col);
            break;
        }
        case CELESTE_P8_MGET: //mget(tx,ty)
        {
            int tx = INT_ARG();
            int ty = INT_ARG();

            RET_INT(tilemap_data[tx + ty * 128]);
            break;
        }
        case CELESTE_P8_CAMERA: //camera(x,y)
        {
            if (enable_screenshake)
            {
                camera_x = INT_ARG();
                camera_y = INT_ARG();
            }
            break;
        }
        case CELESTE_P8_FGET: //fget(tile,flag)
        {
            int tile = INT_ARG();
            int flag = INT_ARG();

            RET_INT(gettileflag(tile, flag));
            break;
        }
        case CELESTE_P8_MAP: //map(mx,my,tx,ty,mw,mh,mask)
        {
            int mx = INT_ARG(), my = INT_ARG();
            int tx = INT_ARG(), ty = INT_ARG();
            int mw = INT_ARG(), mh = INT_ARG();
            int mask = INT_ARG();
            int x, y;

            for (x = 0; x < mw; x++)
            {
                for (y = 0; y < mh; y++)
                {
                    int tile = tilemap_data[x + mx + (y + my) * 128];
                    //hack
                    if (mask == 0 || (mask == 4 && tile_flags[tile] == 4) || gettileflag(tile, mask != 4 ? mask - 1 : mask))
                    {
                        SDL_Rect srcrc =
                        {
                            8 * (tile % 16),
                            8 * (tile / 16)
                        };
                        SDL_Rect dstrc =
                        {
                            (tx + x * 8 - camera_x) * scale, (ty + y * 8 - camera_y) * scale,
                            scale * 8, scale * 8
                        };
                        srcrc.x *= scale;
                        srcrc.y *= scale;
                        srcrc.w = srcrc.h = scale * 8;

                        /*
                        if (0)
                        {
                            srcrc.x = srcrc.y = 0;
                            srcrc.w = srcrc.h = 8;
                            dstrc.x = x*8, dstrc.y = y*8;
                            dstrc.w = dstrc.h = 8;
                        }
                        */

                        Xblit(gfx, &srcrc, screen, &dstrc, 0, 0, 0);
                    }
                }
            }
            break;
        }
    }

end:
    va_end(args);
    return ret;
}

static int gettileflag(int tile, int flag)
{
    return tile < sizeof(tile_flags) / sizeof(*tile_flags) && (tile_flags[tile] & (1 << flag)) != 0;
}

//coordinates should NOT be scaled before calling this
static void p8_line(int x0, int y0, int x1, int y1, unsigned char color)
{
    Uint32   realcolor = getcolor(color);
    int      sx, sy, dx, dy, err;
    SDL_Rect rect;

#define CLAMP(v,min,max) v = v < min ? min : v >= max ? max-1 : v;
    CLAMP(x0, 0, screen->w);
    CLAMP(y0, 0, screen->h);
    CLAMP(x1, 0, screen->w);
    CLAMP(y1, 0, screen->h);

#undef CLAMP
#define PLOT(xa, ya) \
    rect.x = xa * scale; \
    rect.y = ya * scale; \
    rect.w = scale; \
    rect.h = scale; \
    do { \
        SDL_FillSurfaceRect(screen, &rect, realcolor); \
    } \
    while (0)
    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    if (!dx && !dy)
    {
        return;
    }

    if (x0 < x1)
    {
        sx = 1;
    }
    else
    {
        sx = -1;
    }
    if (y0 < y1)
    {
        sy = 1;
    }
    else
    {
        sy = -1;
    }
    err = dx - dy;
    if (!dy && !dx)
    {
        return;
    }
    else if (!dx) //vertical line
    {
        int y;
        for (y = y0; y != y1; y += sy)
        {
            PLOT(x0, y);
        }
    }
    else if (!dy) //horizontal line
    {
        int x;
        for (x = x0; x != x1; x += sx)
        {
            PLOT(x, y0);
        }
    }

    while (x0 != x1 || y0 != y1)
    {
        int e2;
        PLOT(x0, y0);
        e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
#undef PLOT
}
