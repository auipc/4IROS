#pragma once
#include <kernel/tasking/Process.h>

class Scheduler {
  public:
	Scheduler();
	~Scheduler();

	inline static Scheduler *the() { return s_the; }

	static void setup();

	void schedule();

  private:
	static Scheduler *s_the;
	Process *m_current;
};