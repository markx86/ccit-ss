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
  SlideSplit activeSplit;
  float padding;
  int numActiveSplits;
  SlideSplit activeSplits[SLIDE_MAX_SPLITS];
} Slide;

int SlideBegin(Slide* s, float padding);
int SlideBeginWithTitle(Slide* s, float padding, const char* title);

static inline Rectangle SlideSplitRect(Slide* s) {
  return s->activeSplit.lastRect;
}

int SlideSplitByPercent(Slide* s, float perc);
int SlideSplitBySize(Slide* s, int pixels);
int SlideSplitHalf(Slide* s);
int SlideSplitRemaining(Slide* s);

int  SlideBeginSplit(Slide* s, int splitDirection);
int  SlideEndSplit(Slide* s);
void SlideRebaseOnSplit(Slide* s);

#define SlideSplit(s, direction) \
  for (int i = SlideBeginSplit(s, direction); i; i = SlideEndSplit(s))

#endif
