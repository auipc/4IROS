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
