#include <slideshow/slideshow.h>
#include <slideshow/slide.h>
#include <base/sdf-font.h>
#include <base/pak.h>
#include <base/arena.h>
#include <base/image.h>

#include <raylib.h>

extern const struct _SlideShow SlideShow;
extern const struct _SlideShowFontSizes SlideShowFontSizes;
extern const struct _SlideShowColors SlideShowColors;

static struct {
  size_t slideIndex;
  struct {
    Font normal[FONT_STYLE_MAX];
    Font monospaced[FONT_STYLE_MAX];
  } fonts;
  struct _SlideShowFontSizes fontSizes;
  struct _SlideShowColors colors;
} State = {0};

size_t SlideShowGetCurrentSlideNumber(void) { return State.slideIndex + 1; }

int SlideShowSetFont(int fontStyle, const char* file) {
  Font* array = fontStyle & FONT_STYLE_MONOSPACED ? State.fonts.monospaced : State.fonts.normal;
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
  Font* array = fontStyle & FONT_STYLE_MONOSPACED ? State.fonts.monospaced : State.fonts.normal;
  int fontIndex = fontStyle & 3;
  return array[fontIndex];
}

Color SlideShowGetAccentColor(void)     { return State.colors.accent;     }
Color SlideShowGetPrimaryColor(void)    { return State.colors.primary;    }
Color SlideShowGetSecondaryColor(void)  { return State.colors.secondary;  }
Color SlideShowGetBackgroundColor(void) { return State.colors.background; }

size_t SlideShowGetTextFontSize(void)  { return State.fontSizes.text;  }
size_t SlideShowGetTitleFontSize(void) { return State.fontSizes.title; }

void SlideShowSetFontSizes(size_t text, size_t title) {
  if (text > 16 && title > 16) {
    State.fontSizes.text = text;
    State.fontSizes.title = title;
  }
}

void SlideShowResetFontSizes(void) {
  State.fontSizes = SlideShowFontSizes;
}

void SlideShowSetColors(Color background, Color secondary, Color primary, Color accent) {
  State.colors.background = background;
  State.colors.secondary = secondary;
  State.colors.primary = primary;
  State.colors.accent = accent;
}

void SlideShowResetColors(void) {
  State.colors = SlideShowColors;
}

static void LoadFonts(void) {
  Font defaultFont = GetFontDefault();
  for (int i = 0; i < FONT_STYLE_MAX; ++i) {
    State.fonts.normal[i] = defaultFont;
    State.fonts.monospaced[i] = defaultFont;
  }
}

static int KeyPressed(int key) {
  return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

static void EndSlide(void) {
  ClearBackground(BLACK);
  SlideBeginWithTitleEx(64.0f, "End of the presentation.", RAYWHITE);
}

static int DrawCurrentSlide(void) {
  if (State.slideIndex >= SlideShow.numSlides) {
    EndSlide();
    return 0;
  }

  ClearBackground(State.colors.background);
  return SlideShow.slides[State.slideIndex]();
}

static int SlideShowInput(void) {
  size_t initialIndex = State.slideIndex;

  if /**/ (KeyPressed(KEY_LEFT)  && State.slideIndex > 0)
    --State.slideIndex;
  else if (KeyPressed(KEY_RIGHT) && State.slideIndex < SlideShow.numSlides)
    ++State.slideIndex;

  if /**/ (IsKeyPressed(KEY_HOME))
    State.slideIndex = 0;
  else if (IsKeyPressed(KEY_END))
    State.slideIndex = SlideShow.numSlides - 1;

  if (IsKeyDown(KEY_RIGHT_CONTROL)) {
    if /**/ (KeyPressed(KEY_RIGHT_BRACKET))
      SlideShowSetFontSizes(State.fontSizes.text + 1, State.fontSizes.title + 1);
    else if (KeyPressed(KEY_LEFT_BRACKET))
      SlideShowSetFontSizes(State.fontSizes.text - 1, State.fontSizes.title - 1);
  }

  return initialIndex != State.slideIndex;
}

int main(void) {
  SetConfigFlags(/*FLAG_WINDOW_RESIZABLE |*/ FLAG_VSYNC_HINT);
  InitWindow(1280, 720, "Raylib SlideShow");

  /* Disable exit key */
  SetExitKey(KEY_NULL);

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

  while (!WindowShouldClose()) {
    BeginDrawing();

    if (!DrawCurrentSlide() && SlideShowInput()) {
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
