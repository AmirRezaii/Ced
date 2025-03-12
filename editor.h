#ifndef EDITOR_H_
#define EDITOR_H_

#include <stddef.h>

typedef struct {
    char text[256];
} Line;

typedef struct {
    Line *lines;
    size_t capacity;
    size_t count;
} Buffer;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} Vec2f;

typedef struct {
    size_t line;
    size_t col;
    size_t right;
} Cursor;

void buffAppend(Buffer *buff, Line line);
int buffInsert(Buffer *buff, Line line, size_t index);
int buffRemove(Buffer *buff, size_t index);
Buffer loadToBuffer(const char *text);
char *stringFromBuffer(Buffer *buff);
void buffFree(Buffer *buff);

int addMultilineBeforeCursor(Buffer *buff, const char *text, Cursor *cursor);
int addTextBeforeCursor(Buffer *buff, const char *text, Cursor cursor);
void cursorRight(Buffer *buff, Cursor *cursor);
void cursorLeft(Buffer *buff, Cursor *cursor);
void cursorUp(Buffer *buff, Cursor *cursor);
void cursorDown(Buffer *buff, Cursor *cursor);
void cursorBack(Buffer *buffer, Cursor *cursor);
int cursorReturn(Buffer *buffer, Cursor *cursor);

char *copyTextSelection(Buffer *buffer, Cursor *select, Cursor *cursor);
int removeSelection(Buffer *buffer, Cursor *select, Cursor *cursor);

#endif // EDITOR_H_
