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

#include <stdio.h>
#include <time.h>
#include "SDL3/SDL.h"
#include "celeste_SDL3.h"
#include "celeste.h"
#include "tilemap.h"

extern SDL_Renderer* renderer;

static SDL_Texture* SDL_screen = NULL;
static SDL_Texture* frame = NULL;
static SDL_Surface* screen = NULL;
static SDL_Surface* gfx = NULL;
static SDL_Surface* font = NULL;

static const SDL_Color base_palette_colors[16] =
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

static SDL_Color palette_colors[16];
static SDL_Palette palette;
static Uint32 map[16];

#define getcolor(col) map[col % 16]

static Uint16 buttons_state = 0;
static _Bool enable_screenshake = 1;
static _Bool paused= 0;
static void* initial_game_state = NULL;
static void* game_state = NULL;
#if CELESTE_P8_ENABLE_AUDIO
static Mix_Music* game_state_music   = NULL;
#endif

// On-screen display (for info, such as loading a state, toggling screenshake, toggling fullscreen, etc).
static char osd_text[200] = "";
static int  osd_timer = 0;

static Uint32 getpixel(SDL_Surface* surface, int x, int y);
static int gettileflag(int tile, int flag);
static void loadbmpscale(char* filename, SDL_Surface** s);

static void Flip();
static void LoadData(void);
static void SetPaletteEntry(unsigned char idx, unsigned char base_idx);
static void RefreshPalette(void);
static void ResetPalette(void);

static void OSDset(const char* fmt, ...);
static void OSDdraw(void);

static void p8_line(int x0, int y0, int x1, int y1, unsigned char color);
static void p8_print(const char* str, int x, int y, int col);
static void p8_rectfill(int x0, int y0, int x1, int y1, int col);
static int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);
static inline void Xblit(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect, int color, int flipx, int flipy);

#define LOGLOAD(w) SDL_Log("loading %s...", w)
#define LOGDONE() SDL_Log("done")

int Init()
{
    int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);

    SDL_PixelFormat format = SDL_PIXELFORMAT_ARGB4444;

    SDL_screen = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, PICO8_W, PICO8_H);
    if (!SDL_screen)
    {
        SDL_Log("SDL_CreateTexture: %s\n", SDL_GetError());
        return false;
    }

    screen = SDL_CreateSurface(PICO8_W, PICO8_H, format);
    if (!screen)
    {
        SDL_Log("SDL_CreateSurface: %s\n", SDL_GetError());
        return false;
    }

    ResetPalette();
    SDL_HideCursor();

    SDL_Log("game state size %gkb", Celeste_P8_get_state_size()/1024.);
    SDL_Log("now loading...");

    LoadData();
    Celeste_P8_set_call_func(pico8emu);

    // For reset.
    initial_game_state = SDL_malloc(Celeste_P8_get_state_size());
    if (initial_game_state)
    {
        Celeste_P8_save_state(initial_game_state);
    }

    Celeste_P8_set_rndseed((unsigned)(time(NULL) + SDL_GetTicks()));

    Celeste_P8_init();

    char tmpath[256];
    SDL_snprintf(tmpath, sizeof tmpath, "%sframe.bmp", SDL_GetBasePath());

    SDL_Surface* frame_sf = SDL_LoadBMP(tmpath);
    if (!frame_sf)
    {
        SDL_Log("Failed to load image: frame.bmp", SDL_GetError());
        return false;
    }
    else
    {
        if (0 != SDL_SetSurfaceColorKey(frame_sf, true, SDL_MapSurfaceRGB(frame_sf, 0xff, 0x00, 0xff)))
        {
            SDL_Log("Failed to set color key for frame.bmp: %s", SDL_GetError());
        }

        frame = SDL_CreateTextureFromSurface(renderer, frame_sf);
        if (!frame)
        {
            SDL_Log("Could not create texture from surface: %s", SDL_GetError());
        }
        SDL_DestroySurface(frame_sf);
        SDL_RenderTexture(renderer, frame, NULL, NULL);
    }

    return true;
}

