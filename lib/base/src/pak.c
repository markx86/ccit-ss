#include <base/pak.h>

#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PAK_EXT ".pak"

typedef struct {
  unsigned long hash;
  unsigned int  offset;
  unsigned int  size;
  char          fileName[];
} PakHeaderEntry;

typedef struct PakEntry {
  struct PakEntry* next;
  PakHeaderEntry e;
} PakEntry;

static FILE* pakFile;
static PakEntry* pakEntries;

static unsigned long ComputeFileHash(const char* str) {
  unsigned long hash = 0;
  unsigned char c;

  while ((c = *(str++)))
    hash = c + (hash << 6) + (hash << 16) - hash;

  return hash;
}

static PakHeaderEntry* FindHeaderEntry(const char* path, unsigned long hash) {
  PakEntry* entry = pakEntries;
  while (entry != NULL) {
    if (entry->e.hash == 0) {
      if (!strcmp(path, entry->e.fileName))
        return &entry->e;
    } else if (entry->e.hash == hash)
      return &entry->e;
    entry = entry->next;
  }
  return NULL;
}

static unsigned char* LoadFileFromPak(const char* path, int* size) {
  if (pakFile == NULL)
    return NULL;

  unsigned long hash = ComputeFileHash(path);

  PakHeaderEntry* e = FindHeaderEntry(path, hash);
  if (e == NULL) {
    TraceLog(LOG_WARNING, "[PAKIO] Could not find file '%s' (hash: %016X)", path, hash);
    return NULL;
  }

  if (fseek(pakFile, e->offset, SEEK_SET) < 0) {
    TraceLog(LOG_ERROR, "[PAKIO] Could not seek to %08x (file: '%s', hash: %016X)", e->offset, path, hash);
    return NULL;
  }

  unsigned char* buffer = MemAlloc(e->size);
  if (buffer == NULL) {
    TraceLog(LOG_ERROR, "[PAKIO] Could not allocate buffer for file '%s' (hash: %016X)", path, hash);
    return NULL;
  }

  TraceLog(LOG_INFO, "[PAKIO] Reading %u bytes @ offset %08X (file: '%s', hash: %016X)", e->size, e->offset, path, hash);
  if (fread(buffer, e->size, 1, pakFile) != 1) {
    TraceLog(LOG_ERROR, "[PAKIO] Could not read file '%s' (hash: %016X)", path, hash);
    MemFree(buffer);
    return NULL;
  }

  *size = e->size;
  return buffer;
}

static unsigned char* PakLoadFileData(const char* path, int* size) {
  if (size == NULL || path == NULL)
    return NULL;

  if (!strncmp(path, "pak://", 6))
    return LoadFileFromPak(path + 6, size);

  /* Dirty hack to call the original function */
  SetLoadFileDataCallback(NULL);
  unsigned char* data = LoadFileData(path, size);
  SetLoadFileDataCallback(PakLoadFileData);

  return data;
}

static int GetPakFileName(char* buffer, size_t bufferSize) {
  ssize_t pathLen = readlink("/proc/self/exe", buffer, bufferSize);
  if (pathLen < 0 || (size_t)pathLen >= bufferSize - sizeof(PAK_EXT))
    return 0;

  char* ext;
  if ((ext = strrchr(buffer, '.')) == NULL)
    ext = buffer + pathLen;

  strcpy(ext, PAK_EXT);
  return 1;
}

static PakEntry* BuildEntriesList(void* header, int headerSize) {
  void* entriesEnd = (unsigned char*)header + headerSize;
  PakEntry* prevEntry = NULL;
  PakEntry* entriesList = NULL;

  for (PakHeaderEntry* e = header; (void*)e < entriesEnd;) {
    int fileNameSize = e->hash ? 0 : strlen(e->fileName) + 1;

    PakEntry* entry = MemAlloc(sizeof(PakEntry) + fileNameSize);
    if (entry == NULL)
      break;

    /* Set list head */
    if (entriesList == NULL)
      entriesList = entry;

    /* Link the list */
    if (prevEntry != NULL)
      prevEntry->next = entry;
    prevEntry = entry;

    entry->e = *e;
    strncpy(entry->e.fileName, e->fileName, fileNameSize);

    /* Go to next entry */
    e = (PakHeaderEntry*)((unsigned char*)(e + 1) + fileNameSize);
  }

  return entriesList;
}

static int ParseHeader(unsigned int headerSize) {
  void* headerData = MemAlloc(headerSize);
  if (headerData == NULL)
    return 0;

  if (fread(headerData, headerSize, 1, pakFile) != 1) {
    MemFree(headerData);
    return 0;
  }

  pakEntries = BuildEntriesList(headerData, headerSize);
  MemFree(headerData);
  return 1;
}

void UnloadPakFile(void) {
  if (pakFile == NULL)
    return;

  SetLoadFileDataCallback(NULL);

  fclose(pakFile);
  pakFile = NULL;

  PakEntry* entry = pakEntries;
  while (entry != NULL) {
    void* ptr = entry;
    entry = entry->next;
    MemFree(ptr);
  }
  pakEntries = NULL;
}

int LoadPakFile(void) {
  if (pakFile != NULL)
    return 1;

  char path[4096];
  if (!GetPakFileName(path, sizeof(path)))
    return 0;

  pakFile = fopen(path, "rb");
  if (pakFile == NULL)
    return 0;

  /* Check file magic */
  unsigned int magic;
  if (fread(&magic, 4, 1, pakFile) != 1 || magic != 0xDEFEC8ED)
    return 0;

  /* Read header size */
  unsigned int headerSize;
  if (fread(&headerSize, 4, 1, pakFile) != 1 || headerSize == 0)
    return 0;

  /* Read and parse file entries */
  if (!ParseHeader(headerSize))
    return 0;

  SetLoadFileDataCallback(PakLoadFileData);
  return 1;
}
