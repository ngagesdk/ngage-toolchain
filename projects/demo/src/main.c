/* @file main.c
*
* Nokia N-Gage toolchain demo application.
*
* CC0 http://creativecommons.org/publicdomain/zero/1.0/
* SPDX-License-Identifier: CC0-1.0
*
*/

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture* splash = NULL;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("N-Gage SDK example", "1.0", "com.example.ngagesdk");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("ngage", 176, 208, 0, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Surface* splash_bmp = SDL_LoadBMP("E:\\splash.bmp");
    if (splash_bmp == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Couldn't load splash.bmp: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return SDL_APP_FAILURE;
    }

    splash = SDL_CreateTextureFromSurface(renderer, splash_bmp);
    if (!splash)
    {
        SDL_Log("Couldn't create texture from splash.bmp: %s", SDL_GetError());
        SDL_DestroySurface(splash_bmp);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return SDL_APP_FAILURE;
    }
    SDL_DestroySurface(splash_bmp);

    SDL_SetRenderScale(renderer, 0.5f, 0.5f);
    SDL_SetTextureAlphaMod(splash, 127);
    SDL_RenderTextureRotated(renderer, splash, NULL, NULL, 45.f, NULL, SDL_FLIP_HORIZONTAL);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

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
    SDL_DestroyTexture(splash);
    // SDL will clean up the window/renderer for us.
}
