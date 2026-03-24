#include <base/image.h>

#include <nanosvg.h>
#include <nanosvgrast.h>

#include <stddef.h>
#include <string.h>

typedef struct TextureEntry {
  struct TextureEntry* next;
  Texture2D texture;
  unsigned long hash;
} TextureEntry;

TextureEntry* Textures = NULL;

static unsigned long PathHash(const char* str) {
  unsigned long hash = 0;
  unsigned char c;

  while ((c = *(str++)))
    hash = c + (hash << 6) + (hash << 16) - hash;

  return hash;
}

static int GetFromCache(unsigned long hash, Texture2D* texture) {
  TextureEntry* tex = Textures;
  while (tex != NULL) {
    if (tex->hash == hash) {
      *texture = tex->texture;
      return 1;
    }
    tex = tex->next;
  }

  return 0;
}

static void InsertIntoCache(unsigned long hash, Texture2D texture) {
  TextureEntry* entry = MemAlloc(sizeof(*entry));
  if (entry == NULL)
    return;

  entry->texture = texture;
  entry->hash = hash;
  entry->next = Textures;
  Textures = entry;
}

/* Load SVG image, rasteraizing it at desired width and height
 * NOTE: If width/height are 0, using internal default width/height
 *
 * NOTE: The following function was stolen from
 *       https://github.com/raylib-extras/examples-c/blob/main/textures_svg_loading/textures_svg_loading.c
 */
static Image LoadImageSVG(const char *fileName, int width, int height)
{
  Image image = { 0 };

  const char* fileExtension = GetFileExtension(fileName);

  // if (width == 0 && height == 0)
  //   goto fail;

  if (strcmp(fileExtension, ".svg") && strcmp(fileExtension, ".SVG"))
    goto fail;

  int dataSize = 0;
  unsigned char* fileData = NULL;

  fileData = LoadFileData(fileName, &dataSize);

  /* Make sure the file data contains an EOL character: '\0' */
  if ((dataSize > 0) && (fileData[dataSize - 1] != '\0')) {
    fileData = MemRealloc(fileData, dataSize + 1);
    fileData[dataSize] = '\0';
    dataSize += 1;
  }

  if (fileData == NULL)
    goto fail;

  unsigned char* svgStart = fileData;
  /* Validate fileData as valid SVG string data */
  if (strncmp((char*)svgStart, "<svg", 4))
    svgStart = memmem(fileData, dataSize, "<svg", 4);

  if (svgStart == NULL)
    goto fail_unload_file;

  struct NSVGimage* svgImage = nsvgParse((char*)fileData, "px", 96.0f);

  /* NOTE: If required width or height is 0, using default SVG internal value */
  if (width == 0) width = svgImage->width;
  if (height == 0) height = svgImage->height;

  unsigned char* imgData = MemAlloc(width * height * 4);

  /* Calculate scales for both the width and the height */
  float scaleWidth = width/svgImage->width;
  float scaleHeight = height/svgImage->height;

  /* Set the largest of the 2 scales to be the scale to use */
  float scale = (scaleHeight > scaleWidth) ? scaleWidth : scaleHeight;

  int offsetX = 0;
  int offsetY = 0;

  if (scaleHeight > scaleWidth) offsetY = (height - svgImage->height * scale) / 2;
  else offsetX = (width - svgImage->width * scale) / 2;

  /* Rasterize */
  struct NSVGrasterizer* rast = nsvgCreateRasterizer();
  nsvgRasterize(rast, svgImage, offsetX, offsetY, scale, imgData, width, height, width * 4);

  /* Populate image struct with all data */
  image.data = imgData;
  image.width = width;
  image.height = height;
  image.mipmaps = 1;
  image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

  nsvgDelete(svgImage);
  nsvgDeleteRasterizer(rast);

fail_unload_file:
  UnloadFileData(fileData);
fail:
  return image;
}

Texture2D ImageGet(const char* path) {
  unsigned long hash = PathHash(path);
  Texture2D texture;

  if (GetFromCache(hash, &texture))
    return texture;

  texture = LoadTexture(path);
  if (IsTextureValid(texture)) {
    InsertIntoCache(hash, texture);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
  }

  return texture;
}

Texture2D ImageGetSVG(const char* path, int width, int height) {
  unsigned long hash = PathHash(path);
  Texture2D texture;

  if (GetFromCache(hash, &texture)) {
    if ((width == 0 || texture.width == width) && (height == 0 || texture.height == height))
      return texture;
    UnloadTexture(texture);
  }

  Image image = LoadImageSVG(path, width, height);
  texture = LoadTextureFromImage(image);
  UnloadImage(image);

  if (IsTextureValid(texture)) {
    InsertIntoCache(hash, texture);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
  }

  return texture;
}

void ImageClearCache(void) {
  while (Textures != NULL) {
    TextureEntry* entry = Textures;
    UnloadTexture(entry->texture);
    Textures = Textures->next;
    MemFree(entry);
  }
}
