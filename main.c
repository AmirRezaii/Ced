#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <stdio.h>

#include "draw.h"
#include "editor.h"

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 780

#define FONT_WIDTH 15
#define FONT_HEIGHT 36


void renderCursor(SDL_Renderer *renderer, Buffer *buffer, TTF_Font *font, Cursor cursor, Vec2f cam) {
    SDL_Rect rect = {0};
    rect.x = cursor.col*FONT_WIDTH - cam.x;
    rect.y = cursor.line*FONT_HEIGHT - cam.y;
    rect.w = FONT_WIDTH;
    rect.h = FONT_HEIGHT;

    SDL_SetRenderDrawColor(renderer, HEX_TO_RGBA(0xffffffff));
    SDL_RenderFillRect(renderer, &rect);

    if (buffer->lines[cursor.line].text[cursor.col]) {
        SDL_Color letterColor = {HEX_TO_RGBA(0x001b2eff)};
        char c[2];
        c[0] = buffer->lines[cursor.line].text[cursor.col];
        c[1] = '\0';

        SDL_Texture *texture =
            createTextTexture(renderer, font, c, letterColor);
        renderText(renderer, texture, cursor.col * FONT_WIDTH - cam.x,
                   cursor.line * FONT_HEIGHT - cam.y);
        SDL_DestroyTexture(texture);
    }
}

void renderBuffer(SDL_Renderer *renderer, Buffer *buffer, TTF_Font *font, SDL_Color color, Vec2f cam) {
    for (size_t i = 0; i < buffer->count; ++i) {
        if (strlen(buffer->lines[i].text) > 0) {
          SDL_Texture *texture = createTextTexture(
              renderer, font, buffer->lines[i].text, color);
          renderText(renderer, texture, - cam.x, i * FONT_HEIGHT - cam.y);
          SDL_DestroyTexture(texture);
        }
    }
}


char *readFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

int writeStringToFile(const char* filename, const char* string) {
    // Open the file in binary write mode
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        printf("ERROR: Could not open file %s for writing.\n", filename);
        return -1; // Return an error code
    }

    // Write the string to the file
    size_t bytesWritten = fwrite(string, sizeof(char), strlen(string), file);
    if (bytesWritten < strlen(string)) {
        printf("ERROR: Could not write all bytes to file %s.\n", filename);
        fclose(file); // Close the file before returning
        return -1; // Return an error code
    }

    // Close the file
    fclose(file);

    // Return the number of bytes written
    return bytesWritten;
}

void updateCamera(SDL_Window *window, Vec2f *cam, Cursor *cursor) {
        int screen_width;
        int screen_height;
        SDL_GetWindowSize(window, &screen_width, &screen_height);

        int cursor_x = cursor->col * FONT_WIDTH;
        int cursor_y = cursor->line * FONT_HEIGHT;

        Vec2f cam_vel = { .x = FONT_WIDTH*4, .y = FONT_HEIGHT*4 };

        if (cursor_x >= screen_width + cam->x) cam->x += cam_vel.x;
        if (cursor_y >= screen_height + cam->y) cam->y += cam_vel.y;
        if (cursor_x < cam->x) cam->x -= cam_vel.x;
        if (cursor_y < cam->y) cam->y -= cam_vel.y;
}


int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Renderer *renderer;

    if (!initializeSDL(&window, &renderer, WINDOW_WIDTH, WINDOW_HEIGHT)) return 1;
    if (!initializeTTF()) return 1;

    // Load a font
    TTF_Font *font = TTF_OpenFont("./fonts/SpaceMono-Regular.ttf", 24);
    if (font == NULL) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        cleanup(window, renderer, font);
        return 1;
    }

    SDL_StartTextInput();

    // Variables for text input
    SDL_Color textColor = {HEX_TO_RGBA(0xeef3f9ff)}; /* White color (RGBA) */

    Buffer buffer = {0};

    if (argc > 1) {
        char *file_text = readFile(argv[1]);
        if (file_text == NULL) {
          Line line = {0};
          buffAppend(&buffer, line);
        } else {
          buffer = loadToBuffer(file_text);
          free(file_text);
        }
    } else {
        // No file opened. will write to temp.txt
        Line line = {0};
        buffAppend(&buffer, line);
    }

    // Main loop flag
    int running = 1;

    // Event handler
    SDL_Event event;

    Cursor cursor = {.line = 0, .col = 0, .right = 0};
    Vec2f cam = { .x = 0, .y = 0 };

    // Main loop
    while (running) {

        // Handle events on the queue
        while (SDL_PollEvent(&event)) {
          // User requests quit

          switch (event.type) {
          case SDL_QUIT: {
            return 0;
            break;
          }
          case SDL_TEXTINPUT: {
            int len = addTextBeforeCursor(&buffer, event.text.text, cursor);

            cursor.col += len;
            break;
          }
          case SDL_KEYDOWN: {
            switch (event.key.keysym.sym) {
            case SDLK_RIGHT: {
              cursorRight(&buffer, &cursor);
              break;
            }
            case SDLK_LEFT: {
              cursorLeft(&buffer, &cursor);
              break;
            }
            case SDLK_UP: {
              cursorUp(&buffer, &cursor);
              break;
            }
            case SDLK_DOWN: {
              cursorDown(&buffer, &cursor);
              break;
            }
            case SDLK_BACKSPACE: {
              cursorBack(&buffer, &cursor);
              break;
            }
            case SDLK_RETURN: {
              if (!cursorReturn(&buffer, &cursor))
                return 1;
              break;
            }
            case SDLK_TAB: {
              int len = addTextBeforeCursor(&buffer, "    ", cursor);

              cursor.col += len;
              break;
            }
            case SDLK_s: {
              SDL_Keymod mod = SDL_GetModState();
              if (mod & KMOD_CTRL) {
                  if (argc > 1) {
                    char *text = stringFromBuffer(&buffer);
                    int bytes = writeStringToFile(argv[1], text);
                    free(text);
                    printf("Saved %d bytes.\n", bytes);
                  } else {
                    char *text = stringFromBuffer(&buffer);
                    int bytes = writeStringToFile("temp.txt", text);
                    free(text);
                    printf("Saved %d bytes.\n", bytes);
                  }
                break;
              }
            }
            }
            break;
          }
          }
        }

        clearScreen(renderer, HEX_TO_RGBA(0x001b2eff));

        // Render editor buffer
        renderBuffer(renderer, &buffer, font, textColor, cam);

        // Render cursor
        renderCursor(renderer, &buffer, font, cursor, cam);

        // Update camera with appropriate offset
        updateCamera(window, &cam, &cursor);

        // Update the screen
        SDL_RenderPresent(renderer);

    }

    SDL_StopTextInput();

    // Clean up and exit
    cleanup(window, renderer, font);
    buffFree(&buffer);

    return 0;
}
