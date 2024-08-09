// Ism - the C++-ism file
#pragma once

// Casts an L-Value to an R-Value.
template <class T> T &&move(T &v) { return static_cast<T &&>(v); }

#define READ_REGISTER(reg, var) asm volatile("mov %%" reg ", %0" : "=a"(var));
