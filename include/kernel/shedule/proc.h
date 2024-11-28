#pragma once
#include <kernel/arch/amd64/kernel.h>
#include <kernel/arch/amd64/x86_64.h>
#include <kernel/mem/Paging.h>
#include <kernel/vfs/vfs.h>
#include <stddef.h>

#define STACK_SIZE 64*KB

extern KSyscallData ksyscall_data;
class Process;
struct Blocker {
	enum {
		ProcessBlock,
		SleepBlock,
		FileIOBlock
	} blocked_on;
	union {
		Process* blocked_process;
		uint64_t sleep_waiter_ms;
		FileHandle* blocked_handle;
	};
};

enum class ProState {
	Running,
	Blocked,
	Dead,	
};

class Process {
public:
	Process();
	Process(Process* p);
	static void sched(uintptr_t rsp);
	static void reentry();
	static int resolve_cow(uintptr_t fault_addr);
	static Process* create(void* func_ptr);
	static Process* create_user(const char* path, size_t argc=0, const char** argv=NULL);
	inline void kill() {
		m_state = ProState::Dead;
	}

	Process* fork(SyscallReg* regs);

	inline RootPageLevel* root_page_level() {
		return m_pglv;
	}

	inline uintptr_t kern_stack_top() {
		return m_kern_stack_top;
	}

	inline ProState state() {
		return m_state;
	}

	void block_on_proc(Process* p);
	void block_on_sleep(size_t ms);
	void block_on_handle(FileHandle* handle);
	void poll_is_blocked();

	int exit_code;
	uint64_t pid;
	Process* next;
	Process* prev;
	Vec<FileHandle*> m_file_handles;
private:
	Blocker m_blocker;
	ProState m_state = ProState::Running;
	uintptr_t m_stack_bot;
	uintptr_t m_stack_top;
	uintptr_t m_kern_stack_bot;
	uintptr_t m_kern_stack_top;
	uintptr_t m_actual_kern_stack_top;
	RootPageLevel* m_pglv;
};

extern Process* current_process;
