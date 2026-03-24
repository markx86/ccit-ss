#include <slideshow/slide.h>
#include <slideshow/slideshow.h>

#include <base/sdf-font.h>
#include <base/arena.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

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
    case SLIDE_SPLIT_HORIZONTAL:
      Slide.activeSplit.lastRect.width = view.width;
      Slide.activeSplit.lastRect.height = 0.0f;
      break;
    case SLIDE_SPLIT_VERTICAL:
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
    case SLIDE_SPLIT_HORIZONTAL:
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
    case SLIDE_SPLIT_VERTICAL:
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
  return SlideBeginWithTitleEx(padding, title, SlideShowGetPrimaryColor());
}

int SlideBeginWithTitleEx(float padding, const char* title, Color titleColor) {
  if (!SlideBegin(padding))
    return 0;

  Font font = SlideShowGetFont(FONT_STYLE_BOLD);
  int fontSize = SlideShowGetTitleFontSize();
  Vector2 titleSize = MeasureTextEx(font, title, fontSize, 1);

  if (!SlideBeginSplit(SLIDE_SPLIT_HORIZONTAL))
    return 0;

  if (SlideSplitBySize(titleSize.y))
    SlideTextPro(title, fontSize, TextBuildStyle(1, 0, 0, 1, titleColor));

  SlideRebaseOnSplit();
  return 1;
}

static char* SplitLines(char* text) {
  char* textEnd = text + strlen(text);
  char* p = text - 1;
  while ((p = strpbrk(p + 1, "\n")) != NULL && p < textEnd)
    *p = '\0';
  return textEnd;
}

typedef struct TextToken {
  struct TextToken* next;
  TextStyle style;
  float width;
  float spaceWidth;
  bool joinWithNext;
  size_t length;
  char  string[];
} TextToken;

#define COLOR_ESCAPE '\33'

TextStyle TextBuildStyle(int bold, int italic, int monospaced, int underline, Color tint) {
  return (TextStyle) {
    .bold       = !!bold,
    .italic     = !!italic,
    .underline  = !!underline,
    .code       = !!monospaced,
    .colorRed   = tint.r,
    .colorGreen = tint.g,
    .colorBlue  = tint.b
  };
}

static Font GetFontByStyle(TextStyle style) {
  int fontStyle = FONT_STYLE_REGULAR;
  if (style.bold)
    fontStyle |= FONT_STYLE_BOLD;
  if (style.italic)
    fontStyle |= FONT_STYLE_ITALIC;
  if (style.code)
    fontStyle |= FONT_STYLE_MONOSPACED;
  return SlideShowGetFont(fontStyle);
}

static Color GetColorByStyle(TextStyle style) {
  return (Color) { style.colorRed, style.colorGreen, style.colorBlue, 255 };
}

static float TokenWidth(TextToken* tok, int fontSize) {
  Font font = GetFontByStyle(tok->style);
  return MeasureTextEx(font, tok->string, fontSize, 1.0f).x + 1.0f /* end spacing */;
}

static void TokenDraw(TextToken* tok, Vector2* pos, int fontSize) {
  Color color = GetColorByStyle(tok->style);
  DrawTextEx(GetFontByStyle(tok->style), tok->string, *pos, fontSize, 1.0f, color);
  if (tok->style.underline) {
    int y = pos->y + fontSize + 1;
    float width = tok->width;
    /* If the next token shouldn't be underlined, do not underline the space inbetween */
    if (tok->next != NULL && !tok->next->style.underline)
      width -= tok->spaceWidth;
    DrawLineEx((Vector2) { pos->x, y }, (Vector2) { pos->x + width, y }, 2.0f, color);
  }
  pos->x += tok->width;
}

static char* ParseColor(char* text, TextStyle* style) {
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
  Color c;
  if (commas == 0) {
    /* Indexed color */
    int index = strtoul(text + 1, &endp, 10);
    if (index >= 3)
      goto out;

    const Color slideShowColors[] = {
      SlideShowGetPrimaryColor(),
      SlideShowGetSecondaryColor(),
      SlideShowGetAccentColor()
    };
    c = slideShowColors[index];
  } else if (commas == 2) {
    /* RGB color */
    char* comma1 = strpbrk(text   + 1, ",");
    char* comma2 = strpbrk(comma1 + 1, ",");

    c.r = strtoul(text + 1, &endp, 10);
    if (endp != comma1)
      goto out;

    c.g = strtoul(comma1 + 1, &endp, 10);
    if (endp != comma2)
      goto out;

    c.b = strtoul(comma2 + 1, &endp, 10);
    if (endp != end)
      goto out;
  } else
    goto out;

  style->colorRed   = c.r;
  style->colorGreen = c.g;
  style->colorBlue  = c.b;
  return end + 1;
out:
  return text + 1;
}

static char* MaybeUpdateStyle(char* end, TextStyle* style) {
  switch (*end) {
    case '*':
      style->bold = !style->bold;
      break;
    case '_':
      style->italic = !style->italic;
      break;
    case '~':
      style->underline = !style->underline;
      break;
    case '`':
      style->code = !style->code;
      break;
    case COLOR_ESCAPE:
      return ParseColor(end, style);
    default:
      break;
  }
  return end + 1;
}

static void StripEscapeCharacters(char* str, size_t len) {
  char *p = str, *pEnd = str + len + 1;
  while (p < pEnd) {
    if (*p == '\\') {
      memmove(p, p + 1, pEnd - (p + 1));
      --pEnd;
      assert(*pEnd == '\0');
    }
    ++p;
  }
}

