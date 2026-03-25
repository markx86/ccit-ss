#include <debugger/debugger.h>
#include <runner/runner.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <linux/limits.h>

typedef union {
  int raw[2];
  struct {
    int read;
    int write;
  };
} ProcPipe;

typedef struct{
  int pid;
  int stdin;
  int stdout;
  int stderr;
} Runner;

#define MAX_BREAKPOINTS      32
#define MAX_SHELLCODE_LENGTH 0x10000

struct Debugger {
  Runner*     runner;
  void*       shellcodeAddress;
  uint8_t*    shellcode;
  size_t      shellcodeLength;
  int         restartSignal;
  Registers   regs;
  Breakpoint* currentBreakpoint;
  Breakpoint  breakpoints[MAX_BREAKPOINTS];
};

static char RunnerPath[PATH_MAX];

static int CreatePipe(ProcPipe* pipeStruct) {
  return -(pipe(pipeStruct->raw) < 0);
}

static int ExecRunner(ProcPipe* stdin, ProcPipe* stdout, ProcPipe* stderr) {
  close(stdin->write);
  close(stdout->read);
  close(stderr->read);

  dup2(stdin->read,   0);
  dup2(stdout->write, 1);
  dup2(stderr->write, 2);

  close(stdin->read);
  close(stdout->write);
  close(stderr->write);

  return execve(RunnerPath,
                (char*[]) { RunnerPath, NULL },
                (char*[]) { NULL });
}

static Runner* StartRunner(void) {
  ProcPipe stdinPipe, stdoutPipe, stderrPipe;

  if (CreatePipe(&stdinPipe) < 0 || CreatePipe(&stdoutPipe) < 0 || CreatePipe(&stderrPipe) < 0) {
    fprintf(stderr, "Could not create pipes! %m\n");
    return NULL;
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(stderr, "Could not fork: %m\n");
    return NULL;
  }

  if (pid == 0) {
    int rc = ExecRunner(&stdinPipe, &stdoutPipe, &stderrPipe);
    int error = errno;
    write(2, &error, sizeof(error));
    exit(rc);
  } else {
    close(stdinPipe.read);
    close(stdoutPipe.write);
    close(stderrPipe.write);
  }

  int error = 0;
  if (read(stderrPipe.read, &error, sizeof(error)) != sizeof(error) || error != 0) {
    fprintf(stderr, "Failed to start '%s': %s\n", RunnerPath, error == 0 ? "Broken pipe" : strerror(error));
    /* Close the pipes */
    close(stdinPipe.write);
    close(stdoutPipe.read);
    close(stderrPipe.read);
    return NULL;
  }

  Runner* runner = calloc(1, sizeof(*runner));
  assert(runner != NULL);

  runner->pid = pid;
  runner->stdin = stdinPipe.write;
  runner->stdout = stdoutPipe.read;
  runner->stderr = stderrPipe.read;

  return runner;
}

static void* LoadShellcode(Runner* runner,
                           const uint8_t* shellcode,
                           size_t shellcodeLength,
                           void* shellcodeAddress,
                           int disableRWX) {
  RunnerRequest request = {
    .shellcodeAddress = shellcodeAddress,
    .shellcodeLength = shellcodeLength,
    .disableRWX = !!disableRWX
  };

  if (write(runner->stdin, &request, sizeof(request)) < 0) {
    fprintf(stderr, "Could not send initialization data to runner! %m\n");
    return NULL;
  }

  if (write(runner->stdin, shellcode, shellcodeLength) < 0) {
    fprintf(stderr, "Could not load shellcode! %m\n");
    return NULL;
  }

  RunnerResponse response;
  if (read(runner->stdout, &response, sizeof(response)) < 0
      || response.shellcodeAddress == NULL) {
    fprintf(stderr, "Could not initialize runner! %m\n");
    return NULL;
  }

  return response.shellcodeAddress;
}

static void RunnerFree(Runner* runner) {
  int wstatus = 0;
  int rc = waitpid(runner->pid, &wstatus, WNOHANG);
  if (rc == 0 || (rc > 0 && !WIFEXITED(wstatus)))
    kill(runner->pid, SIGKILL);

  close(runner->stdin);
  close(runner->stdout);
  close(runner->stderr);

  free(runner);
}

static int DebuggerPtrace(Debugger* debugger, int op,
                           void* addr, void* data) {
  return syscall(__NR_ptrace, op, debugger->runner->pid, addr, data);
}

static Breakpoint* GetNextFreeBreakpoint(Debugger* debugger) {
  for (int i = 0; i < MAX_BREAKPOINTS; ++i) {
    if (!debugger->breakpoints[i].active)
      return &debugger->breakpoints[i];
  }
  return NULL;
}

static void* BreakpointAddress(Debugger* debugger, Breakpoint* breakpoint) {
  return (void*)((uintptr_t)debugger->shellcodeAddress + breakpoint->offset);
}

