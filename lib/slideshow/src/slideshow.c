#include <slideshow/slideshow.h>

#include <base/sdf-font.h>
#include <base/pak.h>
#include <base/arena.h>

#include <raylib.h>

extern struct _SlideShow SlideShow;
extern const struct _SlideShowFonts SlideShowFonts;
extern const struct _SlideShowFontSizes SlideShowFontSizes;
extern const struct _SlideShowColors SlideShowColors;

typedef struct {
  Font text, monospaced, title;
} TextFonts;

static TextFonts Fonts;
static struct _SlideShowFontSizes FontSizes;
static struct _SlideShowColors CurrentColors;

Font SlideShowGetMonospacedFont(void) { return Fonts.monospaced; }
Font SlideShowGetTextFont(void)       { return Fonts.text;       }
Font SlideShowGetTitleFont(void)      { return Fonts.title;      }

Color SlideShowGetAccentColor(void)     { return CurrentColors.accent;     }
Color SlideShowGetPrimaryColor(void)    { return CurrentColors.primary;    }
Color SlideShowGetSecondaryColor(void)  { return CurrentColors.secondary;  }
Color SlideShowGetBackgroundColor(void) { return CurrentColors.background; }

size_t SlideShowGetTextFontSize(void)  { return FontSizes.text;  }
size_t SlideShowGetTitleFontSize(void) { return FontSizes.title; }

void SlideShowSetFontSizes(size_t text, size_t title) {
  FontSizes.text = text;
  FontSizes.title = title;
}

void SlideShowResetFontSizes(void) {
  FontSizes = SlideShowFontSizes;
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

static Font LoadFontOrGetDefault(const char* fontPath) {
  if (fontPath == NULL)
    return GetFontDefault();
  else
    return LoadFontSDF(fontPath);
}

static void LoadFonts(void) {
  Fonts.text     = LoadFontOrGetDefault(SlideShowFonts.text);
  Fonts.monospaced = LoadFontOrGetDefault(SlideShowFonts.monospaced);
  Fonts.title      = LoadFontOrGetDefault(SlideShowFonts.title);
}

static int IsKeyPressedOrRepeated(int key) {
  return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

static void InitSlides(void) {
  struct _SlideDef* slide;
  for (size_t i = 0; i < SlideShow.numSlides; ++i) {
    slide = &SlideShow.slides[i];
    if (slide->init != NULL)
      slide->data = slide->init();
    else
      slide->data = NULL;
  }
}

static void FreeSlides(void) {
  struct _SlideDef* slide;
  for (size_t i = 0; i < SlideShow.numSlides; ++i) {
    slide = &SlideShow.slides[i];
    if (slide->free != NULL)
      slide->free(slide->data);
  }
}

static void EndSlide(void) {
  ClearBackground(BLACK);
  DrawTextSDF(Fonts.text, "End of the presentation.", (Vector2) { 32, 32 }, 32, 1, RAYWHITE);
}

static int DrawSlide(size_t slideIndex) {
  if (slideIndex >= SlideShow.numSlides) {
    EndSlide();
    return 0;
  }

  ClearBackground(CurrentColors.background);
  struct _SlideDef* slide = &SlideShow.slides[slideIndex];
  return slide->draw(slide->data, slideIndex + 1);
}

static int SlideShowInput(size_t* currentSlide) {
  int initialValue = *currentSlide;

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
  InitSlides();

  SlideShowResetFontSizes();
  SlideShowResetColors();

  ArenaInit(0x100000);

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

  FreeSlides();
  UnloadSDFShader();
  UnloadPakFile();

  CloseWindow();
  return 0;
}
