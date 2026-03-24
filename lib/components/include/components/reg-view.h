#ifndef REG_VIEW_H_
#define REG_VIEW_H_

#include <raylib.h>

Vector2 RegView(Font font, Vector2 pos, unsigned long value, float fontSize, const char** regNames, int regNamesCount, int showSeparator);

#endif
