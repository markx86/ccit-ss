#include <base/sdf-font.h>

#include <stddef.h>

/* NOTE: This file has been stolen from raylib's SDF example */
static const char* sdfFragmentShader =
  "#version 330\n"
  "// Input vertex attributes (from vertex shader)\n"
  "in vec2 fragTexCoord;\n"
  "in vec4 fragColor;\n"
  "\n"
  "// Input uniform values\n"
  "uniform sampler2D texture0;\n"
  "uniform vec4 colDiffuse;\n"
  "\n"
  "// Output fragment color\n"
  "out vec4 finalColor;\n"
  "\n"
  "// NOTE: Add your custom variables here\n"
  "\n"
  "void main()\n"
  "{\n"
  "    // Texel color fetching from texture sampler\n"
  "    // NOTE: Calculate alpha using signed distance field (SDF)\n"
  "    float distanceFromOutline = texture(texture0, fragTexCoord).a - 0.5;\n"
  "    float distanceChangePerFragment = length(vec2(dFdx(distanceFromOutline), dFdy(distanceFromOutline)));\n"
  "    float alpha = smoothstep(-distanceChangePerFragment, distanceChangePerFragment, distanceFromOutline);\n"
  "\n"
  "    // Calculate final fragment color\n"
  "    finalColor = vec4(fragColor.rgb, fragColor.a*alpha);\n"
  "}";

#define SDF_BASE_SIZE 32

static Shader sdfShader = {0};

void LoadSDFShader(void) {
  if (!IsShaderValid(sdfShader))
    sdfShader = LoadShaderFromMemory(NULL, sdfFragmentShader);
}

void UnloadSDFShader(void) {
  if (IsShaderValid(sdfShader))
    UnloadShader(sdfShader);
}

Font LoadFontSDF(const char* fontPath) {
  int fileSize = 0;
  unsigned char* fileData = LoadFileData(fontPath, &fileSize);

  Font font = {0};
  font.baseSize = SDF_BASE_SIZE;
  font.glyphCount = 95;
  font.glyphs = LoadFontData(fileData, fileSize, font.baseSize, NULL, 0, FONT_SDF, &font.glyphCount);
  Image atlas = GenImageFontAtlas(font.glyphs, &font.recs, font.glyphCount, font.baseSize, 0, 1);
  font.texture = LoadTextureFromImage(atlas);
  UnloadImage(atlas);
  UnloadFileData(fileData);

  SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

  return font;
}

void DrawTextSDF(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
  BeginShaderMode(sdfShader);
    DrawTextEx(font, text, position, fontSize, spacing, tint);
  EndShaderMode();
}
