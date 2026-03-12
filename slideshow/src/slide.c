#include <components/sdf-font.h>
#include <slideshow/slide.h>
#include <slideshow/slideshow.h>

static void InitActiveSplit(Slide* s, Rectangle view, SlideSplitDirection direction) {
  s->activeSplit.view = view;
  s->activeSplit.freeRect = view;
  s->activeSplit.direction = direction;
  switch (s->activeSplit.direction) {
    case SLIDE_SPLIT_VERTICAL:
      s->activeSplit.lastRect.width = view.width;
      s->activeSplit.lastRect.height = 0.0f;
      break;
    case SLIDE_SPLIT_HORIZONTAL:
      s->activeSplit.lastRect.width = 0.0f;
      s->activeSplit.lastRect.height = view.height;
      break;
    default:
      s->activeSplit.lastRect = view;
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
  InitActiveSplit(s, view, SLIDE_SPLIT_NONE);

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
        s->activeSplit.freeRect.x,
        s->activeSplit.freeRect.y,
        s->activeSplit.freeRect.width,
        h
      };
      newRect = (Rectangle) {
        split.x,
        split.y + h + s->padding,
        s->activeSplit.freeRect.width,
        s->activeSplit.freeRect.height - h - s->padding,
      };
      break;
    case SLIDE_SPLIT_HORIZONTAL:
      split = (Rectangle) {
        s->activeSplit.freeRect.x,
        s->activeSplit.freeRect.y,
        w,
        s->activeSplit.freeRect.height
      };
      newRect = (Rectangle) {
        split.x + w + s->padding,
        split.y,
        s->activeSplit.freeRect.width - w - s->padding,
        s->activeSplit.freeRect.height,
      };
      break;
  }

  s->activeSplit.lastRect = split;
  s->activeSplit.freeRect = newRect;

  return    split.x >= s->activeSplit.view.x && split.x + split.width  <= s->activeSplit.view.x + s->activeSplit.view.width
         && split.y >= s->activeSplit.view.y && split.y + split.height <= s->activeSplit.view.y + s->activeSplit.view.height;
}

int SlideSplitByPercent(Slide* s, float perc) {
  return SplitCurrentRect(s,
                          s->activeSplit.freeRect.width * perc - s->padding * 0.5f,
                          s->activeSplit.freeRect.height * perc - s->padding * 0.5f);
}

int SlideSplitBySize(Slide* s, int pixels) {
  return SplitCurrentRect(s, pixels, pixels);
}

int SlideSplitHalf(Slide* s) {
  return SlideSplitByPercent(s, 0.5f);
}

int SlideSplitRemaining(Slide* s) {
  return SplitCurrentRect(s, s->activeSplit.freeRect.width, s->activeSplit.freeRect.height);
}

int SlideBeginSplit(Slide* s, int splitDirection) {
  Rectangle view = s->activeSplit.lastRect;
  if (!PushSplit(s))
    return 0;

  InitActiveSplit(s, view, splitDirection);
  return 1;
}

int SlideEndSplit(Slide* s) {
  PopSplit(s);
  return 0;
}

void SlideRebaseOnSplit(Slide* s) {
  s->numActiveSplits = 0;
  InitActiveSplit(s, s->activeSplit.freeRect, SLIDE_SPLIT_NONE);
}

int SlideBeginWithTitle(Slide* s, float padding, const char* title) {
  if (!SlideBegin(s, padding))
    return 0;

  Font font = SlideShowGetTitleFont();
  int fontSize = SlideShowGetTitleFontSize();
  Vector2 titleSize = MeasureTextEx(font, title, fontSize, 1);

  if (!SlideBeginSplit(s, SLIDE_SPLIT_VERTICAL))
    return 0;

  if (SlideSplitBySize(s, titleSize.y * 2)) {
    Rectangle r = SlideSplitRect(s);
    Vector2 pos = {
      r.x + 64.0f,
      r.y + (r.height - titleSize.y) * 0.5f
    };
    DrawTextSDF(font, title, pos, fontSize, 1, SlideShowGetPrimaryColor());
  }

  SlideRebaseOnSplit(s);
  return 1;
}
