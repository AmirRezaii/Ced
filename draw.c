#include "draw.h"

int initializeSDL(SDL_Window **window, SDL_Renderer **renderer, int window_width, int window_height) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Create a window
    *window = SDL_CreateWindow(
        "ed",                    /* Window title */
        SDL_WINDOWPOS_CENTERED,  /* Initial x position */
        SDL_WINDOWPOS_CENTERED,  /* Initial y position */
        window_width,                     /* width */
        window_height,                     /* Height */
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN         /* Flags */
    );
    if (*window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    // Create a renderer
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (*renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    return 1;
}

int initializeTTF() {
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }
    return 1;
}

void cleanup(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font) {
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

SDL_Texture *createTextTexture(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color) {
    // Create a text surface
    SDL_Surface *textSurface = TTF_RenderUTF8_Solid(font, text, color);
    if (textSurface == NULL) {
        printf("Unable to render text surface! TTF_Error: %s\n", TTF_GetError());
        return NULL;
    }

    // Create a texture from the surface
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface); /* Free the surface as it's no longer needed */
    if (textTexture == NULL) {
        printf("Unable to create texture from rendered text! SDL_Error: %s\n", SDL_GetError());
        return NULL;
    }

    return textTexture;
}

void renderText(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y) {
    int textWidth, textHeight;
    SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);

    // Render the text
    SDL_Rect renderQuad = {x, y, textWidth, textHeight}; /* Center the text */
    SDL_RenderCopy(renderer, texture, NULL, &renderQuad);
}

void clearScreen(SDL_Renderer *renderer, int r, int g, int b, int a) {
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, r, g, b, a); /* Black background */
    SDL_RenderClear(renderer);
}
