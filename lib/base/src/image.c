#include <base/image.h>

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

Texture2D ImageGet(const char* path) {
  unsigned long hash = PathHash(path);
  Texture2D texture;

  if (GetFromCache(hash, &texture))
    return texture;

  texture = LoadTexture(path);
  if (IsTextureValid(texture))
    InsertIntoCache(hash, texture);

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
