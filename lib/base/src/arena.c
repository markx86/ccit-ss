#include <base/arena.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

struct {
  uint8_t* chunk;
  uint8_t* chunkEnd;
  uint8_t* base;
  uint8_t* top;
} Arena;

int ArenaInit(unsigned long size) {
  if (size & 0xFFFUL)
    return 0;

  uint8_t* chunk = mmap(NULL,
                        size,
                        PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE,
                        -1,
                        0);
  if (chunk == MAP_FAILED)
    return 0;

  Arena.chunk    = chunk;
  Arena.chunkEnd = chunk + size;
  Arena.base     = chunk;
  Arena.top      = chunk;

  return 1;
}

void* ArenaAlloc(unsigned long size) {
  /* Always align size to 16 bytes */
  size = (size + 0xFUL) & ~0xFUL;

  if (Arena.top + size > Arena.chunkEnd)
    return NULL;

  void* chunk = Arena.top;
  Arena.top += size;
  return chunk;
}

char* ArenaStrdup(const char* str) {
  unsigned long strSize = strlen(str) + 1;
  char* duped = ArenaAlloc(strSize);
  memcpy(duped, str, strSize);
  return duped;
}

int ArenaPush(void) {
  void** prevBase = ArenaAlloc(sizeof(*prevBase));
  if (prevBase == NULL)
    return 0;

  *prevBase = Arena.base;
  Arena.base = Arena.top;
  return 1;
}

int ArenaPop(void) {
  if (Arena.base <= Arena.chunk)
    return 0;

  void** prevBase = (void**)(Arena.base - sizeof(*prevBase));
  Arena.base = *prevBase;
  Arena.top = (uint8_t*)prevBase;
  return 1;
}

void ArenaFree(void) {
  Arena.top = Arena.base;
}

void ArenaReset(void) {
  Arena.base = Arena.chunk;
  ArenaFree();
}
