#pragma once
#include <kernel/tasking/Process.h>

class Scheduler {
  public:
	Scheduler();
	~Scheduler();

	inline static Scheduler *the() { return s_the; }
	inline Process *current() { return m_current; }
	inline void set_current(Process* current) {
		m_current = current;
	}

	static void setup();

	static void schedule();

  private:
	static Scheduler *s_the;
	Process *m_current;
};
