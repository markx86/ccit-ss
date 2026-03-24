#include <components/reg-view.h>

#include <stdio.h>

Vector2 RegView(Font font, Vector2 pos, unsigned long value, float fontSize, const char** regNames, int regNamesCount, int showSeparator) {
  const Color colors[] = {
    ORANGE,
    RED,
    MAGENTA,
    BLUE
  };

  if (regNamesCount <= 0)
    return (Vector2) { 0.0f, 0.0f };

  char regValue[32];
  snprintf(regValue, sizeof(regValue), "%016lX", value);

  float valueFontSize = fontSize * 2.0f;
  Vector2 valueSize = MeasureTextEx(font, regValue, valueFontSize, 2.0f);
  Vector2 regNameSize = MeasureTextEx(font, regNames[0], fontSize, 1.0f);

  DrawTextEx(font, regValue, (Vector2) { pos.x, pos.y }, valueFontSize, 2.0f, BLACK);

  float lineWidth = valueSize.x;
  for (int i = 0; i < regNamesCount && i < 4; ++i) {
    Vector2 endPos = {
      pos.x + valueSize.x,
      pos.y + valueSize.y + regNameSize.y * i + 8.0f
    };
    Vector2 startPos = { endPos.x - lineWidth, endPos.y };
    Vector2 textPos = {
      endPos.x + 8.0f,
      endPos.y - regNameSize.y * 0.5f
    };
    Vector2 vertEndPos   = { startPos.x + 2.0f, pos.y + valueSize.y * 0.8f };
    Vector2 vertStartPos = { startPos.x + 2.0f, startPos.y };
    Color color = colors[i];

    DrawLineEx(startPos, endPos, 4.0f, color);
    if (showSeparator)
      DrawLineEx(vertStartPos, vertEndPos, 4.0f, color);
    DrawTextEx(font, regNames[i], textPos, fontSize, 1.0f, BLACK);

    lineWidth *= 0.5f;
  }

  return (Vector2) {
    valueSize.x + 8.0f + regNameSize.x,
    valueSize.y + regNameSize.y * regNamesCount + 4.0f
  };
}
