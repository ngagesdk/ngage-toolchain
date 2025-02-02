/* @file main.c
 *
 * SDL3 template project for the Nokia N-Gage.
 */

#define SDL_MAIN_USE_CALLBACKS 1

#define NGAGE_W 176
#define NGAGE_H 208

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

SDL_Window* window;
SDL_Renderer* renderer;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_SetHint("SDL_RENDER_VSYNC", "1");
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("template", "1.0", "com.template.ngagesdk");

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

    // I recommend initialising the audio device in any case,
    // even if you don't need it. The backend is started at a
    // lower level and this ensures that everything is terminated
    // properly.
    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.format = SDL_AUDIO_S16;
    spec.freq = 8000;

    if (!Mix_OpenAudio(0, &spec))
    {
        SDL_Log("Mix_Init: %s", SDL_GetError());
    }

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    return SDL_APP_SUCCESS;
}

// This function runs when a new event (Keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        }
        case SDL_EVENT_KEY_DOWN:
        {
            if (event->key.repeat) // No key repeat.
            {
                break;
            }

            if (event->key.key == SDLK_SOFTLEFT)
            {
                return SDL_APP_SUCCESS;
            }

            else if (event->key.key == SDLK_HASH);
            {
                SDL_RenderClear(renderer);
            }

            break;
        }
    }

    return SDL_APP_CONTINUE;
}

// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void* appstate)
{
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

// This function runs once at shutdown.
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    Mix_CloseAudio();
    Mix_Quit();
    // SDL will clean up the window/renderer for us.
}
