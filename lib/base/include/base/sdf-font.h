#ifndef SDF_FONT_H_
#define SDF_FONT_H_

#include <raylib.h>

void LoadSDFShader(void);
void UnloadSDFShader(void);
Font LoadFontSDF(const char* path);
void DrawTextSDF(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);

#endif
