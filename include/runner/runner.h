#ifndef RUNNER_H_
#define RUNNER_H_

#include <stddef.h>

typedef struct {
  void*  shellcodeAddress;
  size_t shellcodeLength;
  int    disableRWX;
} RunnerRequest;

typedef struct {
  void* shellcodeAddress;
} RunnerResponse;

#endif
