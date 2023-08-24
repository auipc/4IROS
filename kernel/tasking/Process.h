#pragma once
#include <kernel/mem/Paging.h>
#include <kernel/stdint.h>

static const size_t STACK_SIZE = 4096;

class Process {
  public:
	Process(void *entry);
	~Process();
	void setup(void *entry);

	inline uintptr_t stack_top() { return m_stacktop; }
	inline Process *next() { return m_next; }
	inline Process *prev() { return m_prev; }
	inline void set_next(Process *next) { m_next = next; }
	inline void set_prev(Process *prev) { m_prev = prev; }

  private:
	friend class Scheduler;
	PageDirectory *m_page_directory;
	uintptr_t m_stacktop;
	uintptr_t m_stack;
	Process *m_next;
	Process *m_prev;
	uint32_t m_pid;
};
