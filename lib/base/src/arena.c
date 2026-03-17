#include <base/arena.h>

#include <raylib.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

struct {
  uint8_t* mem;
  uint8_t* memEnd;
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

  Arena.mem    = chunk;
  Arena.memEnd = chunk + size;
  Arena.base     = chunk;
  Arena.top      = chunk;

  return 1;
}

void* ArenaAlloc(unsigned long size) {
  /* Always align size to 8 bytes */
  unsigned long chunkSize = (size + 0x7UL) & ~0x7UL;

  if (Arena.top + chunkSize > Arena.memEnd) {
    TraceLog(LOG_ERROR,
             "ARENA: OOM! Trying to allocated %zu bytes (arena top: %p, arena base: %p, arena mem: %p, arena size: %zu)",
             chunkSize, Arena.top, Arena.base, Arena.mem, Arena.memEnd - Arena.mem);
    return NULL;
  }

  void* chunk = Arena.top;
  Arena.top += chunkSize;
  TraceLog(LOG_TRACE, "ARENA: Allocating %zu bytes (chunk = %p, chunk end = %p)",
           chunkSize, chunk, Arena.top);
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
  if (Arena.base <= Arena.mem)
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
  Arena.base = Arena.mem;
  ArenaFree();
}