SDL_AppResult HandleEvents(SDL_Event* ev)
{
    switch (ev->type)
    {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        }
        case SDL_EVENT_KEY_DOWN:
        {
            if (ev->key.repeat) // No key repeat.
            {
                break;
            }

            if (ev->key.key == SDLK_SOFTRIGHT) // Do pause.
            {
#if CELESTE_P8_ENABLE_AUDIO
                if (paused)
                {
                    Mix_Resume(-1);
                    Mix_ResumeMusic();
                }
                else
                {
                    Mix_Pause(-1);
                    Mix_PauseMusic();
                }
#endif
                paused = !paused;
            }
            else if (ev->key.key == SDLK_SOFTLEFT) // Exit.
            {
                return SDL_APP_SUCCESS;
            }
            else if (ev->key.key == SDLK_HASH) // Toggle FPS.
            {
                SDL_RenderTexture(renderer, frame, NULL, NULL);
            }
            else if (ev->key.key == SDLK_1) // Save state.
            {
                game_state = game_state ? game_state : SDL_malloc(Celeste_P8_get_state_size());
                if (game_state)
                {
                    OSDset("save state");
                    Celeste_P8_save_state(game_state);
#if CELESTE_P8_ENABLE_AUDIO
                    game_state_music = current_music;
#endif
                    char tmpath[256];
                    SDL_snprintf(tmpath, sizeof(tmpath), "%sceleste.sav", SDL_GetUserFolder(SDL_FOLDER_SAVEDGAMES));
                    FILE* savefile = fopen(tmpath, "wb+");
                    if (savefile)
                    {
                        fwrite(game_state, Celeste_P8_get_state_size(), 1, savefile);
                        fclose(savefile);
                    }
                }
            }
            else if (ev->key.key == SDLK_2) // Load state.
            {
                char tmpath[256];
                SDL_snprintf(tmpath, sizeof(tmpath), "%sceleste.sav", SDL_GetUserFolder(SDL_FOLDER_SAVEDGAMES));
                FILE* savefile = fopen(tmpath, "rb");
                if (savefile)
                {
                    if (!game_state)
                    {
                        game_state = SDL_malloc(Celeste_P8_get_state_size());
                        if (!game_state)
                        {
                            fclose(savefile);
                            break;
                        }
                    }
                    if (game_state)
                    {
                        fread(game_state, Celeste_P8_get_state_size(), 1, savefile);
                    }
                    fclose(savefile);
                }

                if (game_state)
                {
                    OSDset("load state");
#if CELESTE_P8_ENABLE_AUDIO
                    if (paused)
                    {
                        paused = 0;
                        Mix_Resume(-1);
                        Mix_ResumeMusic();
                    }
#endif
                    Celeste_P8_load_state(game_state);
#if CELESTE_P8_ENABLE_AUDIO
                    if (current_music != game_state_music)
                    {
                        Mix_HaltMusic();
                        current_music = game_state_music;
                        if (game_state_music)
                        {
                            Mix_PlayMusic(game_state_music, -1);
                        }
                    }
#endif
                }
            }
            else if (ev->key.key == SDLK_3) // Toggle screenshake.
            {
                enable_screenshake = !enable_screenshake;
                OSDset("screenshake: %s", enable_screenshake ? "on" : "off");
            }
            break;
        }
    }

    return SDL_APP_CONTINUE;
}

int Iterate()
{
    int numkeys;
    const bool* kbstate = SDL_GetKeyboardState(&numkeys);
    static int reset_input_timer  = 0;
    Uint16 prev_buttons_state = buttons_state;

    // Hold C (backspace) to reset.
    if (initial_game_state != NULL && kbstate[SDL_SCANCODE_BACKSPACE])
    {
        reset_input_timer++;
        if (reset_input_timer >= 30)
        {
            reset_input_timer=0;
            OSDset("reset");
            paused = 0;
            Celeste_P8_load_state(initial_game_state);
            Celeste_P8_set_rndseed((unsigned)(time(NULL) + SDL_GetTicks()));
#if CELESTE_P8_ENABLE_AUDIO
            Mix_HaltChannel(-1);
            Mix_HaltMusic();
#endif
            Celeste_P8_init();
        }
    }
    else
    {
        reset_input_timer = 0;
    }

    prev_buttons_state = buttons_state;
    buttons_state = 0;

    if (kbstate[SDL_SCANCODE_LEFT])  buttons_state |= (1 << 0);
    if (kbstate[SDL_SCANCODE_RIGHT]) buttons_state |= (1 << 1);
    if (kbstate[SDL_SCANCODE_UP])    buttons_state |= (1 << 2);
    if (kbstate[SDL_SCANCODE_DOWN])  buttons_state |= (1 << 3);
    if (kbstate[SDL_SCANCODE_7])     buttons_state |= (1 << 4);
    if (kbstate[SDL_SCANCODE_5])     buttons_state |= (1 << 5);

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
    Flip();

    return true;
}

