#pragma once
#include <kernel/idt.h>

namespace Syscall {
void handler(InterruptRegisters &regs);
};