static int SetBreakpoint(Debugger* debugger, Breakpoint* breakpoint) {
  void* address = BreakpointAddress(debugger, breakpoint);
  if (DebuggerMemRead(debugger, address, &breakpoint->byte, 1) < 0)
    return -1;
  return DebuggerMemWrite(debugger, address, (uint8_t[]) { 0xCC }, 1);
}

static int UnsetBreakpoint(Debugger* debugger, Breakpoint* breakpoint) {
  void* address = BreakpointAddress(debugger, breakpoint);
  return DebuggerMemWrite(debugger, address, &breakpoint->byte, 1);
}

static Breakpoint* GetBreakpointAt(Debugger* debugger, void* address) {
  intptr_t offset = address - debugger->shellcodeAddress;

  if (offset < 0 || offset > MAX_SHELLCODE_LENGTH)
    return NULL;

  for (size_t i = 0; i < MAX_BREAKPOINTS; ++i) {
    Breakpoint* breakpoint = &debugger->breakpoints[i];
    if (breakpoint->active && breakpoint->offset == offset)
      return breakpoint;
  }

  return NULL;
}

static void RestoreInt3(Debugger* debugger) {
  if (debugger->currentBreakpoint != NULL) {
    SetBreakpoint(debugger, debugger->currentBreakpoint);
    debugger->currentBreakpoint = NULL;
  }
}

static void RemoveInt3(Debugger* debugger) {
  Breakpoint* breakpoint = GetBreakpointAt(debugger, (void*)(debugger->regs.rip - 1));
  if (breakpoint == NULL)
    return;

  UnsetBreakpoint(debugger, breakpoint);
  debugger->regs.rip -= 1;
  debugger->currentBreakpoint = breakpoint;
}

static int ReadRegs(Debugger* debugger) {
  struct user_regs_struct uregs;
  Registers* regs;

  if (DebuggerPtrace(debugger, PTRACE_GETREGS, 0, &uregs) < 0)
    return -1;

  regs = &debugger->regs;

  regs->rflags = uregs.eflags;
#define COPYREG(name) regs->name = uregs.name
  COPYREG(rip);
  COPYREG(rax);
  COPYREG(rbx);
  COPYREG(rcx);
  COPYREG(rdx);
  COPYREG(rdi);
  COPYREG(rsi);
  COPYREG(rsp);
  COPYREG(rbp);
  COPYREG(r8);
  COPYREG(r9);
  COPYREG(r10);
  COPYREG(r11);
  COPYREG(r12);
  COPYREG(r13);
  COPYREG(r14);
  COPYREG(r15);
#undef COPYREG

  return 0;
}

static int WaitChild(Debugger* debugger, int* childTerminated) {
  *childTerminated = 0;

  int wstatus = 0;
  if (waitpid(debugger->runner->pid, &wstatus, 0) < 0)
    return -1;

  if (WIFSIGNALED(wstatus) || WIFEXITED(wstatus)) {
    *childTerminated = 1;
    return 0;
  }

  if (ReadRegs(debugger) < 0)
    return -1;

  siginfo_t siginfo;
  if (DebuggerPtrace(debugger, PTRACE_GETSIGINFO, 0, &siginfo) < 0)
    debugger->restartSignal = 0;
  else
    debugger->restartSignal = siginfo.si_signo;

  return 0;
}

static int HasHitBreakpoint(Debugger* debugger) {
  if (debugger->restartSignal == SIGTRAP) {
    debugger->restartSignal = 0;
    return 1;
  }
  return 0;
}

void DebuggerSetRunnerPath(const char* runnerPath) {
  strncpy(RunnerPath, runnerPath, sizeof(RunnerPath)-1);
}

Debugger* DebugShellcode(const uint8_t* shellcode, size_t shellcodeLength, void* shellcodeAddress, int disableRWX) {
  Debugger* debugger;
  Runner* runner;

  if (shellcodeLength == 0 || shellcodeLength > MAX_SHELLCODE_LENGTH)
    return NULL;

  runner = StartRunner();
  if (runner == NULL) {
    fputs("Could not start runner!\n", stderr);
    return NULL;
  }

  debugger = calloc(1, sizeof(*debugger));
  assert(debugger != NULL);

  debugger->runner = runner;
  debugger->shellcodeLength = shellcodeLength;
  debugger->shellcode = malloc(debugger->shellcodeLength);
  assert(debugger->shellcode != NULL);
  memcpy(debugger->shellcode, shellcode, debugger->shellcodeLength);

  debugger->shellcodeAddress = LoadShellcode(runner, shellcode, shellcodeLength, shellcodeAddress, disableRWX);
  if (debugger->shellcodeAddress == NULL) {
    fputs("Could not load shellcode!\n", stderr);
    goto fail;
  }

  if (DebuggerPtrace(debugger, PTRACE_ATTACH, 0, 0) < 0) {
    fprintf(stderr, "Could not attach to runner: %m\n");
    goto fail;
  }

  int childTerminated = 0;
  if (WaitChild(debugger, &childTerminated) < 0) {
    fprintf(stderr, "Could not wait for runner: %m\n");
    goto fail;
  }

  if (childTerminated) {
    fputs("Runner exited early.", stderr);
    goto fail;
  }

  return debugger;
fail:
  DebuggerFree(debugger);
  return NULL;
}

