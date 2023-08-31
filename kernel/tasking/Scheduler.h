#pragma once

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

	static void schedule();
	static void schedule_no_save();

  private:
	static void reaper();
	static void yield();
	static Scheduler *s_the;
	static Process *s_current;
	Process *m_kreaper_process;
};