void Destroy()
{
    if (game_state)
    {
        SDL_free(game_state);
    }
    if (initial_game_state)
    {
        SDL_free(initial_game_state);
    }
    if (gfx)
    {
        SDL_DestroySurface(gfx);
    }
    if (font)
    {
        SDL_DestroySurface(font);
    }
    if (frame)
    {
        SDL_DestroyTexture(frame);
    }
    if (SDL_screen)
    {
        SDL_DestroyTexture(SDL_screen);
    }
#if CELESTE_P8_ENABLE_AUDIO
    for (i = 0; i < (sizeof snd)/(sizeof *snd); i++)
    {
        if (snd[i])
        {
            Mix_FreeChunk(snd[i]);
        }
    }
    for (i = 0; i < (sizeof mus)/(sizeof *mus); i++)
    {
        if (mus[i])
        {
            Mix_FreeMusic(mus[i]);
        }
    }

    Mix_CloseAudio();
    Mix_Quit();
#endif
}

static void Flip()
{
    SDL_FRect source = { 0.f, 0.f, 128.f, 128.f };
    SDL_FRect dest   = { 24.f, 25.f, 128.f, 128.f };

    SDL_UpdateTexture(SDL_screen, NULL, screen->pixels, screen->pitch);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_RenderTexture(renderer, SDL_screen, &source, &dest);
    SDL_RenderPresent(renderer);
}

static void LoadData(void)
{
#if CELESTE_P8_ENABLE_AUDIO
    static const char sndids[] = { 0,1,2,3,4,5,6,7,8,9,13,14,15,16,23,35,37,38,40,50,51,54,55 };
    static const char musids[] = { 0,10,20,30,40 };
    int iid;
#endif
    LOGLOAD("gfx.bmp");
    loadbmpscale("gfx.bmp", &gfx);
    LOGDONE();

    LOGLOAD("font.bmp");
    loadbmpscale("font.bmp", &font);
    LOGDONE();

#if CELESTE_P8_ENABLE_AUDIO
    for (iid = 0; iid < sizeof sndids; iid++)
    {
        int  id = sndids[iid];
        char fname[20];
        char path[256];

        SDL_snprintf(fname, 20, "snd%i.wav", id);
        LOGLOAD(fname);
        SDL_snprintf("%s%s", SDL_GetBasePath(), fname);
        snd[id] = Mix_LoadWAV(path);
        if (!snd[id])
        {
            ErrLog("snd%i: Mix_LoadWAV: %s\n", id, Mix_GetError());
        }
        LOGDONE();
    }

    for (iid = 0; iid < sizeof musids; iid++)
    {
        int  id = musids[iid];
        char fname[20];
        char path[256];

        SDL_snprintf(fname, 20, "mus%i.ogg", id);
        LOGLOAD(fname);
        SDL_snprintf("%s%s", SDL_GetBasePath(), fname);
        mus[id / 10] = Mix_LoadMUS(path);
        if (!mus[id / 10])
        {
            ErrLog("mus%i: Mix_LoadMUS: %s\n", id, Mix_GetError());
        }
        LOGDONE();
    }
#endif
}

static void SetPaletteEntry(unsigned char idx, unsigned char base_idx)
{
    palette_colors[idx] = base_palette_colors[base_idx];
    map[idx] = SDL_MapSurfaceRGB(screen, palette_colors[idx].r, palette_colors[idx].g, palette_colors[idx].b);
}

static void RefreshPalette(void)
{
    for (int i = 0; i < SDL_arraysize(map); i++)
    {
        map[i] = SDL_MapSurfaceRGB(screen, palette_colors[i].r, palette_colors[i].g, palette_colors[i].b);
    }
}