static char* NextStopChar(char* s, TextStyle* style) {
  /* NOTE: When we are in a block of code, we don't want to treat ~, *, _ as special characters. */
  if (style->code)
    return strpbrk(s, " `\33");
  else
    return strpbrk(s, " ~*_`\33");
}

static TextToken* Tokenize(char* line, char* lineEnd, TextStyle* style, int fontSize) {
  char* tokBegin = line;
  TextToken* firstToken = NULL;
  TextToken* lastToken = NULL;
  while (tokBegin < lineEnd) {
    char* tokEnd = tokBegin;
  continue_search:
    tokEnd = NextStopChar(tokEnd, style);
    if (tokEnd == NULL)
      tokEnd = lineEnd;
    else if (tokEnd > line && *tokEnd != ' ' && tokEnd[-1] == '\\') {
      ++tokEnd;
      goto continue_search;
    }

    size_t tokenLength = tokEnd - tokBegin;
    if (*tokEnd == ' ')
      ++tokenLength;
    else if (tokenLength == 0)
      goto next_token;

    TextToken* tok = ArenaAlloc(sizeof(*tok) + tokenLength + 1);
    if (tok == NULL)
      break;

    tok->length = tokenLength;

    strncpy(tok->string, tokBegin, tok->length);
    tok->string[tok->length] = '\0';

    /* Remove \ escape characters */
    StripEscapeCharacters(tok->string, tokenLength);

    tok->next = NULL;
    tok->style = *style;
    tok->width = TokenWidth(tok, fontSize);
    tok->joinWithNext = *tokEnd != '\0' && *tokEnd != ' ';

    if (*tokEnd == ' ') {
      tok->string[tok->length - 1] = '\0';
      tok->spaceWidth =  tok->width - TokenWidth(tok, fontSize);
      tok->string[tok->length - 1] = ' ';
    } else
      tok->spaceWidth = 1.0f;

    if /**/ (firstToken == NULL)
      firstToken = tok;
    else if (lastToken != NULL)
      lastToken->next = tok;

    lastToken = tok;

  next_token:
    /* Update style */
    tokBegin = MaybeUpdateStyle(tokEnd, style);
  }

  return firstToken;
}

static TextToken* DrawTokenChain(TextToken* tok, Vector2* cursor, int fontSize) {
  TokenDraw(tok, cursor, fontSize);
  if (tok->joinWithNext) {
    for (tok = tok->next; tok != NULL; tok = tok->next) {
      TokenDraw(tok, cursor, fontSize);
      if (!tok->joinWithNext)
        break;
    }
  }
  return tok;
}

static float GetTokenChainWidth(TextToken* tok, TextToken** lastToken) {
  float width = tok->width;
  TextToken* last = tok;
  if (tok->joinWithNext) {
    for (tok = tok->next; tok != NULL; tok = tok->next) {
      width += tok->width;
      last = tok;
      if (!tok->joinWithNext)
        break;
    }
  }

  *lastToken = last;
  return width;
}

static TextStyle GetDefaultTextStyle(void) {
  return TextBuildStyle(0, 0, 0, 0, SlideShowGetPrimaryColor());
}

static void DrawTextImpl(const char* txt, int fontSize, TextStyle style) {
  Rectangle rect = SlideSplitRect();
  const float lineHeight = fontSize + 8.0f;

  BeginScissorMode(rect.x, rect.y, rect.width, rect.height);

  ArenaPush();

  char *text = ArenaStrdup(txt);
  /* Split text into lines first */
  char* textEnd = SplitLines(text);

  ArenaPush();

  Vector2 cursor = { rect.x, rect.y };
  /* For every line in the text */
  char* lineEnd;
  for (char* line = text; line < textEnd; line = lineEnd + 1) {
    /* If the line is outside the rectangle, break out */
    if (cursor.y >= rect.y + rect.height)
      break;

    lineEnd = line + strlen(line);
    /* If the line is empty, increment the next line's Y coordinate and go to the next one */
    if (lineEnd == line)
      goto end;

    TextToken* tok = Tokenize(line, lineEnd, &style, fontSize);

    while (tok != NULL) {
      /* Compute joined width */
      TextToken* lastToken;
      float width = GetTokenChainWidth(tok, &lastToken);

      if (cursor.x + width > rect.x + rect.width) {
        /* If the line doesn't end with a space, we overflow */
        if (lastToken->string[lastToken->length - 1] != ' ')
          goto overflow;

        float width2 = width - lastToken->spaceWidth;
        /* If the line does end with a space, but we still don't fit in the line, we overflow */
        if (cursor.x + width2 > rect.x + rect.width)
          goto overflow;
      }

      tok = DrawTokenChain(tok, &cursor, fontSize);

      if (tok != NULL)
        tok = tok->next;

      if (cursor.x >= rect.x + rect.width) {
      overflow:
        cursor.x = rect.x;
        cursor.y += lineHeight;
        if (cursor.y >= rect.y + rect.height)
          break;
      }
    }
    ArenaFree();

  end:
    cursor.x = rect.x;
    cursor.y += lineHeight;
  }

  ArenaPop();

  EndScissorMode();

  ArenaPop();
}

void SlideText(const char* txt) {
  return SlideTextEx(txt, SlideShowGetTextFontSize());
}

void SlideTextEx(const char* txt, int fontSize) {
  SlideTextPro(txt, fontSize, GetDefaultTextStyle());
}

void SlideTextPro(const char* txt, int fontSize, TextStyle style) {
  WithShaderSDF() {
    /* NOTE: We do it like this to avoid binding and unbinding a shader every time we want to draw some text */
    DrawTextImpl(txt, fontSize, style);
  }
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
