#pragma once
#include <kernel/util/Smrt.h>
#include <stdint.h>

void screen_init();
void printk(const char *str, ...);
[[noreturn]] void panic(const char *str);