static void ResetPalette(void)
{
    SDL_memcpy(palette_colors, base_palette_colors, sizeof(palette_colors));
    RefreshPalette();
}

static void OSDset(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(osd_text, sizeof osd_text, fmt, ap);
    osd_text[sizeof osd_text - 1] = '\0'; // Make sure to add NUL terminator in case of truncation.
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
        const int y = 120 + (osd_timer < 10 ? 10-osd_timer : 0); // Disappear by going below the screen.
        p8_rectfill(x-2, y-2, x+4*(int)SDL_strlen(osd_text), y+6, 6); // Outline.
        p8_rectfill(x-1, y-1, x+4*(int)SDL_strlen(osd_text)-1, y+5, 0);
        p8_print(osd_text, x, y, 7);
    }
}

static int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...)
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
#define BOOL_ARG() (Celeste_P8_bool_t)va_arg(args, int)
#define RET_INT(_i)   do {ret = (_i); goto end;} while (0)
#define RET_BOOL(_b) RET_INT(!!(_b))

    switch (call)
    {
        case CELESTE_P8_MUSIC: //music(idx,fade,mask)
        {
#if CELESTE_P8_ENABLE_AUDIO
            int index = INT_ARG();
            int fade = INT_ARG();
            int mask = INT_ARG();

            (void)mask; //we do not care about this since sdl mixer keeps sounds and music separate

            if (index == -1) { //stop playing
                Mix_FadeOutMusic(fade);
                current_music = NULL;
            }
            else if (mus[index / 10])
            {
                Mix_Music* musi = mus[index / 10];
                current_music = musi;
                Mix_FadeInMusic(musi, -1, fade);
            }
#endif
            break;
        }
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

            SDL_assert(rows == 1 && cols == 1);

            if (sprite >= 0)
            {
                SDL_Rect srcrc =
                {
                    8 * (sprite % 16),
                    8 * (sprite / 16)
                };
                SDL_Rect dstrc =
                {
                    (x - camera_x) * SCALE, (y - camera_y) * SCALE,
                    SCALE, SCALE
                };
                srcrc.x *= SCALE;
                srcrc.y *= SCALE;
                srcrc.w = srcrc.h = SCALE * 8;
                Xblit(gfx, &srcrc, screen, &dstrc, 0, flipx, flipy);
            }
            break;
        }
        case CELESTE_P8_BTN: //btn(b)
        {
            int b = INT_ARG();
            SDL_assert(b >= 0 && b <= 5);
            RET_BOOL(buttons_state & (1 << b));
            break;
        }
        case CELESTE_P8_SFX: //sfx(id)
        {
#if CELESTE_P8_ENABLE_AUDIO
            int id = INT_ARG();

            if (id < (sizeof snd) / (sizeof * snd) && snd[id])
            {
                Mix_PlayChannel(-1, snd[id], 0);
            }
#endif
            break;
        }
        case CELESTE_P8_PAL: //pal(a,b)
        {
            int a = INT_ARG();
            int b = INT_ARG();
            if (a >= 0 && a < 16 && b >= 0 && b < 16)
            {
                // Swap palette colors.
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
                SDL_Rect rect_a = { SCALE * (cx - 1), SCALE * cy, SCALE * 3, SCALE };
                SDL_Rect rect_b = { SCALE * cx, SCALE * (cy - 1), SCALE, SCALE * 3 };

                SDL_FillSurfaceRect(screen, &rect_a, realcolor);
                SDL_FillSurfaceRect(screen, &rect_b, realcolor);
            }
            else if (r <= 2)
            {
                SDL_Rect rect_a = { SCALE * (cx - 2), SCALE * (cy - 1), SCALE * 5, SCALE * 3 };
                SDL_Rect rect_b = { SCALE * (cx - 1), SCALE * (cy - 2), SCALE * 3, SCALE * 5 };

                SDL_FillSurfaceRect(screen, &rect_a, realcolor);
                SDL_FillSurfaceRect(screen, &rect_b, realcolor);
            }
            else if (r <= 3)
            {
                SDL_Rect rect_a = { SCALE * (cx - 3), SCALE * (cy - 1), SCALE * 7, SCALE * 3 };
                SDL_Rect rect_b = { SCALE * (cx - 1), SCALE * (cy - 3), SCALE * 3, SCALE * 7 };
                SDL_Rect rect_c = { SCALE * (cx - 2), SCALE * (cy - 2), SCALE * 5, SCALE * 5 };

                SDL_FillSurfaceRect(screen, &rect_a, realcolor);
                SDL_FillSurfaceRect(screen, &rect_b, realcolor);
                SDL_FillSurfaceRect(screen, &rect_c, realcolor);
            }
            else  // I dont think the game uses this.
            {
                int f = 1 - r; // Used to track the progress of the drawn circle (since its semi-recursive).
                int ddFx = 1;   // Step x.
                int ddFy = -2 * r; // Step y.
                int x = 0;
                int y = r;

                // This algorithm doesn't account for the diameters
                // so we have to set them manually
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

                    // Build our current arc.
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
        case CELESTE_P8_LINE: // line(x0,y0,x1,y1,col)
        {
            int x0 = INT_ARG() - camera_x;
            int y0 = INT_ARG() - camera_y;
            int x1 = INT_ARG() - camera_x;
            int y1 = INT_ARG() - camera_y;
            int col = INT_ARG();

            p8_line(x0, y0, x1, y1, col);
            break;
        }
        case CELESTE_P8_MGET: // mget(tx,ty)
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
                    // Hack.
                    if (mask == 0 || (mask == 4 && tile_flags[tile] == 4) || gettileflag(tile, mask != 4 ? mask - 1 : mask))
                    {
                        SDL_Rect srcrc =
                        {
                        8 * (tile % 16),
                        8 * (tile / 16)
                        };
                        SDL_Rect dstrc =
                        {
                        (tx + x * 8 - camera_x) * SCALE, (ty + y * 8 - camera_y) * SCALE,
                        SCALE * 8, SCALE * 8
                        };
                        srcrc.x *= SCALE;
                        srcrc.y *= SCALE;
                        srcrc.w = srcrc.h = SCALE * 8;

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

static Uint32 getpixel(SDL_Surface* surface, int x, int y)
{
    int bpp = SDL_BYTESPERPIXEL(surface->format);
    /* Here p is the address to the pixel we want to retrieve */
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    if (bpp == 2) // RGB565.
    {
        Uint16 pixel = *(Uint16*)p;

        // Extract RGB565 channels.
        Uint8 r5 = (pixel >> 11) & 0x1F; // 5 bits for red.
        Uint8 g6 = (pixel >> 5) & 0x3F;  // 6 bits for green.
        Uint8 b5 = pixel & 0x1F;         // 5 bits for blue.

        // Scale to 4 bits for ARGB4444.
        Uint8 r4 = (r5 * 15) / 31;
        Uint8 g4 = (g6 * 15) / 63;
        Uint8 b4 = (b5 * 15) / 31;

        Uint8 a4;
        Uint32 key;
        SDL_GetSurfaceColorKey(surface, &key);
        if (pixel == key)
        {
            a4 = 0;
        }
        else
        {
            // Set alpha to 15 (fully opaque).
            a4 = 15;
        }

        // Combine into ARGB4444.
        return (a4 << 12) | (r4 << 8) | (g4 << 4) | b4;
    }
    else if (bpp == 4) // ARGB8888, passthrough.
    {
        return *(Uint32*)p;
    }
    else // Unsupported format.
    {
        return 0;
    }
}

static int gettileflag(int tile, int flag)
{
    return tile < sizeof(tile_flags) / sizeof(*tile_flags) && (tile_flags[tile] & (1 << flag)) != 0;
}

static void loadbmpscale(char* filename, SDL_Surface** s)
{
    SDL_Surface* surf = *s;
    char tmpath[256];
    SDL_Surface* bmp;
    int w, h;
    Uint16* data;

    if (surf)
    {
        SDL_DestroySurface(surf), surf = *s = NULL;
    }

    SDL_snprintf(tmpath, sizeof tmpath, "%s%s", SDL_GetBasePath(), filename);

    bmp = SDL_LoadBMP(tmpath);
    if (!bmp)
    {
        SDL_Log("Error loading bmp '%s': %s\n", filename, SDL_GetError());
        return;
    }

    w = bmp->w;
    h = bmp->h;

    surf = SDL_CreateSurface(w * SCALE, h * SCALE, screen->format);
    SDL_assert(surf != NULL);
    data = (Uint16*)surf->pixels;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            Uint16 pix = (Uint16)getpixel(bmp, x, y);
            for (int i = 0; i < SCALE; i++)
            {
                for (int j = 0; j < SCALE; j++)
                {
                    data[(x * SCALE + i) + (y * SCALE + j) * w * SCALE] = pix;
                }
            }
        }
    }
    SDL_DestroySurface(bmp);

    palette.colors = palette_colors;
    palette.ncolors = 16;

    SDL_SetSurfacePalette(surf, &palette);
    SDL_SetSurfaceColorKey(surf, true, 0);

    *s = surf;
}

// Coordinates should NOT be scaled before calling this.
static void p8_line(int x0, int y0, int x1, int y1, unsigned char color)
{
    Uint32 realcolor = getcolor(color);
    int sx, sy, dx, dy, err;
    SDL_Rect rect;

#define CLAMP(v,min,max) v = v < min ? min : v >= max ? max-1 : v;
    CLAMP(x0, 0, screen->w);
    CLAMP(y0, 0, screen->h);
    CLAMP(x1, 0, screen->w);
    CLAMP(y1, 0, screen->h);

#undef CLAMP
#define PLOT(xa, ya) \
    rect.x = xa * SCALE; \
    rect.y = ya * SCALE; \
    rect.w = SCALE; \
    rect.h = SCALE; \
    do { \
        SDL_FillSurfaceRect(screen, &rect, realcolor); \
    } \
    while (0)
    dx = SDL_abs(x1 - x0);
    dy = SDL_abs(y1 - y0);
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

static void p8_print(const char* str, int x, int y, int col)
{
    for (char c = *str; c; c = *(++str))
    {
        SDL_Rect srcrc;
        SDL_Rect dstrc;
        c &= 0x7F;
        srcrc.x = 8 * (c % 16);
        srcrc.y = 8 * (c / 16);
        srcrc.x *= SCALE;
        srcrc.y *= SCALE;
        srcrc.w = srcrc.h = 8 * SCALE;

        dstrc.x = x * SCALE;
        dstrc.y = y * SCALE;
        dstrc.w = SCALE;
        dstrc.h = SCALE;

        Xblit(font, &srcrc, screen, &dstrc, col, 0, 0);
        x += 4;
    }
}

static void p8_rectfill(int x0, int y0, int x1, int y1, int col)
{
    int w = (x1 - x0 + 1) * SCALE;
    int h = (y1 - y0 + 1) * SCALE;
    if (w > 0 && h > 0)
    {
        SDL_Rect rc = { x0 * SCALE, y0 * SCALE, w, h };
        SDL_FillSurfaceRect(screen, &rc, getcolor(col));
    }
}

//lots of code from https://github.com/SDL-mirror/SDL/blob/bc59d0d4a2c814900a506d097a381077b9310509/src/video/SDL_surface.c#L625
//coordinates should be scaled already
static inline void Xblit(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect, int color, int flipx, int flipy)
{
    SDL_Rect fulldst;
    int srcx, srcy, w, h;
    SDL_assert(SDL_BYTESPERPIXEL(dst->format) == 2);

    if (!dstrect)
    {
        dstrect = (fulldst = (SDL_Rect){ 0,0,dst->w,dst->h }, &fulldst);
    }

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

    SDL_Rect clip;
    SDL_GetSurfaceClipRect(dst, &clip);
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

    if (w && h)
    {
        Uint16* srcpix = (Uint16*)src->pixels;
        Uint16* dstpix = (Uint16*)dst->pixels;
        int srcpitch = src->pitch / 2;
        int dstpitch = dst->pitch / 2;
        int x, y;

        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
            {
                Uint16 p = srcpix[!flipx ? srcx + x + (srcy + y) * srcpitch : srcx + (w - x - 1) + (srcy + y) * srcpitch];
                if (p)
                {
                    dstpix[dstrect->x + x + (dstrect->y + y) * dstpitch] = p;
                }
            }
        }
    }
}