void DebuggerFree(Debugger* debugger) {
  if (debugger->runner != NULL)
    RunnerFree(debugger->runner);
  free(debugger->shellcode);
  free(debugger);
}

int DebuggerContinue(Debugger* debugger) {
  int rc;

  rc = DebuggerStep(debugger);
  if (rc < 0)
    return -1;
  else if (rc == 1)
    return 1;

  rc = DebuggerPtrace(debugger, PTRACE_CONT, 0, (void*)(uintptr_t)debugger->restartSignal);
  return rc;
}

int DebuggerStep(Debugger* debugger) {
  int rc = DebuggerPtrace(debugger, PTRACE_SINGLESTEP, 0, (void*)(uintptr_t)debugger->restartSignal);
  if (rc < 0)
    return -1;

  int childTerminated = 0;
  if (WaitChild(debugger, &childTerminated) < 0)
    return -1;

  if (!childTerminated) {
    RestoreInt3(debugger);
    if (HasHitBreakpoint(debugger))
      RemoveInt3(debugger);
  }

  return childTerminated;
}

const Registers* DebuggerGetRegs(Debugger* debugger) {
  return &debugger->regs;
}

int DebuggerMemWrite(Debugger* debugger, void* address, const void* buffer, size_t bufferLength) {
  void* data;
  uint8_t* addr;
  const uint8_t* bufp;
  size_t words = bufferLength / sizeof(void*);

  addr = address;
  bufp = buffer;
  for (size_t i = 0; i < words; ++i) {
    data = *(void**)bufp;
    if (DebuggerPtrace(debugger, PTRACE_POKEDATA, addr, data) < 0) {
      fprintf(stderr, "Could not write process memory @ %p! %m\n", addr);
      return -1;
    }
    addr += sizeof(void*);
    bufp += sizeof(void*);
  }

  if (DebuggerPtrace(debugger, PTRACE_PEEKDATA, addr, &data) < 0) {
    fprintf(stderr, "Could not read process memory @ %p! %m\n", addr);
    return -1;
  }

  memcpy(&data, bufp, bufferLength - words*sizeof(void*));

  if (DebuggerPtrace(debugger, PTRACE_POKEDATA, addr, data) < 0) {
    fprintf(stderr, "Could not write process memory @ %p! %m\n", addr);
    return -1;
  }

  return 0;
}

int DebuggerMemRead(Debugger* debugger, void* address, void* buffer, size_t bufferLength) {
  size_t words = bufferLength / sizeof(long);
  uint8_t* addr = address;
  uint8_t* bufp = buffer;
  long data;

  for (size_t i = 0; i < words; ++i) {
    if (DebuggerPtrace(debugger, PTRACE_PEEKDATA, addr, &data) < 0) {
      fprintf(stderr, "Could not read process memory @ %p! %m\n", addr);
      return -1;
    }
    *(long*)bufp = data;
    addr += sizeof(long);
    bufp += sizeof(long);
  }

  if (DebuggerPtrace(debugger, PTRACE_PEEKDATA, addr, &data) < 0) {
    fprintf(stderr, "Could not read process memory @ %p! %m\n", addr);
    return -1;
  }

  memcpy(&data, bufp, bufferLength - words*sizeof(void*));
  return 0;
}

int DebuggerSetBreakpoint(Debugger* debugger, void* address) {
  intptr_t shellcodeOffset = address - debugger->shellcodeAddress;

  if (shellcodeOffset < 0 || shellcodeOffset >= MAX_SHELLCODE_LENGTH)
    return -1;

  Breakpoint* breakpoint = GetNextFreeBreakpoint(debugger);
  if (breakpoint == NULL)
    return -1;

  breakpoint->active = 1;
  breakpoint->offset = shellcodeOffset;
  return SetBreakpoint(debugger, breakpoint);
}

int DebuggerUnsetBreakpoint(Debugger* debugger, void* address) {
  Breakpoint* breakpoint = GetBreakpointAt(debugger, address);
  if (breakpoint == NULL)
    return -1;

  breakpoint->active = 0;
  return UnsetBreakpoint(debugger, breakpoint);
}

const Breakpoint* DebuggerGetCurrentBreakpoint(Debugger* debugger) {
  return debugger->currentBreakpoint;
}

int DebuggerWait(Debugger* debugger) {
  int childTerminated;

  if (WaitChild(debugger, &childTerminated) < 0)
    return -1;

  if (!childTerminated && HasHitBreakpoint(debugger))
    RemoveInt3(debugger);

  return childTerminated;
}
