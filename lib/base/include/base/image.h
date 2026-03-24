#ifndef IMAGE_H_
#define IMAGE_H_

#include <raylib.h>

Texture2D ImageGet(const char* path);
Texture2D ImageGetSVG(const char* path, int width, int height);
void      ImageClearCache(void);

#endif
