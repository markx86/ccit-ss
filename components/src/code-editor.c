#include <components/code-editor.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 4096
#define MIN_LINE_LENGTH 32

#define LINE_NUMBER_TEMPLATE " 000  "

#define TAB_STR "  "

typedef struct Line {
  struct Line* prev;
  struct Line* next;
  int length;
  int capacity;
  char chars[];
} Line;

typedef struct {
  Line* head;
  Line* tail;
} LineList;

struct CodeEditor {
  int editable;
  int focused;
  LineList lines;
  int cursorCol;
  Line* cursorLine;
  /* Font stuff */
  Font font;
  double spacing;
  double fontSize;
  double linePadding;
  double lineNumberWidth;
  double charWidth;
  double charHeight;
  /* View stuff */
  Rectangle viewRect;
  int numLines;
  int charsPerLine;
  char* printBuffer;
  int printBufferLength;
};

static void RelinkLine(CodeEditor* codeEditor, Line* line) {
  if (line->prev == NULL)
    codeEditor->lines.head = line;
  else
    line->prev->next = line;

  if (line->next == NULL)
    codeEditor->lines.tail = line;
  else
    line->next->prev = line;
}

static void UnlinkLine(CodeEditor* codeEditor, Line* line) {
  /* Cannot unlink a non linked line! */
  if (line->prev == NULL && line->next == NULL)
    return;

  if (line->prev == NULL)
    codeEditor->lines.head = line->next;
  else
    line->prev->next = line->next;

  if (line->next == NULL)
    codeEditor->lines.tail = line->prev;
  else
    line->next->prev = line->prev;

  line->next = line->prev = NULL;
}

static void LinkLineAfter(CodeEditor* codeEditor, Line* pos, Line* line) {
  line->prev = pos;
  line->next = pos->next;
  pos->next = line;
  if (line->next != NULL)
    line->next->prev = line;

  if (pos == codeEditor->lines.tail)
    codeEditor->lines.tail = line;
}

static Line* AllocLine(void) {
  Line* line = MemAlloc(sizeof(*line) + MIN_LINE_LENGTH);
  if (line == NULL)
    return NULL;

  line->capacity = MIN_LINE_LENGTH;
  return line;
}

static Line* AllocNextLine(CodeEditor* codeEditor) {
  Line* line = AllocLine();
  if (line == NULL)
    return NULL;

  LinkLineAfter(codeEditor, codeEditor->cursorLine, line);
  return line;
}

static int InitLinesList(CodeEditor* codeEditor) {
  codeEditor->cursorLine = AllocLine();
  if (codeEditor->cursorLine == NULL)
    return 0;

  codeEditor->lines.tail = codeEditor->lines.head = codeEditor->cursorLine;
  return 1;
}

static int GetLineNumberWidth(Font font, int fontSize, int spacing) {
  Vector2 lineNumberSize = MeasureTextEx(font, LINE_NUMBER_TEMPLATE, fontSize, spacing);
  return lineNumberSize.x;
}

static int RecomputeCharSize(CodeEditor* codeEditor) {
  Rectangle r = codeEditor->viewRect;

  r.width -= codeEditor->lineNumberWidth;
  r.height -= codeEditor->linePadding;

  int charsPerLine = ceil(r.width / (codeEditor->charWidth + codeEditor->spacing));
  int numLines = ceil(r.height / (codeEditor->charHeight + codeEditor->linePadding));

  codeEditor->numLines = numLines;
  codeEditor->charsPerLine = charsPerLine;

  codeEditor->printBufferLength = charsPerLine + sizeof(LINE_NUMBER_TEMPLATE);
  codeEditor->printBuffer = MemRealloc(codeEditor->printBuffer, codeEditor->printBufferLength);

  return codeEditor->printBuffer != NULL;
}

static int SetFontSize(CodeEditor* codeEditor, int fontSize) {
  /* NOTE: The font should be monospaced */
  Vector2 charSize = MeasureTextEx(codeEditor->font, "A", fontSize, codeEditor->spacing);
  codeEditor->fontSize = fontSize;
  codeEditor->lineNumberWidth = GetLineNumberWidth(codeEditor->font, fontSize, codeEditor->spacing);
  codeEditor->charWidth = charSize.x;
  codeEditor->charHeight = charSize.y;
  return RecomputeCharSize(codeEditor);
}

CodeEditor* CodeEditorCreate(Font font, int width, int height, int fontSize) {
  return CodeEditorCreateEx(font, width, height, fontSize, 1, 1);
}

