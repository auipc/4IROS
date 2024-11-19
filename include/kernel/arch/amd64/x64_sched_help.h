#pragma once

//void x64_switch_save(void* src_stack, void* dest_stack, uint64_t dest_cr3);
extern "C" [[noreturn]] void x64_switch(void* dest_stack, uint64_t dest_cr3);
extern "C" [[noreturn]] void x64_switch_fake();
extern "C" uint64_t* x64_create_kernel_task_stack(uint64_t* stack, uintptr_t rip);
extern "C" uint64_t* x64_create_user_task_stack(uint64_t* stack, uintptr_t rip, uint64_t* user_stack);
extern "C" uint64_t* x64_syscall_block_help(uint64_t* stack, uintptr_t rip);
extern "C" uint64_t get_rip();
