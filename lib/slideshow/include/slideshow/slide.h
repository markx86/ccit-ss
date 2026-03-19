#ifndef SLIDE_H_
#define SLIDE_H_

#include <raylib.h>

#define SLIDE_MAX_SPLITS 10

typedef enum {
  SLIDE_SPLIT_NONE,
  SLIDE_SPLIT_VERTICAL,
  SLIDE_SPLIT_HORIZONTAL,
} SlideSplitDirection;

typedef struct {
  Rectangle view;
  Rectangle freeRect;
  Rectangle lastRect;
  SlideSplitDirection direction;
} SlideSplit;

typedef struct {
  int bold       : 1;
  int italic     : 1;
  int underline  : 1;
  int code       : 1;
  int _          : 4;
  int colorRed   : 8;
  int colorGreen : 8;
  int colorBlue  : 8;
} TextStyle;

int SlideBegin(float padding);
int SlideBeginWithTitle(float padding, const char* title);
int SlideBeginWithTitleEx(float padding, const char* title, Color titleColor);

Rectangle SlideSplitRect(void);

int SlideSplitByPercent(float perc);
int SlideSplitBySize(int pixels);
int SlideSplitHalf(void);
int SlideSplitRemaining(void);

int  SlideBeginSplit(int splitDirection);
int  SlideEndSplit(void);
void SlideRebaseOnSplit(void);

void SlideText(const char* text);
void SlideTextEx(const char* txt, int fontSize);
void SlideTextPro(const char* txt, int fontSize, TextStyle style);
void SlideImage(Texture2D texture);

TextStyle TextBuildStyle(int bold, int italic, int monospaced, int underline, Color tint);

#define SlideSplit(direction) \
  for (int _ = SlideBeginSplit(direction); _; _ = SlideEndSplit())

#define CRGB(r, g, b) "\33" #r "," #g "," #b ";"
#define CIDX(i)       "\33" #i ";"
#define CRST()        CIDX(0)

#endif