CodeEditor* CodeEditorCreateEx(Font font, int width, int height, int fontSize, int linePadding, int spacing) {
  CodeEditor* codeEditor = MemAlloc(sizeof(*codeEditor));

  if (width <= 0 || height <= 0 || linePadding < 0 || spacing < 0 || fontSize <= 0)
    return NULL;

  codeEditor->editable = 1;
  codeEditor->focused = 0;

  codeEditor->font = font;
  codeEditor->spacing = spacing;
  codeEditor->linePadding = linePadding;

  codeEditor->viewRect.width = width;
  codeEditor->viewRect.height = height;
  if (SetFontSize(codeEditor, fontSize) && InitLinesList(codeEditor))
    return codeEditor;

  CodeEditorDestroy(codeEditor);
  return NULL;
}

void CodeEditorDestroy(CodeEditor* codeEditor) {
  Line *nextLine, *line = codeEditor->lines.head;
  while (line != NULL) {
    nextLine = line->next;
    MemFree(line);
    line = nextLine;
  }
  if (codeEditor->printBuffer != NULL)
    MemFree(codeEditor->printBuffer);
  MemFree(codeEditor);
}

int CodeEditorResizeView(CodeEditor* codeEditor, int width, int height) {
  if (width <= 0 || height <= 0)
    return 0;

  codeEditor->viewRect.width = width;
  codeEditor->viewRect.height = height;
  return RecomputeCharSize(codeEditor);
}

static int IsPointInsideArea(CodeEditor* codeEditor, int x, int y) {
  return    x >= codeEditor->viewRect.x && x < codeEditor->viewRect.x + codeEditor->viewRect.width
         && y >= codeEditor->viewRect.y && y < codeEditor->viewRect.y + codeEditor->viewRect.height;
}

static int IsFocused(CodeEditor* codeEditor) {
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Vector2 mousePos = GetMousePosition();
    codeEditor->focused = IsPointInsideArea(codeEditor, mousePos.x, mousePos.y);
  }
  return codeEditor->focused;
}

static int ResizeLine(CodeEditor* codeEditor, Line** linep, int requiredLength) {
  Line* line = *linep;

  if (line->capacity > requiredLength)
    return 1;

  while (line->capacity < requiredLength && line->capacity < MAX_LINE_LENGTH)
    line->capacity <<= 1;

  if (line->capacity < requiredLength)
    return 0;

  line = MemRealloc(line, sizeof(*line) + line->capacity);
  if (line == NULL)
    return 0;

  RelinkLine(codeEditor, line);

  *linep = line;
  return 1;
}

static void DeleteLine(CodeEditor* codeEditor, Line* line) {
  UnlinkLine(codeEditor, line);
  MemFree(line);
}

static int InsertStringInLine(CodeEditor* codeEditor, Line** linep, int pos, const char* src, int len) {
  Line* line = *linep;

  if (pos > line->length)
    return 0;

  int newLength = line->length + len;
  if (line->capacity < newLength && !ResizeLine(codeEditor, linep, newLength))
    return 0;

  /* Grab the pointer again as it could've been updated by ResizeLine(..) */
  line = *linep;

  /* Move existing line data forward */
  if (pos < line->length)
    memmove(line->chars + pos + len, line->chars + pos, line->length - pos);
  /* Copy in the new data */
  memcpy(line->chars + pos, src, len);

  /* NOTE: We update the line length *after* the memmove, otherwise we would
   *       risk a OOB write access.
   */
  line->length = newLength;

  return 1;
}

static int DeleteRangeInLine(CodeEditor* codeEditor, Line** linep, int pos, int len) {
  Line* line = *linep;
  Line* nextLine = line->next;

  int newLength = line->length - len;
  if (newLength < 0) {
    /* Handle deleting over the start of the line */
    if (nextLine == NULL || !DeleteRangeInLine(codeEditor, &nextLine, 0, -newLength - 1))
      return 0;
    DeleteLine(codeEditor, line);
    *linep = nextLine;
    return 1;
  } else if (line->length < pos + len) {
    /* Handle deleting at the end of the line */
    if (nextLine != NULL) {
      if (!InsertStringInLine(codeEditor, linep, line->length, nextLine->chars, nextLine->length))
        return 0;
      DeleteLine(codeEditor, nextLine);
      /* The -1 is for the new-line char */
      return DeleteRangeInLine(codeEditor, linep, pos, len - 1);
    }
  }

  line->length = newLength;
  /* Move line data back */
  memmove(line->chars + pos, line->chars + pos + len, line->length - pos);

  return 1;
}

static int IsCursorPositionValid(CodeEditor* codeEditor) {
  return    codeEditor->cursorLine != NULL
         && codeEditor->cursorCol <= codeEditor->cursorLine->length;
}

