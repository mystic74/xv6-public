#include "syscall.h"
#include "traps.h"

.globl start_sigret
.globl end_sigret

start_sigret:
  movl $SYS_sigret, %eax
  int  $T_SYSCALL
end_sigret: