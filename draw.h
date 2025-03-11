#ifndef DRAW_H_
#define DRAW_H_

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>

#define HEX_TO_RGBA(hex) ((hex) >> 24) & 0xff, ((hex) >> 16) & 0xff, ((hex) >> 8) & 0xff, (hex) & 0xff

int initializeSDL(SDL_Window **window, SDL_Renderer **renderer, int window_width, int window_height);
int initializeTTF();
void cleanup(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font);
SDL_Texture *createTextTexture(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color);
void renderText(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y);
void clearScreen(SDL_Renderer *renderer, int r, int g, int b, int a);


#endif // DRAW_H_
