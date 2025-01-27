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

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "celeste.h"
#include "celeste_SDL3.h"

static SDL_Window *window;

static bool paused = false;
static void* initial_game_state = NULL;

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
