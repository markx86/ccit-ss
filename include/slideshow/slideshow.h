#ifndef SLIDESHOW_H_
#define SLIDESHOW_H_

#include <raylib.h>
#include <stddef.h>

struct _SlideDef {
  void* data;
  void* (*init)(void);
  int   (*draw)(void*, size_t);
  void  (*free)(void*);
} ;

struct _SlideShow {
  size_t numSlides;
  struct _SlideDef slides[];
};

struct _SlideShowFonts {
  const char* normal;
  const char* monospaced;
  const char* title;
};

Font SlideShowGetMonospacedFont(void);
Font SlideShowGetNormalFont(void);
Font SlideShowGetTitleFont(void);

#define SLIDE_EX(_name)    \
  { .init  = _name##Init,  \
    .draw  = _name##Draw,  \
    .free  = _name##Free }
#define SLIDE(_name) \
  { .init = NULL, .draw = _name##Draw, .free = NULL }

#define SLIDE_INIT(_name) \
  void* _name##Init(void)

#define SLIDE_DRAW(_name) \
  int _name##Draw(void* slideData, size_t slideNumber)

#define SLIDE_FREE(_name) \
  void _name##Free(void* slideData)

#define SLIDESHOW(...)                      \
  struct _SlideShow SlideShow = {           \
    .numSlides = sizeof((struct _SlideDef[]) { __VA_ARGS__ }) \
                   / sizeof(struct _SlideDef), \
    .slides = { __VA_ARGS__ }               \
  }

#define SLIDESHOW_FONTS(_normal, _monospaced, _title) \
  const struct _SlideShowFonts SlideShowFonts = {     \
    .normal = _normal,                                \
    .monospaced = _monospaced,                        \
    .title = _title                                   \
  }

#define SLIDESHOW_FONTS_DEFAULT() \
  const struct _SlideShowFonts SlideShowFonts = {0}

#endif
