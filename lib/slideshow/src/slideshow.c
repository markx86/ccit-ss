#include "slideshow/slide.h"
#include <slideshow/slideshow.h>

#include <base/sdf-font.h>
#include <base/pak.h>
#include <base/arena.h>
#include <base/image.h>

#include <raylib.h>

extern const struct _SlideShow SlideShow;
extern const struct _SlideShowFontSizes SlideShowFontSizes;
extern const struct _SlideShowColors SlideShowColors;

static struct {
  Font normal[FONT_STYLE_MAX];
  Font monospaced[FONT_STYLE_MAX];
} CurrentFonts = {0};
static struct _SlideShowFontSizes CurrentFontSizes = {0};
static struct _SlideShowColors CurrentColors = {0};

int SlideShowSetFont(int fontStyle, const char* file) {
  Font* array = fontStyle & FONT_STYLE_MONOSPACED ? CurrentFonts.monospaced : CurrentFonts.normal;
  int fontIndex = fontStyle & 3;
  if (fontIndex >= FONT_STYLE_MAX)
    return 0;

  Font font = file == DEFAULT_FONT ? GetFontDefault() : LoadFontSDF(file);
  if (!IsFontValid(font))
    return 0;

  Font* fontp = &array[fontIndex];
  if (IsFontValid(*fontp))
    UnloadFont(*fontp);

  *fontp = font;
  return 1;
}

Font SlideShowGetFont(int fontStyle) {
  Font* array = fontStyle & FONT_STYLE_MONOSPACED ? CurrentFonts.monospaced : CurrentFonts.normal;
  int fontIndex = fontStyle & 3;
  return array[fontIndex];
}

Color SlideShowGetAccentColor(void)     { return CurrentColors.accent;     }
Color SlideShowGetPrimaryColor(void)    { return CurrentColors.primary;    }
Color SlideShowGetSecondaryColor(void)  { return CurrentColors.secondary;  }
Color SlideShowGetBackgroundColor(void) { return CurrentColors.background; }

size_t SlideShowGetTextFontSize(void)  { return CurrentFontSizes.text;  }
size_t SlideShowGetTitleFontSize(void) { return CurrentFontSizes.title; }

void SlideShowSetFontSizes(size_t text, size_t title) {
  CurrentFontSizes.text = text;
  CurrentFontSizes.title = title;
}

void SlideShowResetFontSizes(void) {
  CurrentFontSizes = SlideShowFontSizes;
}

void SlideShowSetColors(Color background, Color secondary, Color primary, Color accent) {
  CurrentColors.background = background;
  CurrentColors.secondary = secondary;
  CurrentColors.primary = primary;
  CurrentColors.accent = accent;
}

void SlideShowResetColors(void) {
  CurrentColors = SlideShowColors;
}

static void LoadFonts(void) {
  Font defaultFont = GetFontDefault();
  for (int i = 0; i < FONT_STYLE_MAX; ++i) {
    CurrentFonts.normal[i] = defaultFont;
    CurrentFonts.monospaced[i] = defaultFont;
  }
}

static int IsKeyPressedOrRepeated(int key) {
  return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

static void EndSlide(void) {
  ClearBackground(BLACK);
  SlideBeginWithTitleEx(64.0f, "End of the presentation.", RAYWHITE);
}

static int DrawSlide(size_t slideIndex) {
  if (slideIndex >= SlideShow.numSlides) {
    EndSlide();
    return 0;
  }

  ClearBackground(CurrentColors.background);
  return SlideShow.slides[slideIndex](slideIndex + 1);
}

static int SlideShowInput(size_t* currentSlide) {
  size_t initialValue = *currentSlide;

  if /**/ (IsKeyPressedOrRepeated(KEY_LEFT)
           && *currentSlide > 0)
    --(*currentSlide);
  else if (IsKeyPressedOrRepeated(KEY_RIGHT)
           && *currentSlide < SlideShow.numSlides)
    ++(*currentSlide);

  if /**/ (IsKeyPressed(KEY_HOME))
    *currentSlide = 0;
  else if (IsKeyPressed(KEY_END))
    *currentSlide = SlideShow.numSlides - 1;

  return initialValue != *currentSlide;
}

int main(void) {
  size_t currentSlide;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(800, 600, "Software Security 0");

  LoadPakFile();
  LoadSDFShader();
  LoadFonts();

  SlideShowResetFontSizes();
  SlideShowResetColors();

  ArenaInit(0x100000);

  if (SlideShow.init != NULL && !SlideShow.init()) {
    TraceLog(LOG_ERROR, "SLIDESHOW: Could not initialize slideshow!");
    goto fail;
  }

  currentSlide = 0;
  while (!WindowShouldClose()) {
    BeginDrawing();

    if (!DrawSlide(currentSlide) && SlideShowInput(&currentSlide)) {
      /* If we changed slides, deallocate all the memory int the arena */
      ArenaReset();
    }

    if (IsKeyPressed(KEY_F11))
      ToggleFullscreen();

    EndDrawing();
  }

  if (SlideShow.cleanup != NULL) SlideShow.cleanup();
fail:
  ImageClearCache();
  UnloadSDFShader();
  UnloadPakFile();

  CloseWindow();
  return 0;
}
