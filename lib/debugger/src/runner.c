#include <runner/runner.h>

#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/mman.h>

static int ReadRequest(RunnerRequest* request) {
  ssize_t rc = read(STDIN_FILENO, request, sizeof(*request));
  return -(rc != sizeof(*request));
}

static int MapMemoryRWX(void** outAddress, size_t length) {
  int extraFlags = *outAddress != NULL ? MAP_FIXED_NOREPLACE : 0;

  length = (length + 0xFFFUL) & ~0xFFFUL;
  *outAddress = mmap(*outAddress, length, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_ANONYMOUS | MAP_PRIVATE | extraFlags, -1, 0);
  return -(*outAddress == MAP_FAILED);
}

static int ReadShellcode(void* shellcodeAddress, size_t shellcodeLength) {
  ssize_t rc = read(STDIN_FILENO, shellcodeAddress, shellcodeLength);
  return -((size_t)rc != shellcodeLength);
}

static int SendResponse(RunnerResponse* response) {
  ssize_t rc = write(STDOUT_FILENO, response, sizeof(*response));
  return -(rc != sizeof(*response));
}

static int DisableRWX(void* shellcodeAddress, size_t shellcodeLength) {
  int rc;
  shellcodeLength = (shellcodeLength + 0xFFFUL) & ~0xFFFUL;
  rc = mprotect(shellcodeAddress, shellcodeLength, PROT_READ | PROT_EXEC);
  return -(rc < 0);
}

int main(void) {
  int rc;
  void* shellcodeAddress;
  RunnerRequest request;
  RunnerResponse response;

  /* Send OK signal to parent */
  rc = 0;
  if (write(2, &rc, sizeof(rc)) != sizeof(rc))
    return -1;

  /* If we fail in any of these steps,
   * the response will have a NULL shellcode address.
   */
  response.shellcodeAddress = NULL;

  rc = ReadRequest(&request);
  if (rc < 0)
    goto fail;

  shellcodeAddress = request.shellcodeAddress;
  rc = MapMemoryRWX(&shellcodeAddress, request.shellcodeLength);
  if (rc < 0)
    goto fail;

  rc = ReadShellcode(shellcodeAddress, request.shellcodeLength);
  if (rc < 0)
    goto fail;

  response.shellcodeAddress = shellcodeAddress;
  rc = SendResponse(&response);
  if (rc < 0)
    goto fail;

  if (request.disableRWX) {
    rc = DisableRWX(shellcodeAddress, request.shellcodeLength);
    if (rc < 0)
      goto fail;
  }

  /* We stop the process here and wait for the parent */
  rc = raise(SIGSTOP);
  if (rc < 0)
    goto fail;

  ((void (*)(void))shellcodeAddress)();
  return 0;
fail:
  SendResponse(&response);
  return rc;
}
