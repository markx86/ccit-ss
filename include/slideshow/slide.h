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
  Rectangle currentRect;
  Rectangle lastSplit;
  SlideSplitDirection direction;
} SlideSplit;

typedef struct {
  SlideSplit activeSplit;
  float padding;
  int numActiveSplits;
  SlideSplit activeSplits[SLIDE_MAX_SPLITS];
} Slide;

int SlideBegin(Slide* s, float padding);

static inline Rectangle SlideSplitRect(Slide* s) {
  return s->activeSplit.lastSplit;
}

int SlideSplitByPercent(Slide* s, float perc);
int SlideSplitBySize(Slide* s, int pixels);
int SlideSplitHalf(Slide* s);
int SlideSplitRemaining(Slide* s);

int _SlideBeginSplit(Slide* s, int splitDirection);
int _SlideEndSplit(Slide* s);

#define SlideSplit(s, direction) \
  for (int i = _SlideBeginSplit(s, direction); i; i = _SlideEndSplit(s))

#endif