static void DeletePrevChar(CodeEditor* codeEditor) {
  /* Sanity check */
  if (!IsCursorPositionValid(codeEditor))
    return;

  if (codeEditor->cursorCol - 1 < 0) {
    /* We cannot delete the newline character of the first line! */
    Line* prevLine = codeEditor->cursorLine->prev;
    if (prevLine == NULL)
      return;

    /* Handle deleting the newline character */
    int oldLength = prevLine->length;
    DeleteRangeInLine(codeEditor, &prevLine, prevLine->length, 1);
    codeEditor->cursorCol = oldLength;
    codeEditor->cursorLine = prevLine;
    return;
  }

  DeleteRangeInLine(codeEditor, &codeEditor->cursorLine, --codeEditor->cursorCol, 1);
}

static void DeleteChar(CodeEditor* codeEditor) {
  /* Sanity check */
  if (!IsCursorPositionValid(codeEditor))
    return;

  /* Avoid deleting the newline character for the last line */
  if (codeEditor->cursorLine->next == NULL && codeEditor->cursorCol == codeEditor->cursorLine->length)
    return;

  DeleteRangeInLine(codeEditor, &codeEditor->cursorLine, codeEditor->cursorCol, 1);
}

static void InsertNewLine(CodeEditor* codeEditor) {
  /* Sanity check */
  if (!IsCursorPositionValid(codeEditor))
    return;

  Line* nextLine = AllocNextLine(codeEditor);
  if (nextLine == NULL)
    return;

  int copyLength = codeEditor->cursorLine->length - codeEditor->cursorCol;
  if (copyLength > 0) {
    char* copyPos = codeEditor->cursorLine->chars + codeEditor->cursorCol;
    if (!InsertStringInLine(codeEditor, &nextLine, 0, copyPos, copyLength)) {
      DeleteLine(codeEditor, nextLine);
      return;
    }
    codeEditor->cursorLine->length = codeEditor->cursorCol;
  }

  codeEditor->cursorCol = 0;
  codeEditor->cursorLine = nextLine;
}

static void InsertAtCursorPosition(CodeEditor* codeEditor, const char* src, int len) {
  if (InsertStringInLine(codeEditor, &codeEditor->cursorLine, codeEditor->cursorCol, src, len))
    codeEditor->cursorCol += len;
}

static void AppendTab(CodeEditor* codeEditor) {
  /* Sanity check */
  if (!IsCursorPositionValid(codeEditor))
    return;

  InsertAtCursorPosition(codeEditor, TAB_STR, sizeof(TAB_STR)-1);
}

static void AppendChar(CodeEditor* codeEditor, char chr) {
  /* Sanity check */
  if (!IsCursorPositionValid(codeEditor))
    return;

  InsertAtCursorPosition(codeEditor, &chr, 1);
}

static void MoveCursor(CodeEditor* codeEditor, int dx, int dy) {
  Line* line = codeEditor->cursorLine;
  if (dx != 0) {
    /* Handle horizontal movement */
    int newCol = codeEditor->cursorCol + dx;
    if (newCol < 0) {
      line = line->prev;
      if (line == NULL)
        return;
      newCol = line->length;
    } else if (newCol > line->length) {
      line = line->next;
      if (line == NULL)
        return;
      newCol = 0;
    }
    codeEditor->cursorCol = newCol;
  } else if (dy != 0) {
    /* Handle vertical movement */
    while (dy != 0 && line != NULL) {
      line = dy < 0 ? line->prev : line->next;
      dy += (dy < 0) - (dy > 0);
    }
    if (line == NULL)
      return;
    if (codeEditor->cursorCol > line->length)
      codeEditor->cursorCol = line->length;
  }

  codeEditor->cursorLine = line;
}

