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

int SlideBegin(float padding);
int SlideBeginWithTitle(float padding, const char* title);

Rectangle SlideSplitRect(void);

int SlideSplitByPercent(float perc);
int SlideSplitBySize(int pixels);
int SlideSplitHalf(void);
int SlideSplitRemaining(void);

int  SlideBeginSplit(int splitDirection);
int  SlideEndSplit(void);
void SlideRebaseOnSplit(void);

void SlideText(const char* text);
void SlideCode(const char* text);
void SlideImage(Texture2D texture);

#define SlideSplit(direction) \
  for (int _ = SlideBeginSplit(direction); _; _ = SlideEndSplit())

#define CRGB(r, g, b) "\33" #r "," #g "," #b ";"
#define CIDX(i)       "\33" #i ";"
#define CRST()        CIDX(0)

#endif
