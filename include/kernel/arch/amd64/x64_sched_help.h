#pragma once

//void x64_switch_save(void* src_stack, void* dest_stack, uint64_t dest_cr3);
extern "C" [[noreturn]] void x64_switch(void* dest_stack, uint64_t dest_cr3);
extern "C" uint64_t* x64_create_task_stack(uint64_t* stack);
