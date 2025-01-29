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

#define SDL_MAIN_USE_CALLBACKS 1

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "celeste_SDL3.h"

SDL_Window* window;
SDL_Renderer* renderer;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_SetHint("SDL_RENDER_VSYNC", "1");
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("Celeste", "1.3", "com.example.ngagesdk");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Celeste", NGAGE_W, NGAGE_H, 0, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!Init())
    {
        return SDL_APP_FAILURE;
    }
    else
    {
        return SDL_APP_CONTINUE;
    }
}

// This function runs when a new event (Keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    return HandleEvents(event);
}

// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void* appstate)
{
    if (!Iterate())
    {
        return SDL_APP_SUCCESS;
    }
    else
    {
        return SDL_APP_CONTINUE;
    }
}

// This function runs once at shutdown.
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    Destroy();
    // SDL will clean up the window/renderer for us.
}
