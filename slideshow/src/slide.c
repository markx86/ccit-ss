#include <slideshow/slide.h>

static void SetActiveSplitView(Slide* s, Rectangle r) {
  s->activeSplit.view.x = r.x;
  s->activeSplit.view.y = r.y;
  s->activeSplit.view.width = r.width;
  s->activeSplit.view.height = r.height;
}

static void InitCurrentRect(Slide* s, float bias) {
  s->activeSplit.currentRect = s->activeSplit.view;
  switch (s->activeSplit.direction) {
    case SLIDE_SPLIT_VERTICAL:
      s->activeSplit.currentRect.height *= bias;
      s->activeSplit.currentRect.height -= s->padding * 0.5f;
      break;
    case SLIDE_SPLIT_HORIZONTAL:
      s->activeSplit.currentRect.width  *= bias;
      s->activeSplit.currentRect.width -= s->padding * 0.5f;
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
  InitCurrentRect(s, 1.0f);
  s->activeSplit.currentRect = s->activeSplit.view;

  return 1;
}

static int PushSplit(Slide* s) {
  if (s->numActiveSplits >= SLIDE_MAX_SPLITS)
    return 0;
  s->activeSplits[s->numActiveSplits++] = s->activeSplit;
  return 1;
}

static int PopSplit(Slide* s) {
  if (s->numActiveSplits <= 0)
    return 0;
  s->activeSplit = s->activeSplits[--s->numActiveSplits];
  return 1;
}

static float GetWidthLeft(SlideSplit* s) {
  return s->view.width - s->currentRect.width - (s->currentRect.x - s->view.x);
}

static float GetHeightLeft(SlideSplit* s) {
  return s->view.height - s->currentRect.height - (s->currentRect.y - s->view.y);
}

int SlideNextSplit(Slide* s) {
  Rectangle r;
  switch (s->activeSplit.direction) {
    case SLIDE_SPLIT_NONE:
      /* Cannot split this slide */
      return 0;
    case SLIDE_SPLIT_VERTICAL:
      r = (Rectangle) {
        s->activeSplit.currentRect.x,
        s->activeSplit.currentRect.y + s->activeSplit.currentRect.height + s->padding,
        s->activeSplit.currentRect.width,
        GetHeightLeft(&s->activeSplit) - s->padding,
      };
      break;
    case SLIDE_SPLIT_HORIZONTAL:
      r = (Rectangle) {
        s->activeSplit.currentRect.x + s->activeSplit.currentRect.width + s->padding,
        s->activeSplit.currentRect.y,
        GetWidthLeft(&s->activeSplit) - s->padding,
        s->activeSplit.currentRect.height,
      };
      break;
  }
  s->activeSplit.currentRect = r;
  return    r.width  > 0.0f && r.x < s->activeSplit.view.x + s->activeSplit.view.width
         && r.height > 0.0f && r.y < s->activeSplit.view.y + s->activeSplit.view.height;
}

Rectangle SlideCurrentRect(Slide* s) {
  return s->activeSplit.currentRect;
}

int SlideBeginBiasedSplit(Slide* s, int splitDirection, float bias) {
  if (bias > 1.0f)
    return 0;

  Rectangle view = SlideCurrentRect(s);

  if (!PushSplit(s))
    return 0;

  s->activeSplit.view = view;
  s->activeSplit.direction = splitDirection;

  InitCurrentRect(s, bias);

  return 1;
}

int SlideBeginSplit(Slide* s, int splitDirection) {
  return SlideBeginBiasedSplit(s, splitDirection, 0.5f);
}

int SlideEndSplit(Slide* s) {
  return PopSplit(s);
}
