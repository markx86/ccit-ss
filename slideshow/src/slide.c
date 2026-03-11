#include <slideshow/slide.h>
#include <slideshow/slideshow.h>

static void SetActiveSplitView(Slide* s, Rectangle r) {
  s->activeSplit.view.x = r.x;
  s->activeSplit.view.y = r.y;
  s->activeSplit.view.width = r.width;
  s->activeSplit.view.height = r.height;
}

static void InitCurrentRect(Slide* s) {
  s->activeSplit.lastSplit = s->activeSplit.view;
  s->activeSplit.currentRect = s->activeSplit.view;
  switch (s->activeSplit.direction) {
    case SLIDE_SPLIT_VERTICAL:
      s->activeSplit.lastSplit.height = 0.0f;
      break;
    case SLIDE_SPLIT_HORIZONTAL:
      s->activeSplit.lastSplit.width = 0.0f;
      break;
    default:
      break;
  }
}

int SlideBegin(Slide* s, float padding) {
  Rectangle view = {
    padding,
    padding,
    GetScreenWidth() - padding*2.0f,
    GetScreenHeight() - padding*2.0f
  };

  if (padding < 0.0f)
    return 0;

  s->padding = padding;
  s->numActiveSplits = 0;
  s->activeSplit.direction = SLIDE_SPLIT_NONE;
  SetActiveSplitView(s, view);
  InitCurrentRect(s);
  s->activeSplit.currentRect = s->activeSplit.view;

  return 1;
}

static int PushSplit(Slide* s) {
  if (s->numActiveSplits >= SLIDE_MAX_SPLITS)
    return 0;
  s->activeSplits[s->numActiveSplits++] = s->activeSplit;
  return 1;
}

static void PopSplit(Slide* s) {
  if (s->numActiveSplits > 0)
    s->activeSplit = s->activeSplits[--s->numActiveSplits];
}

static int SplitCurrentRect(Slide* s, float w, float h) {
  Rectangle newRect;
  Rectangle split;
  switch (s->activeSplit.direction) {
    case SLIDE_SPLIT_NONE:
      /* Cannot split this slide */
      return 0;
    case SLIDE_SPLIT_VERTICAL:
      split = (Rectangle) {
        s->activeSplit.currentRect.x,
        s->activeSplit.currentRect.y,
        s->activeSplit.currentRect.width,
        h
      };
      newRect = (Rectangle) {
        split.x,
        split.y + h + s->padding,
        s->activeSplit.currentRect.width,
        s->activeSplit.currentRect.height - h - s->padding,
      };
      break;
    case SLIDE_SPLIT_HORIZONTAL:
      split = (Rectangle) {
        s->activeSplit.currentRect.x,
        s->activeSplit.currentRect.y,
        w,
        s->activeSplit.currentRect.height
      };
      newRect = (Rectangle) {
        split.x + w + s->padding,
        split.y,
        s->activeSplit.currentRect.width - w - s->padding,
        s->activeSplit.currentRect.height,
      };
      break;
  }

  s->activeSplit.lastSplit = split;
  s->activeSplit.currentRect = newRect;

  return    split.x >= s->activeSplit.view.x && split.x + split.width  <= s->activeSplit.view.x + s->activeSplit.view.width
         && split.y >= s->activeSplit.view.y && split.y + split.height <= s->activeSplit.view.y + s->activeSplit.view.height;
}

int SlideSplitByPercent(Slide* s, float perc) {
  return SplitCurrentRect(s,
                          s->activeSplit.currentRect.width * perc - s->padding * 0.5f,
                          s->activeSplit.currentRect.height * perc - s->padding * 0.5f);
}

int SlideSplitBySize(Slide* s, int pixels) {
  return SplitCurrentRect(s, pixels, pixels);
}

int SlideSplitHalf(Slide* s) {
  return SlideSplitByPercent(s, 0.5f);
}

int SlideSplitRemaining(Slide* s) {
  return SplitCurrentRect(s, s->activeSplit.currentRect.width, s->activeSplit.currentRect.height);
}

int _SlideBeginSplit(Slide* s, int splitDirection) {
  Rectangle view = s->activeSplit.lastSplit;
  if (!PushSplit(s))
    return 0;

  s->activeSplit.view = view;
  s->activeSplit.direction = splitDirection;
  InitCurrentRect(s);

  return 1;
}

int _SlideEndSplit(Slide* s) {
  PopSplit(s);
  return 0;
}

}
