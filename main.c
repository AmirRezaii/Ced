#include <SDL2/SDL_clipboard.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "draw.h"
#include "editor.h"

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 780

#define FONT_WIDTH 15
#define FONT_HEIGHT 36

void resetSelect(_Bool *selecting, Cursor *select, Cursor *cursor) {
    *selecting = 0;
    select->line = cursor->line;
    select->col  = cursor->col;
}


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
    // Rendering only lines which are on screen.
    size_t first_in_view = (size_t)cam.y / FONT_HEIGHT;
    size_t last_in_view = first_in_view + 1 + cam.h / FONT_HEIGHT;
    size_t last = last_in_view <= buffer->count ? last_in_view : buffer->count;

    for (size_t i = first_in_view; i < last; ++i) {
        if (strlen(buffer->lines[i].text) > 0) {
          SDL_Texture *texture = createTextTexture(
              renderer, font, buffer->lines[i].text, color);
          renderText(renderer, texture, - cam.x, i * FONT_HEIGHT - cam.y);
          SDL_DestroyTexture(texture);
        }
    }
}

void renderSelect(SDL_Renderer *renderer, Cursor select, Cursor cursor, Vec2f cam) {
    SDL_Rect rect = {0};
    rect.h = FONT_HEIGHT;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, HEX_TO_RGBA(0xffff33aa)); /* Selection Color */

    if (select.line == cursor.line) {
        if (cursor.col >= select.col) {
            rect.w = (cursor.col - select.col)*FONT_WIDTH;
            rect.x = select.col*FONT_WIDTH - cam.x;
        } else {
            rect.w = (select.col - cursor.col)*FONT_WIDTH;
            rect.x = cursor.col*FONT_WIDTH - cam.x;
        }
        rect.y = select.line * FONT_HEIGHT - cam.y;
        SDL_RenderFillRect(renderer, &rect);
    } else if (select.line < cursor.line) {
        for (size_t i = select.line; i <= cursor.line; ++i) {
            if (i == select.line) {
              rect.w = cam.w - select.col * FONT_WIDTH;
              rect.x = select.col * FONT_WIDTH - cam.x;
            } else {
              if (i < cursor.line) {
                rect.w = cam.w;
              } else {
                rect.w = cursor.col * FONT_WIDTH;
              }
              rect.x = 0 - cam.x;
            }

            rect.y = i * FONT_HEIGHT - cam.y;
            SDL_RenderFillRect(renderer, &rect);
        }
    } else {
        for (size_t i = cursor.line; i <= select.line; ++i) {
            if (i == cursor.line) {
              rect.w = cam.w - cursor.col * FONT_WIDTH;
              rect.x = cursor.col * FONT_WIDTH - cam.x;
            } else {
              if (i < select.line) {
                rect.w = cam.w;
              } else {
                rect.w = select.col * FONT_WIDTH;
              }
              rect.x = 0 - cam.x;
            }

            rect.y = i * FONT_HEIGHT - cam.y;
            SDL_RenderFillRect(renderer, &rect);
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

        cam->w = (float)screen_width;
        cam->h = (float)screen_height;

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

    // Colors
    long backgroundColor = 0x001b2eff;
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
    Cursor select = {.line = 0, .col = 0, .right = 0};
    Vec2f cam = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };

    _Bool selecting = 0;

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
              if (selecting) {
                  if (!removeSelection(&buffer, &select, &cursor)) return 1;
                  selecting = 0;
              }
              int len = addTextBeforeCursor(&buffer, event.text.text, cursor);

              cursor.col += len;
              break;
          }
          case SDL_KEYDOWN: {
            switch (event.key.keysym.sym) {
            case SDLK_RIGHT: {
                if (selecting) {
                  SDL_Keymod mod = SDL_GetModState();
                  if (mod & KMOD_SHIFT) {
                      cursorRight(&buffer, &cursor);
                  } else {
                      selecting = 0;
                  }
                } else {
                    cursorRight(&buffer, &cursor);
                }
                break;
            }
            case SDLK_LEFT: {
                if (selecting) {
                  SDL_Keymod mod = SDL_GetModState();
                  if (mod & KMOD_SHIFT) {
                      cursorLeft(&buffer, &cursor);
                  } else {
                      selecting = 0;
                  }
                } else {
                    cursorLeft(&buffer, &cursor);
                }
                break;
            }
            case SDLK_UP: {
                if (selecting) {
                  SDL_Keymod mod = SDL_GetModState();
                  if (mod & KMOD_SHIFT) {
                      cursorUp(&buffer, &cursor);
                  } else {
                      selecting = 0;
                  }
                } else {
                    cursorUp(&buffer, &cursor);
                }
                break;
            }
            case SDLK_DOWN: {
                if (selecting) {
                  SDL_Keymod mod = SDL_GetModState();
                  if (mod & KMOD_SHIFT) {
                      cursorDown(&buffer, &cursor);
                  } else {
                      selecting = 0;
                  }
                } else {
                    cursorDown(&buffer, &cursor);
                }
                break;
            }
            case SDLK_BACKSPACE: {
                if (selecting) {
                    if (!removeSelection(&buffer, &select, &cursor)) return 1;
                    selecting = 0;
                } else {
                    cursorBack(&buffer, &cursor);
                }
                break;
            }
            case SDLK_RETURN: {
                if (selecting) {
                    if (!removeSelection(&buffer, &select, &cursor)) return 1;
                    selecting = 0;
                }
                if (!cursorReturn(&buffer, &cursor))
                    return 1;
                break;
            }
            case SDLK_TAB: {
                if (selecting) {
                    if (!removeSelection(&buffer, &select, &cursor)) return 1;
                    selecting = 0;
                }
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
              break;
            }
            case SDLK_c: {
                if (selecting) {
                    char *text = copyTextSelection(&buffer, &select, &cursor);
                    SDL_SetClipboardText(text);
                    free(text);

                    selecting = 0;
                }
                break;
            }
            case SDLK_x: {
                if (selecting) {
                    char *text = copyTextSelection(&buffer, &select, &cursor);
                    SDL_SetClipboardText(text);
                    free(text);
                    if (!removeSelection(&buffer, &select, &cursor)) return 1;
                    selecting = 0;
                }
                break;
            }
            case SDLK_v: {
              SDL_Keymod mod = SDL_GetModState();
              if (mod & KMOD_CTRL) {
                if (selecting) {
                  if (!removeSelection(&buffer, &select, &cursor))
                    return 1;
                  selecting = 0;
                }

                if (SDL_HasClipboardText()) {
                  char *text = SDL_GetClipboardText();
                  addMultilineBeforeCursor(&buffer, text, &cursor);
                  SDL_free(text);
                }
              }
              break;
            }
            case SDLK_LSHIFT: {
                if (selecting == 0) {
                    selecting = 1;
                }
                break;
            }
            case SDLK_RSHIFT: {
                if (selecting == 0) {
                    selecting = 1;
                }
                break;
            }
            case SDLK_ESCAPE: {
                resetSelect(&selecting, &select, &cursor);
                break;
            }
            }
            break;
          }
          case SDL_KEYUP: {
              switch (event.key.keysym.sym) {
                  case SDLK_LSHIFT: {
                      if (cursor.line == select.line && cursor.col == select.col) {
                          resetSelect(&selecting, &select, &cursor);
                      }
                      break;
                  }
                  case SDLK_RSHIFT: {
                      if (cursor.line == select.line && cursor.col == select.col) {
                          resetSelect(&selecting, &select, &cursor);
                      }
                      break;
                  }
              }
              break;
          }
          }
        }

        clearScreen(renderer, HEX_TO_RGBA(backgroundColor));

        // Render editor buffer
        renderBuffer(renderer, &buffer, font, textColor, cam);

        // Render cursor
        renderCursor(renderer, &buffer, font, cursor, cam);

        // Render selection area.
        if (selecting) {
            renderSelect(renderer, select, cursor, cam);
        } else {
            resetSelect(&selecting, &select, &cursor);
        }

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
