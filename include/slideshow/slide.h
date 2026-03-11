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
  SlideSplitDirection direction;
} SlideSplit;

typedef struct {
  SlideSplit activeSplit;
  float padding;
  int numActiveSplits;
  SlideSplit activeSplits[SLIDE_MAX_SPLITS];
} Slide;

int SlideBegin(Slide* s, float padding);
int SlideBeginBiasedSplit(Slide* s, int splitDirection, float bias);
int SlideBeginSplit(Slide* s, int splitDirection);
int SlideNextSplit(Slide* s);
int SlideEndSplit(Slide* s);
Rectangle SlideCurrentRect(Slide* s);

#endif