static int KeyPressed(int key) {
  return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

static int LineNumber(Line* line) {
  int i = 0;
  while (line->prev != NULL) {
    ++i;
    line = line->prev;
  }
  return i;
}

static void ProcessInput(CodeEditor* codeEditor) {
  int chr;

  /* Movement commands */
  if /**/ (KeyPressed(KEY_LEFT))
    MoveCursor(codeEditor, -1, 0);
  else if (KeyPressed(KEY_RIGHT))
    MoveCursor(codeEditor, +1, 0);
  else if (KeyPressed(KEY_UP))
    MoveCursor(codeEditor, 0, -1);
  else if (KeyPressed(KEY_DOWN))
    MoveCursor(codeEditor, 0, +1);
  else if (codeEditor->editable) {
    /* Editing commands */
    if /**/ (KeyPressed(KEY_BACKSPACE))
      DeletePrevChar(codeEditor);
    else if (KeyPressed(KEY_DELETE))
      DeleteChar(codeEditor);
    else if (KeyPressed(KEY_ENTER))
      InsertNewLine(codeEditor);
    else if (KeyPressed(KEY_TAB))
      AppendTab(codeEditor);
    else if (IsKeyDown(KEY_RIGHT_CONTROL)) {
      if /**/ (KeyPressed(KEY_RIGHT_BRACKET))
        SetFontSize(codeEditor, codeEditor->fontSize + 1);
      else if (KeyPressed(KEY_LEFT_BRACKET))
        SetFontSize(codeEditor, codeEditor->fontSize - 1);
    }
    else if ((chr = GetCharPressed()) != 0)
      AppendChar(codeEditor, chr);
  }
}

static void RenderBackground(CodeEditor* codeEditor) {
  const int lineThickness = 2;
  Rectangle r = codeEditor->viewRect;
  Rectangle outline = {
    r.x - lineThickness,
    r.y - lineThickness,
    r.width + lineThickness * 2,
    r.height + lineThickness * 2
  };
  DrawRectangleLinesEx(outline, lineThickness, codeEditor->focused ? BLACK : LIGHTGRAY);
  DrawRectangle(r.x, r.y, r.width, r.height, RAYWHITE);
}

static void RenderCursor(CodeEditor* codeEditor) {
  double lineHeight = codeEditor->charHeight + codeEditor->linePadding;
  double charWidth = codeEditor->charWidth + codeEditor->spacing;
  Rectangle r = codeEditor->viewRect;

  int cursorX = r.x + codeEditor->lineNumberWidth + codeEditor->cursorCol * charWidth;
  int cursorY = r.y + codeEditor->linePadding + LineNumber(codeEditor->cursorLine) * lineHeight;

  if (IsPointInsideArea(codeEditor, cursorX, cursorY))
    DrawRectangle(cursorX, cursorY, 2, codeEditor->charHeight, BLACK);
}

static void RenderLines(CodeEditor* codeEditor) {
  int lineHeight = codeEditor->linePadding + codeEditor->charHeight;
  Rectangle r = codeEditor->viewRect;
  Line* line = codeEditor->lines.head;

  Vector2 linePos = { r.x, r.y + codeEditor->linePadding };
  for (int i = 0; i < codeEditor->numLines && line != NULL; ++i) {
    snprintf(codeEditor->printBuffer, codeEditor->printBufferLength,
             " %3d  %.*s", i + 1, line->length, line->chars);

    DrawTextEx(codeEditor->font,
               codeEditor->printBuffer,
               linePos,
               codeEditor->fontSize,
               codeEditor->spacing,
               DARKGRAY);

    linePos.y += lineHeight;
    line = line->next;
  }
}

int CodeEditorUpdate(CodeEditor* codeEditor) {
  if (!IsFocused(codeEditor))
    return 0;
  ProcessInput(codeEditor);
  return 1;
}

void CodeEditorRender(CodeEditor* codeEditor) {
  RenderBackground(codeEditor);

  Rectangle r = codeEditor->viewRect;
  BeginScissorMode(r.x, r.y, r.width, r.height);
  {
    RenderCursor(codeEditor);
    RenderLines(codeEditor);
  }
  EndScissorMode();
}

void CodeEditorSetPosition(CodeEditor* codeEditor, int x, int y) {
  codeEditor->viewRect.x = x;
  codeEditor->viewRect.y = y;
}

void CodeEditorSetEditable(CodeEditor* codeEditor, int yes) {
  codeEditor->editable = yes;
}

void CodeEditorSetFocused(CodeEditor* codeEditor, int yes) {
  codeEditor->focused = yes;
}

Rectangle CodeEditorGetRect(CodeEditor* codeEditor) {
  return codeEditor->viewRect;
}

static size_t GetTextLength(CodeEditor* codeEditor) {
  size_t length;
  Line* line = codeEditor->lines.head;
  for (length = 0; line != NULL; line = line->next)
    length += line->length + 1 /* for the new-line character */;
  return length;
}

char* CodeEditorGetText(CodeEditor* codeEditor) {
  size_t length = GetTextLength(codeEditor);
  char *p, *buffer = MemAlloc(length + 1 /* for the null terminator */);
  Line* line = codeEditor->lines.head;

  for (p = buffer; line != NULL; line = line->next) {
    if (p > buffer)
      *(p++) = '\n';
    memcpy(p, line->chars, line->length);
    p += line->length;
  }
  *p = '\0';

  return buffer;
}

void CodeEditorFreeText(char* text) {
  MemFree(text);
}
