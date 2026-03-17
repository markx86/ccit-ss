#ifndef ARENA_H_
#define ARENA_H_

int   ArenaInit(unsigned long size);
void* ArenaAlloc(unsigned long size);
char* ArenaStrdup(const char* str);
int   ArenaPush(void);
int   ArenaPop(void);
void  ArenaFree(void);
void  ArenaReset(void);

#endif
