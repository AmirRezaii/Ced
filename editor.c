#include <stdlib.h>
#include <string.h>

#include <stdio.h>
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

int addTextBeforeCursor(Buffer *buff, const char *text,Cursor cursor) {
    Line *cur_line = &buff->lines[cursor.line];

    int input_len = strlen(text);

    memmove(cur_line->text + cursor.col + input_len,
            cur_line->text + cursor.col, strlen(cur_line->text + cursor.col));
    strncpy(cur_line->text + cursor.col, text, input_len);

    return input_len;
}

int addMultilineBeforeCursor(Buffer *buff, const char *text, Cursor *cursor) {
    size_t i = 0;
    size_t start = 0;
    while (text[i] != '\0') {
        if (text[i] == '\n') {
            char *line_text = (char *)malloc((i-start)*sizeof(char));
            memset(line_text, 0, (i - start + 1)*sizeof(char));
            strncpy(line_text, text+start, i - start);
            line_text[i-start] = '\0';

            int len = addTextBeforeCursor(buff, line_text, *cursor);
            cursor->col += len;

            free(line_text);

            start = i + 1;

            cursorReturn(buff, cursor);
        }
        i++;
    }

    char *line_text = (char *)malloc((i - start + 1)*sizeof(char));
    memset(line_text, 0, (i-start)*sizeof(char));
    strncpy(line_text, text + start, i - start);
    line_text[i-start] = '\0';

    int len = addTextBeforeCursor(buff, line_text, *cursor);
    cursor->col += len;
    free(line_text);

    return len;
}

char *copyTextSelection(Buffer *buffer, Cursor *select, Cursor *cursor) {
    // Get final text size
    size_t text_size = 0;

    if (select->line == cursor->line) {
        if (cursor->col > select->col) {
            text_size = cursor->col - select->col;
        } else {
            text_size = select->col - cursor->col;
        }
    } else if (select->line < cursor->line) {
        for (size_t i = select->line + 1; i < cursor->line; ++i) {
            text_size += strlen(buffer->lines[i].text);
            // Increase text size for newline
            text_size++;
        }
        text_size += strlen(buffer->lines[select->line].text + select->col);
        // Increase text size for newline
        text_size++;
        text_size += cursor->col;
    } else {
        for (size_t i = cursor->line + 1; i < select->line; ++i) {
            text_size += strlen(buffer->lines[i].text);
            // Increase text size for newline
            text_size++;
        }
        text_size += strlen(buffer->lines[cursor->line].text + cursor->col);
        // Increase text size for newline
        text_size++;
        text_size += select->col;
    }

    char *res = (char *)malloc((text_size + 1) * sizeof(char));
    memset(res, 0, text_size);

    if (select->line == cursor->line) {
        if (select->col < cursor->col) {
            strncat(res, buffer->lines[select->line].text + select->col, cursor->col - select->col);
        } else {
            strncat(res, buffer->lines[select->line].text + cursor->col, select->col - cursor->col);
        }
    } else if (select->line < cursor->line) {
        strcat(res, buffer->lines[select->line].text + select->col);
        strcat(res, "\n");

        for (size_t i = select->line + 1; i < cursor->line; ++i) {
            strcat(res, buffer->lines[i].text);
            strcat(res, "\n");
        }

        strncat(res, buffer->lines[cursor->line].text, cursor->col);
    } else {
        strcat(res, buffer->lines[cursor->line].text + cursor->col);
        strcat(res, "\n");

        for (size_t i = cursor->line + 1; i < select->line; ++i) {
            strcat(res, buffer->lines[i].text);
            strcat(res, "\n");
        }

        strncat(res, buffer->lines[select->line].text, select->col);
    }

    res[text_size] = '\0';
    return res;
}

int removeSelection(Buffer *buffer, Cursor *select, Cursor *cursor) {
    if (select->line == cursor->line) {
        if (select->col <= cursor->col) {
            char *line = buffer->lines[select->line].text;
            char *dest = line + select->col;
            char *src = line + cursor->col;
            int n = cursor->col - select->col;

            if (n == 0) return 1;

            memmove(dest, src, strlen(src));
            char *rest = line + strlen(line) - n;
            memset(rest, 0, n * sizeof(char));
        } else {
            char *line = buffer->lines[cursor->line].text;
            char *dest = line + cursor->col;
            char *src = line + select->col;
            int n = select->col - cursor->col;

            memmove(dest, src, strlen(src));
            char *rest = line + strlen(line) - n;
            memset(rest, 0, n);
        }
    } else if (select->line < cursor->line) {
        char *line = buffer->lines[select->line].text;
        memset(line + select->col, 0, strlen(line) - select->col);

        for (size_t i = select->line + 1; i < cursor->line; ++i) {
            if (!buffRemove(buffer, i)) return 0;
        }

        // append rest of the last selection line to first selection line
        char *rest = buffer->lines[cursor->line].text + cursor->col;
        strcat(line, rest);
        if (!buffRemove(buffer, cursor->line)) return 0;
    } else {
        char *line = buffer->lines[cursor->line].text;
        memset(line + cursor->col, 0, strlen(line) - cursor->col);

        for (size_t i = cursor->line + 1; i < select->line; ++i) {
            if (!buffRemove(buffer, i)) return 0;
        }

        // append rest of the last selection line to first selection line
        char *rest = buffer->lines[select->line].text + select->col;
        strcat(line, rest);
        if (!buffRemove(buffer, select->line)) return 0;
    }

    // Place cursor in right place after removal of text
    if (cursor->line == select->line) {
        if (cursor->col > select->col) {
            cursor->col = select->col;
            cursor->right = select->col;
        }
    } else if (cursor->line > select->line) {
        cursor->line = select->line;
        cursor->col = select->col;
        cursor->right = select->col;
    }

    return 1;
}
