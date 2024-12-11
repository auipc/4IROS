#pragma once
#include <kernel/arch/amd64/kernel.h>
#include <kernel/arch/amd64/x86_64.h>
#include <kernel/mem/Paging.h>
#include <kernel/vfs/vfs.h>
#include <stddef.h>
#include <kernel/util/HashTable.h>

#define STACK_SIZE 64 * KB

struct CoWInfo {
	RootPageLevel* dest;
	RootPageLevel* owner;
};

extern KSyscallData ksyscall_data;
class Process;
struct Blocker {
	enum { ProcessBlock, SleepBlock, FileIOBlock } blocked_on;
	union {
		Process *blocked_process;
		uint64_t sleep_waiter_ms;
		FileHandle *blocked_handle;
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
	Process(Process *p);
	~Process();
	static void init();
	static void sched(uintptr_t rsp);
	static void reentry();
	void collapse_cow();
	static int resolve_cow(uintptr_t fault_addr);

	static void kill_process(Process* p);
	static Process *create(void *func_ptr);
	static Process *create_user(const char *path, size_t argc = 0,
								const char **argv = NULL);
	inline void kill() { m_state = ProState::Dead; }
	inline void set_state(ProState state) { m_state = state; }

	Process *fork(SyscallReg *regs);

	inline RootPageLevel *root_page_level() { return m_pglv; }

	inline uintptr_t kern_stack_top() { return m_kern_stack_top; }

	inline ProState state() { return m_state; }

	void block_on_proc(Process *p);
	void block_on_sleep(size_t ms);
	void block_on_handle(FileHandle *handle);
	void poll_is_blocked();

	inline Process* parent() { return m_parent; }
	inline void set_parent(Process* p) { 
		m_parent = p; 
	}

	inline void add_child(Process* p) { 
		m_children.push(p); 
	}

	int exit_code;
	uint64_t pid;
	Process *next;
	Process *prev;
	Vec<FileHandle *> m_file_handles;
	HashTable<CoWInfo>* cow_table;
	bool dont_goto_me = false;
	uintptr_t m_actual_kern_stack_top;
	uintptr_t m_kern_stack_top;
	uintptr_t m_saved_user_stack_ksyscall;
  private:
	Process* m_parent;
	Vec<Process*> m_children;
	Blocker m_blocker;
	ProState m_state = ProState::Running;
	uintptr_t m_stack_bot;
	uintptr_t m_stack_top;
	uintptr_t m_kern_stack_bot;
	uint8_t* m_saved_fpu;
	RootPageLevel *m_pglv;
};

extern Process *current_process;
