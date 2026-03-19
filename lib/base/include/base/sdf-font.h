#ifndef SDF_FONT_H_
#define SDF_FONT_H_

#include <raylib.h>

void LoadSDFShader(void);
void UnloadSDFShader(void);
int  EnableShaderSDF(void);
int  DisableShaderSDF(void);
Font LoadFontSDF(const char* path);
/* NOTE: Avoid using DrawTextSDF(..), use DrawTextEx(..) with Enable/DisableShaderSDF() instead */
void DrawTextSDF(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);

#define WithShaderSDF() for (int _ = EnableShaderSDF(); _; _ = DisableShaderSDF())

#endif
