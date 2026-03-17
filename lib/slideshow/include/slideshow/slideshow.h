#ifndef SLIDESHOW_H_
#define SLIDESHOW_H_

#include <raylib.h>
#include <stddef.h>

typedef int (*SlideFunc)(size_t);

struct _SlideShow {
  int    (*init)(void);
  void   (*cleanup)(void);
  size_t numSlides;
  SlideFunc slides[];
};

struct _SlideShowFonts {
  const char* text;
  const char* monospaced;
  const char* title;
};

struct _SlideShowColors {
  Color background;
  Color secondary;
  Color primary;
  Color accent;
};

struct _SlideShowFontSizes {
  size_t text;
  size_t title;
};

Font SlideShowGetMonospacedFont(void);
Font SlideShowGetTextFont(void);
Font SlideShowGetTitleFont(void);

void   SlideShowSetFontSizes(size_t text, size_t title);
void   SlideShowResetFontSizes(void);
size_t SlideShowGetTextFontSize(void);
size_t SlideShowGetTitleFontSize(void);

void  SlideShowSetColors(Color background, Color secondary, Color primary, Color accent);
void  SlideShowResetColors(void);
Color SlideShowGetAccentColor(void);
Color SlideShowGetPrimaryColor(void);
Color SlideShowGetSecondaryColor(void);
Color SlideShowGetBackgroundColor(void);

#define SLIDESHOW(_init, _cleanup, ...)     \
  struct _SlideShow SlideShow = {           \
    .init = _init, .cleanup = _cleanup,     \
    .numSlides = sizeof((SlideFunc[]) { __VA_ARGS__ }) / sizeof(SlideFunc), \
    .slides = { __VA_ARGS__ }               \
  }

#define DEFAULT_FONT NULL

#define SLIDESHOW_FONTS(_text, _monospaced, _title) \
  const struct _SlideShowFonts SlideShowFonts = {   \
    .text = _text,                                  \
    .monospaced = _monospaced,                      \
    .title = _title                                 \
  }

#define SLIDESHOW_FONTS_DEFAULT() \
  SLIDESHOW_FONTS(DEFAULT_FONT, DEFAULT_FONT, DEFAULT_FONT)

#define DEFAULT_FONT_SIZE_TEXT  24
#define DEFAULT_FONT_SIZE_TITLE 72

#define SLIDESHOW_FONT_SIZES(_text, _title)               \
  const struct _SlideShowFontSizes SlideShowFontSizes = { \
    .text = _text,                                        \
    .title = _title                                       \
  }

#define SLIDESHOW_FONT_SIZES_DEFAULT() \
  SLIDESHOW_FONT_SIZES(DEFAULT_FONT_SIZE_TEXT, DEFAULT_FONT_SIZE_TITLE)

#define DEFAULT_BACKGROUND_COLOR RAYWHITE
#define DEFAULT_PRIMARY_COLOR    BLACK
#define DEFAULT_SECONDARY_COLOR  DARKGRAY
#define DEFAULT_ACCENT_COLOR     LIGHTGRAY

#define SLIDESHOW_COLORS(_background, _secondary, _primary, _accent) \
  const struct _SlideShowColors SlideShowColors = {                  \
    .background = _background,                                       \
    .secondary  = _secondary,                                        \
    .primary    = _primary,                                          \
    .accent     = _accent                                            \
  }

#define SLIDESHOW_COLORS_DEFAULT()           \
  SLIDESHOW_COLORS(DEFAULT_BACKGROUND_COLOR, \
                   DEFAULT_SECONDARY_COLOR,  \
                   DEFAULT_PRIMARY_COLOR,    \
                   DEFAULT_ACCENT_COLOR)

#endif
