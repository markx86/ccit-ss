#include <slideshow/slide.h>
#include <slideshow/slideshow.h>

#include <base/sdf-font.h>
#include <base/arena.h>

#include <string.h>

struct {
  SlideSplit activeSplit;
  float padding;
  int numActiveSplits;
  SlideSplit activeSplits[SLIDE_MAX_SPLITS];
} Slide;

static void InitActiveSplit(Rectangle view, SlideSplitDirection direction) {
  Slide.activeSplit.view = view;
  Slide.activeSplit.freeRect = view;
  Slide.activeSplit.direction = direction;
  switch (Slide.activeSplit.direction) {
    case SLIDE_SPLIT_VERTICAL:
      Slide.activeSplit.lastRect.width = view.width;
      Slide.activeSplit.lastRect.height = 0.0f;
      break;
    case SLIDE_SPLIT_HORIZONTAL:
      Slide.activeSplit.lastRect.width = 0.0f;
      Slide.activeSplit.lastRect.height = view.height;
      break;
    default:
      Slide.activeSplit.lastRect = view;
      break;
  }
}

static int PushSplit(void) {
  if (Slide.numActiveSplits >= SLIDE_MAX_SPLITS)
    return 0;
  Slide.activeSplits[Slide.numActiveSplits++] = Slide.activeSplit;
  return 1;
}

static void PopSplit(void) {
  if (Slide.numActiveSplits > 0)
    Slide.activeSplit = Slide.activeSplits[--Slide.numActiveSplits];
}

static int SplitCurrentRect(float w, float h) {
  Rectangle newRect;
  Rectangle split;
  switch (Slide.activeSplit.direction) {
    case SLIDE_SPLIT_NONE:
      /* Cannot split this slide */
      return 0;
    case SLIDE_SPLIT_VERTICAL:
      split = (Rectangle) {
        Slide.activeSplit.freeRect.x,
        Slide.activeSplit.freeRect.y,
        Slide.activeSplit.freeRect.width,
        h
      };
      newRect = (Rectangle) {
        split.x,
        split.y + h + Slide.padding,
        Slide.activeSplit.freeRect.width,
        Slide.activeSplit.freeRect.height - h - Slide.padding,
      };
      break;
    case SLIDE_SPLIT_HORIZONTAL:
      split = (Rectangle) {
        Slide.activeSplit.freeRect.x,
        Slide.activeSplit.freeRect.y,
        w,
        Slide.activeSplit.freeRect.height
      };
      newRect = (Rectangle) {
        split.x + w + Slide.padding,
        split.y,
        Slide.activeSplit.freeRect.width - w - Slide.padding,
        Slide.activeSplit.freeRect.height,
      };
      break;
  }

  Slide.activeSplit.lastRect = split;
  Slide.activeSplit.freeRect = newRect;

  return    split.x >= Slide.activeSplit.view.x && split.x + split.width  <= Slide.activeSplit.view.x + Slide.activeSplit.view.width
         && split.y >= Slide.activeSplit.view.y && split.y + split.height <= Slide.activeSplit.view.y + Slide.activeSplit.view.height;
}

int SlideSplitByPercent(float perc) {
  return SplitCurrentRect(Slide.activeSplit.freeRect.width  * perc - Slide.padding * 0.5f,
                          Slide.activeSplit.freeRect.height * perc - Slide.padding * 0.5f);
}

int SlideSplitBySize(int pixels) {
  return SplitCurrentRect(pixels, pixels);
}

int SlideSplitHalf(void) {
  return SlideSplitByPercent(0.5f);
}

int SlideSplitRemaining(void) {
  return SplitCurrentRect(Slide.activeSplit.freeRect.width, Slide.activeSplit.freeRect.height);
}

Rectangle SlideSplitRect(void) {
  return Slide.activeSplit.lastRect;
}

int SlideBegin(float padding) {
  Rectangle view = {
    padding,
    padding,
    GetScreenWidth() - padding*2.0f,
    GetScreenHeight() - padding*2.0f
  };

  if (padding < 0.0f)
    return 0;

  Slide.padding = padding;
  Slide.numActiveSplits = 0;
  InitActiveSplit(view, SLIDE_SPLIT_NONE);

  return 1;
}

int SlideBeginSplit(int splitDirection) {
  Rectangle view = Slide.activeSplit.lastRect;
  if (!PushSplit())
    return 0;

  InitActiveSplit(view, splitDirection);
  return 1;
}

int SlideEndSplit(void) {
  PopSplit();
  return 0;
}

void SlideRebaseOnSplit(void) {
  Slide.numActiveSplits = 0;
  InitActiveSplit(Slide.activeSplit.freeRect, SLIDE_SPLIT_NONE);
}

int SlideBeginWithTitle(float padding, const char* title) {
  if (!SlideBegin(padding))
    return 0;

  Font font = SlideShowGetTitleFont();
  int fontSize = SlideShowGetTitleFontSize();
  Vector2 titleSize = MeasureTextEx(font, title, fontSize, 1);

  if (!SlideBeginSplit(SLIDE_SPLIT_VERTICAL))
    return 0;

  if (SlideSplitBySize(titleSize.y * 2)) {
    Rectangle r = SlideSplitRect();
    Vector2 pos = {
      r.x + 64.0f,
      r.y + (r.height - titleSize.y) * 0.5f
    };
    DrawTextSDF(font, title, pos, fontSize, 1, SlideShowGetPrimaryColor());
  }

  SlideRebaseOnSplit();
  return 1;
}

void SlideText(const char* txt, Color tint) {
  Font font = SlideShowGetTextFont();
  int fontSize = SlideShowGetTextFontSize();
  Rectangle rect = SlideSplitRect();
  const float lineHeight = fontSize + 4.0f;
  const float spaceWidth = MeasureTextEx(font, " ", fontSize, 1.0f).x;

  BeginScissorMode(rect.x, rect.y, rect.width, rect.height);

  ArenaPush();

  char *text = ArenaStrdup(txt), *textEnd = text + strlen(text);

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

  ArenaPop();
}

void SlideImage(Texture texture) {
  Rectangle rect = SlideSplitRect();
  /* TODO */
}
