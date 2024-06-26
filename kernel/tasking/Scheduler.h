#pragma once
#include <kernel/util/Spinlock.h>

class Spinlock;
class Process;

class Scheduler {
  public:
	Scheduler();
	~Scheduler();

	void add_process(Process &p);
	void kill_process(Process &p);

	inline static Scheduler *the() { return s_the; }
	inline Process *current() { return s_current; }
	inline void set_current(Process *current) { s_current = current; }

	static void setup();

	static void schedule(uint32_t *esp);

	static Process *s_current;

	Spinlock sched_spinlock;

  private:
	static void reaper();
	static Scheduler *s_the;
	Process *m_kreaper_process;
};
