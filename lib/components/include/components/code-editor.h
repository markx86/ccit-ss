#ifndef CODE_EDITOR_H_
#define CODE_EDITOR_H_

#include <raylib.h>

/* TODO: Handle scrolling */

typedef struct CodeEditor CodeEditor;

CodeEditor* CodeEditorCreate(Font font, int width, int height, int fontSize);
CodeEditor* CodeEditorCreateEx(Font font, int width, int height, int fontSize, int linePadding, int spacing);
void  CodeEditorDestroy(CodeEditor* codeEditor);
int   CodeEditorSetBounds(CodeEditor* codeEditor, Rectangle bounds);
int   CodeEditorUpdate(CodeEditor* codeEditor);
void  CodeEditorRender(CodeEditor* codeEditor);
Rectangle CodeEditorGetRect(CodeEditor* codeEditor);
void  CodeEditorSetEditable(CodeEditor* codeEditor, int yes);
void  CodeEditorSetFocused(CodeEditor* codeEditor, int yes);
char* CodeEditorGetText(CodeEditor* codeEditor);
void  CodeEditorFreeText(char* text);
void  CodeEditorSetFontSize(int size);

#endif
