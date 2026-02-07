/*
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <cstdlib>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

typedef struct AppState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Uint64 current_time_ns = 0;
    std::vector<std::string> debug_messages;
} AppState;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    auto *state = static_cast<AppState *>(SDL_calloc(1, sizeof(AppState)));
    if (!state) return SDL_APP_FAILURE;

    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Hello World", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    *appstate = state;

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if ((event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) ||
        event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    /* 初始化AppState */
    auto *state = static_cast<AppState *>(appstate);

    Uint64 current_time_ns = SDL_GetTicksNS();
    Uint64 last_time_ns = state->current_time_ns;
    Uint64 delta_time_ns = current_time_ns - last_time_ns;
    state->current_time_ns = current_time_ns;

    double current_time = current_time_ns / 1000000000.0;
    double last_time = last_time_ns / 1000000000.0;
    double delta_time = delta_time_ns / 1000000000.0;
    double frame_rate = 1.0 / delta_time;
    char message_current_time[64];
    char message_last_time[64];
    char message_delta_time[64];
    char message_frame_rate[64];
    SDL_snprintf(message_current_time, sizeof(message_current_time), "current_time: %.3f", current_time);
    SDL_snprintf(message_last_time, sizeof(message_last_time), "last_time: %.3f", last_time);
    SDL_snprintf(message_delta_time, sizeof(message_delta_time), "delta_time: %.3f", delta_time);
    SDL_snprintf(message_frame_rate, sizeof(message_frame_rate), "frame_rate: %.1f", frame_rate);
    state->debug_messages.emplace_back(message_current_time);
    state->debug_messages.emplace_back(message_last_time);
    state->debug_messages.emplace_back(message_delta_time);
    state->debug_messages.emplace_back(message_frame_rate);

    int w = 0, h = 0;
    float x, y;
    const float scale = 1.0f;

    /* Center the message and scale it up */
    SDL_GetRenderOutputSize(renderer, &w, &h);
    SDL_SetRenderScale(renderer, scale, scale);
    x = ((w / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message_current_time)) / 2;
    y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

    /* 清屏 */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    /* 渲染左上角调试信息 */
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < state->debug_messages.size(); i++) {
        SDL_RenderDebugText(renderer, 0, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * i, state->debug_messages[i].c_str());
    }
    SDL_RenderPresent(renderer);

    /* 帧结束 */
    state->debug_messages.clear();

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
