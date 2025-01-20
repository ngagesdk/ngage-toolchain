/* @file main.c
*
* Nokia N-Gage toolchain demo application.
*
* CC0 http://creativecommons.org/publicdomain/zero/1.0/
* SPDX-License-Identifier: CC0-1.0
*
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture* splash;
static SDL_AudioStream *stream;
static Uint8 *wav_data;
static Uint32 wav_data_len;
static float prev_angle;
static float angle;
static SDL_FlipMode prev_flip;
static SDL_FlipMode flip;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    char file_path[256];

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("N-Gage SDK example", "1.0", "com.example.ngagesdk");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("ngage", 176, 208, 0, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_snprintf(file_path, sizeof(file_path), "%s/splash.bmp", SDL_GetBasePath());

    SDL_Surface* splash_bmp = SDL_LoadBMP(file_path);
    if (splash_bmp == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Couldn't load splash.bmp: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    splash = SDL_CreateTextureFromSurface(renderer, splash_bmp);
    if (!splash)
    {
        SDL_Log("Couldn't create texture from ngage.bmp: %s", SDL_GetError());
        SDL_DestroySurface(splash_bmp);
        return SDL_APP_FAILURE;
    }
    SDL_DestroySurface(splash_bmp);

    SDL_AudioSpec spec;
    SDL_snprintf(file_path, sizeof(file_path), "%s/music.wav", SDL_GetBasePath());

    if (!SDL_LoadWAV(file_path, &spec, &wav_data, &wav_data_len))
    {
        SDL_Log("Couldn't load .wav file: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream)
    {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_ResumeAudioStreamDevice(stream);

    SDL_SetRenderDrawColor(renderer, 0x04, 0x02, 0x04, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, splash, NULL, NULL);
    SDL_RenderPresent(renderer);

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
                    flip = flip + 1;
                    if (flip > SDL_FLIP_VERTICAL)
                    {
                        flip = SDL_FLIP_NONE;
                    }
                case SDLK_UP:
                    angle = 0.f;
                    break;
                case SDLK_RIGHT:
                    angle = 90.f;
                    break;
                case SDLK_DOWN:
                    angle = 180.f;
                    break;
                case SDLK_LEFT:
                    angle = 270.f;
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
    if (angle != prev_angle || flip != prev_flip)
    {
        SDL_RenderClear(renderer);
        SDL_RenderTextureRotated(renderer, splash, NULL, NULL, angle, NULL, flip);
        prev_angle = angle;
        prev_flip = flip;
    }

    if (SDL_GetAudioStreamAvailable(stream) < (int)wav_data_len)
    {
        SDL_Log("Audio stream underflow, rewinding.");
        SDL_PutAudioStreamData(stream, wav_data, wav_data_len);
    }

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

// This function runs once at shutdown.
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_free(wav_data);
    SDL_DestroyTexture(splash);
    // SDL will clean up the window/renderer for us.
}
