#pragma once

class Spinlock {
  public:
	Spinlock();
	~Spinlock();

	void acquire();
	void release();

  private:
	bool m_locked;
};

class SpinlockRAII {
  public:
	SpinlockRAII() { lock.acquire(); }
	~SpinlockRAII() { lock.release(); }

  private:
	Spinlock lock;
};
