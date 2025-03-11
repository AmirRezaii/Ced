#include <stdlib.h>
#include <string.h>

#include "editor.h"

void buffAppend(Buffer *buff, Line line) {
    if (buff->count >= buff->capacity) {
        size_t new_capacity = buff->capacity == 0 ? 8 : buff->capacity * 2;
        buff->lines = (Line *)realloc(buff->lines, sizeof(Line) * new_capacity);
        buff->capacity = new_capacity;
    }

    buff->count++;
    buff->lines[buff->count-1] = line;
}

int buffInsert(Buffer *buff, Line line, size_t index) {
    if (index > buff->count) return 0;

    if (index == buff->count) {
        buffAppend(buff, line);
        return 1;
    }

    if (buff->count >= buff->capacity) {
        size_t new_capacity = buff->capacity == 0 ? 8 : buff->capacity * 2;
        buff->lines = (Line *)realloc(buff->lines, sizeof(Line) * new_capacity);
        buff->capacity = new_capacity;
    }

    memmove(buff->lines+index+1, buff->lines+index, sizeof(Line) * (buff->count - index));
    buff->count++;
    buff->lines[index] = line;
    return 1;
}

int buffRemove(Buffer *buff, size_t index) {
    if (index > buff->count) return 1;
    memmove(buff->lines+index, buff->lines+index+1, sizeof(Line) * (buff->count-index-1));
    buff->count--;
    return 1;
}

Buffer loadToBuffer(const char *text) {
    Buffer buff = {0};

    size_t i = 0;
    size_t start = 0;
    while (text[i] != '\0') {
        if (text[i] == '\n') {
            Line line = {0};
            strncpy(line.text, text+start, i - start);
            buffAppend(&buff, line);
            start = i + 1;
        }
        i++;
    }

    Line line = {0};
    strncpy(line.text, text + start, i - start);
    buffAppend(&buff, line);

    return buff;
}

// Get null-terminated string from buffer
char *stringFromBuffer(Buffer *buff) {
    int text_len = 0;
    for (size_t i = 0; i < buff->count; ++i) {
        text_len += strlen(buff->lines[i].text);
    }

    char *text = (char *)malloc(sizeof(char) * (text_len + 1));
    memset(text, 0, text_len + 1);

    for (size_t i = 0; i < buff->count; ++i) {
        strcat(text, buff->lines[i].text);
        strcat(text, "\n");
    }

    text[strlen(text)-1] = '\0';
    return text;
}

void buffFree(Buffer *buff) {
    free(buff->lines);
}


int addTextBeforeCursor(Buffer *buff, const char *text,Cursor cursor) {
    Line *cur_line = &buff->lines[cursor.line];

    int input_len = strlen(text);

    memmove(cur_line->text + cursor.col + input_len,
            cur_line->text + cursor.col, strlen(cur_line->text + cursor.col));
    strncpy(cur_line->text + cursor.col, text, input_len);

    return input_len;
}

void cursorRight(Buffer *buff, Cursor *cursor) {
    // TODO: add ability to use ctrl for better contorol
    // SDL_Keymod mod = SDL_GetModState();
    if (cursor->col >= strlen(buff->lines[cursor->line].text)) {
        if (cursor->line < buff->count - 1) {
          cursor->col = 0;
          cursor->line += 1;
        }
    } else {
        cursor->col += 1;
    }

    cursor->right = cursor->col;
}
void cursorLeft(Buffer *buff, Cursor *cursor) {
    if (cursor->col == 0) {
        if (cursor->line > 0) {
          cursor->col = strlen(buff->lines[cursor->line - 1].text);
          cursor->line -= 1;
        }
    } else {
        cursor->col -= 1;
    }
    cursor->right = cursor->col;
}

void cursorUp(Buffer *buff, Cursor *cursor) {
    if (cursor->line > 0) {
        size_t up_line_len = strlen(buff->lines[cursor->line-1].text);
        if (cursor->right <= up_line_len) {
            cursor->line -= 1;
            cursor->col = cursor->right;
        } else {
            cursor->col = up_line_len;
            cursor->line -= 1;
        }
    }
}

void cursorDown(Buffer *buff, Cursor *cursor) {
    if (cursor->line < buff->count - 1) {
        size_t down_line_len = strlen(buff->lines[cursor->line+1].text);
        if (cursor->right <= down_line_len) {
          cursor->line += 1;
          cursor->col = cursor->right;
        } else {
          cursor->col = down_line_len;
          cursor->line += 1;
        }
    }
}

void cursorBack(Buffer *buffer, Cursor *cursor) {
    Line *cur_line = &buffer->lines[cursor->line];
    if (strlen(cur_line->text) > 0) {
        if (!cur_line->text[cursor->col] && cursor->col > 0) {
          cur_line->text[cursor->col - 1] = '\0';
          cursor->col -= 1;
        } else if (cur_line->text[cursor->col] && cursor->col > 0) {
          memmove(cur_line->text + cursor->col - 1, cur_line->text + cursor->col,
                  strlen(cur_line->text + cursor->col));
          memset(cur_line->text + strlen(cur_line->text) - 1, 0, sizeof(char));
          cursor->col -= 1;
        } else if (cursor->col == 0) {
          if (cursor->line > 0) {
            Line *up_line = &buffer->lines[cursor->line - 1];
            cursor->col = strlen(up_line->text);
            strcat(up_line->text, cur_line->text);
            buffRemove(buffer, cursor->line);
            cursor->line -= 1;
          }
        }
    } else {
        if (cursor->line > 0) {
          Line *up_line = &buffer->lines[cursor->line - 1];
          buffRemove(buffer, cursor->line);
          cursor->line -= 1;
          cursor->col = strlen(up_line->text);
        }
    }
    cursor->right = cursor->col;
}

int cursorReturn(Buffer *buffer, Cursor *cursor) {
    Line *cur_line = &buffer->lines[cursor->line];
    Line new_line = {0};
    if (cur_line->text[cursor->col]) {
        memmove(new_line.text, cur_line->text + cursor->col,
                strlen(cur_line->text + cursor->col));
        memset(cur_line->text + cursor->col, 0,
               strlen(cur_line->text + cursor->col));
    }

    cursor->col = 0;

    cursor->line += 1;
    if (!buffInsert(buffer, new_line, cursor->line))
        return 0;

    cursor->right = cursor->col;
    return 1;
}
