#ifndef IMAGE_H_
#define IMAGE_H_

#include <raylib.h>

Texture2D ImageGet(const char* path);
void      ImageClearCache(void);

#endif
