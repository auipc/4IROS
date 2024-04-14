#pragma once
#include <kernel/idt.h>
#include <kernel/mem/Paging.h>
#include <kernel/util/Vec.h>
#include <kernel/vfs/vfs.h>
#include <stdint.h>

static const size_t STACK_SIZE = 4096;
static const size_t USER_STACK_SIZE = 512 * KB;

class Process {
  public:
	Process(void *entry, bool userspace = false);
	Process(VFSNode *executable);
	Process(Process &parent, InterruptRegisters &regs);
	~Process();
	void setup(void *entry);
	Process *fork(InterruptRegisters &regs);

	inline uintptr_t stack_top() { return m_stack_top; }
	inline Process *next() { return m_next; }
	inline Process *prev() { return m_prev; }
	inline void set_next(Process *next) { m_next = next; }
	inline void set_prev(Process *prev) { m_prev = prev; }
	inline PageDirectory *page_directory() { return m_page_directory; }

	enum class States { Dead, Blocked, Active };

	inline void set_state(States state) { m_state = state; }
	inline States state() { return m_state; }
	inline uint32_t pid() { return m_pid; }

	Vec<FileHandle *> *m_file_descriptors;

	FileHandle *m_blocked_source;

	inline void set_pid(uint32_t pid) { m_pid = pid; }

	struct ProcessAlert {
		Process *listener;
		int *alert_status_ptr;
	};

	inline void set_alert(ProcessAlert alerts) { m_alerts = alerts; }

	inline ProcessAlert alert() { return m_alerts; }

  private:
	friend class Scheduler;
	PageDirectory *m_page_directory;
	uintptr_t m_stack_base;
	uintptr_t m_stack_top;
	uintptr_t m_user_stack_base;
	uintptr_t m_user_stack_top;
	Process *m_next;
	Process *m_prev;
	uint32_t m_pid;
	bool m_userspace;
	States m_state = States::Active;
	ProcessAlert m_alerts;
};
