#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <stddef.h>
#include <stdint.h>

typedef struct Debugger Debugger;

#define REG8(name, namel, nameh)      \
  union {                             \
    struct { uint8_t namel, nameh; }; \
    uint16_t name;                    \
    uint32_t e##name;                 \
    uint64_t r##name;                 \
  }

#define REG16(name)    \
  union {              \
    uint8_t  name##l;  \
    uint16_t name;     \
    uint32_t e##name;  \
    uint64_t r##name;  \
  }

#define REG64(name)    \
  union {              \
    uint8_t  name##b;  \
    uint16_t name##w;  \
    uint32_t name##d;  \
    uint64_t name;     \
  };

typedef struct {
  /* Special regs */
  uint64_t rip;
  uint64_t rflags;
  /* x86 registers */
  REG8(ax, al, ah);
  REG8(bx, bl, bh);
  REG8(cx, cl, ch);
  REG8(dx, dl, dh);
  REG16(di);
  REG16(si);
  REG16(sp);
  REG16(bp);
  /* x86-64 registers */
  REG64(r8);
  REG64(r9);
  REG64(r10);
  REG64(r11);
  REG64(r12);
  REG64(r13);
  REG64(r14);
  REG64(r15);
} Registers;

typedef struct {
  uint16_t offset;
  uint8_t  byte;
  uint8_t  active;
} Breakpoint;

Debugger* DebugShellcode(const uint8_t* shellcode, size_t shellcodeLength, void* shellcodeAddress, int disableRWX);
void DebuggerFree(Debugger* debugger);
int  DebuggerWait(Debugger* debugger);
int  DebuggerContinue(Debugger* debugger);
int  DebuggerStep(Debugger* debugger);
int  DebuggerSetBreakpoint(Debugger* debugger, void* address);
int  DebuggerUnsetBreakpoint(Debugger* debugger, void* address);
const Breakpoint* DebuggerGetCurrentBreakpoint(Debugger* debugger);
const Registers*  DebuggerGetRegs(Debugger* debugger);
int  DebuggerMemWrite(Debugger* debugger, void* address, const void* buffer, size_t bufferLength);
int  DebuggerMemRead(Debugger* debugger, void* address, void* buffer, size_t bufferLength);

#endif
