#include <slideshow/slide.h>
#include <slideshow/slideshow.h>

#include <base/sdf-font.h>
#include <stdlib.h>
#include <string.h>

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

void SlideText(Slide* s, const char* txt, Color tint) {
  Font font = SlideShowGetTextFont();
  int fontSize = SlideShowGetTextFontSize();
  Rectangle rect = SlideSplitRect(s);
  const float lineHeight = fontSize + 4.0f;
  const float spaceWidth = MeasureTextEx(font, " ", fontSize, 1.0f).x;

  BeginScissorMode(rect.x, rect.y, rect.width, rect.height);

  char *text = strdup(txt), *textEnd = text + strlen(text);

  /* Split text into lines first */
  char* p = text - 1;
  while ((p = strpbrk(p + 1, "\n")) != NULL)
    *p = '\0';

  float lineY = rect.y;
  /* For every line in the text */
  char* lineEnd;
  for (char* line = text; line < textEnd; line = lineEnd + 1) {
    float lineWidth = 0.0f;
    lineEnd = line + strlen(line);
    /* If the line is empty, increment the next line's Y coordinate and go to the next one */
    if (lineEnd == line) {
      lineY += lineHeight;
      continue;
    }

    /* Draw text line with wrapping */
    char* chunk = line;
    char blankChar = *line;
    while (blankChar != '\0') {
      char* chunkEnd = strpbrk(chunk, " ");
      if (chunkEnd == NULL)
        chunkEnd = lineEnd;

      blankChar = *chunkEnd;
      *chunkEnd = '\0';
      float chunkWidth = *chunk == '\0' ? spaceWidth : MeasureTextEx(font, chunk, fontSize, 1.0f).x;
      float newLineWidth = chunkWidth + lineWidth;
      *chunkEnd = blankChar;

      if (newLineWidth >= rect.width) {
        /* The current line overflows the width of the container, draw the current buffer and continue */
      draw:
        chunk[-1] = '\0'; /* `chunk` always points to the first character of this chunk,
                           * that means that chunk[-1] is the last whitespace
                           */
        DrawTextSDF(font, line, (Vector2) { rect.x, lineY }, fontSize, 1.0f, tint);
        line = chunk;
        lineY += lineHeight;
        lineWidth = chunkWidth + spaceWidth;
      } else if (blankChar == '\0') {
        /* End of the line, draw the remaining characters */
        chunk = chunkEnd + 1;
        goto draw;
      } else {
        /* The line still fits into the width of the container, keep counting the line width */
        lineWidth = newLineWidth + spaceWidth;
      }

      chunk = chunkEnd + 1;
    }
  }

  EndScissorMode();

  free(text);
}

