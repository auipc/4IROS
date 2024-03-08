#include <kernel/util/Spinlock.h>

Spinlock::Spinlock() {}

Spinlock::~Spinlock() {}

void Spinlock::acquire() {
	bool comparison = false;
	while (__atomic_compare_exchange_n(&m_locked, &comparison, 1, 0,
									   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		;
}

void Spinlock::release() { __atomic_store_n(&m_locked, 0, __ATOMIC_SEQ_CST); }
