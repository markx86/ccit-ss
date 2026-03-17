#include <assert.h>
#include <slideshow/slide.h>
#include <slideshow/slideshow.h>

#include <base/sdf-font.h>
#include <base/arena.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

static char* ParseColor(char* text, Color* color) {
  if (text[0] != COLOR_ESCAPE)
    goto out;

  char* end = strpbrk(text + 1, ";");
  if (end == NULL)
    goto out;

  int commas = 0;
  for (char* p = text + 1; p < end; ++p) {
    if (isdigit(*p))
      continue;
    else if (*p == ',')
      ++commas;
    else
      goto out;
  }

  char* endp;
  if (commas == 0) {
    /* Indexed color */
    int index = strtoul(text + 1, &endp, 10);
    if (index >= 3)
      goto out;

    const Color colors[] = {
      SlideShowGetPrimaryColor(),
      SlideShowGetSecondaryColor(),
      SlideShowGetAccentColor(),
    };
    *color = colors[index];
  } else if (commas == 2) {
    /* RGB color */
    char* comma1 = strpbrk(text   + 1, ",");
    char* comma2 = strpbrk(comma1 + 1, ",");

    unsigned char r = strtoul(text + 1, &endp, 10);
    if (endp != comma1)
      goto out;

    unsigned char g = strtoul(comma1 + 1, &endp, 10);
    if (endp != comma2)
      goto out;

    unsigned char b = strtoul(comma2 + 1, &endp, 10);
    if (endp != end)
      goto out;

    color->r = r;
    color->g = g;
    color->b = b;
  } else
    goto out;

  text = end + 1;
out:
  return text;
}

static char* NextStopChar(char* cursor, char* end) {
  char* stopChar = strpbrk(cursor, " \33");
  return stopChar == NULL ? end : stopChar;
}

static float TextWidth(Font font, const char* text, int fontSize) {
  return MeasureTextEx(font, text, fontSize, 1.0f).x;
}

static void SlideDrawText(Font font, const char* txt) {
  Color tint = SlideShowGetPrimaryColor();
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

  Vector2 pos = { rect.x, rect.y };
  /* For every line in the text */
  char* lineEnd;
  for (char* line = text; line < textEnd; line = lineEnd + 1) {
    /* If the line is outside the rectangle, break out */
    if (pos.y >= rect.y + rect.height)
      break;

    lineEnd = line + strlen(line);
    /* If the line is empty, increment the next line's Y coordinate and go to the next one */
    if (lineEnd == line) {
      pos.y += lineHeight;
      continue;
    }

    /* Draw text line with wrapping */
    for (;;) {
      char* lineBreak = line - 1;
      float lineWidth = 0.0f;
      bool overflow = false;
      while (lineBreak < lineEnd) {
        char* newLineBreak = NextStopChar(lineBreak + 1, lineEnd);
        char stopChar = newLineBreak[0];

        newLineBreak[0] = '\0';
        float newLineWidth = TextWidth(font, line, fontSize);
        TraceLog(LOG_INFO, "New line width %.2f (max width: %.2f, line = '%s', stopChar = %02x)", newLineWidth + (pos.x - rect.x), rect.width, line, stopChar);
        newLineBreak[0] = stopChar;

        if (pos.x + newLineWidth >= rect.x + rect.width) {
          overflow = true;
          break;
        }

        lineBreak = newLineBreak;
        lineWidth = newLineWidth;

        if (stopChar == '\0' || stopChar == COLOR_ESCAPE)
          break;
      }

      if (lineWidth <= 0.0f) {
        assert(overflow);
        goto overflow;
      }

      char stopChar = lineBreak[0];
      lineBreak[0] = '\0';
      TraceLog(LOG_INFO, "Drawing line '%s'", line);
      DrawTextSDF(font, line, pos, fontSize, 1.0f, tint);
      lineBreak[0] = stopChar;

      if (overflow) {
      overflow:
        pos.x = rect.x;
        pos.y += lineHeight;
        /* If the next line won't fit into the rectangle, we can exit out early */
        if /**/ (pos.y >= rect.y + rect.height)
          goto out;
        /* We overflowed without parsing text, retry */
        else if (lineWidth <= 0.0f)
          goto skip_spaces;
      } else {
        pos.x += lineWidth;
        /* If the stop char was a space, we have to account for it's width */
        if (stopChar == ' ') {
          pos.x += spaceWidth;
          /* Check if we've overflowed thanks to the space */
          if (pos.x >= rect.x + rect.width) {
            overflow = true;
            goto overflow;
          }
        }
      }

      /* End of line */
      if (stopChar == '\0')
        break;

      /* Color sequence is next */
      if /**/ (stopChar == COLOR_ESCAPE)
        line = ParseColor(lineBreak, &tint);
      else /* (stopChar == ' ') */ {
        assert(overflow);
        line = lineBreak + 1;
        /* If we broke due to a space, skip extra spaces that would otherwise end up at the start of the new line */
      skip_spaces:
        while (isblank(*line))
          ++line;
      }
    }
  }
out:

  EndScissorMode();

  ArenaPop();
}

void SlideText(const char* text) {
  SlideDrawText(SlideShowGetTextFont(), text);
}

void SlideCode(const char* text) {
  SlideDrawText(SlideShowGetMonospacedFont(), text);
}

void SlideImage(Texture2D texture) {
  Rectangle rect = SlideSplitRect();
  float scaleX = rect.width  / (float)texture.width;
  float scaleY = rect.height / (float)texture.height;
  float scale  = scaleX > scaleY ? scaleY : scaleX;
  if (scale > 1.0f)
    scale = 1.0f;
  Vector2 pos = {
    rect.x + (rect.width  - texture.width  * scale) * 0.5f,
    rect.y + (rect.height - texture.height * scale) * 0.5f,
  };
  DrawTextureEx(texture, pos, 0.0f, scale, RAYWHITE);
}
