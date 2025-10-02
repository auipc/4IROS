#include <kernel/driver/TTY.h>
#include <kernel/minmax.h>
#include <kernel/printk.h>
#include <kernel/shedule/proc.h>
#include <kernel/util/Spinlock.h>

// FIXME figure out how to write larger than this size
#define TTY_BUFFER_SZ (8192 * 4)
Spinlock spin;

void *memmove(void *s1, const void *s2, size_t n) {
	if (s1 == s2)
		return s1;

	// FIXME
	char *m = (char *)kmalloc(n);
	memcpy(m, s2, n);
	memcpy(s1, m, n);
	kfree(m);
	return s1;
}

TTY::TTY() : VFSNode("tty") { m_buffer = new char[TTY_BUFFER_SZ]; }

size_t TTY::size() {
	size_t s;
	__atomic_load(&m_size, &s, __ATOMIC_RELAXED);
	return s;
}

void TTY::set_size(size_t s) { __atomic_store(&m_size, &s, __ATOMIC_RELAXED); }

void TTY::block_if_required(size_t) {}

bool TTY::check_blocked() {
	spin.acquire();
	bool b = (!size());
	spin.release();
	return b;
}

bool TTY::check_blocked_write(size_t sz) {
	spin.acquire();
	bool b = (m_size + sz) >= TTY_BUFFER_SZ;
	spin.release();
	return b;
}

int TTY::write(void *buffer, size_t sz) {
	spin.acquire();
	assert((m_size + sz) < TTY_BUFFER_SZ);
	memcpy((uint8_t *)m_buffer + m_size, buffer, sz);
	m_size += sz;
	spin.release();
	return sz;
}

int TTY::read(void *buffer, size_t sz) {
	if (!sz)
		return 0;
	if (!m_size)
		return 0;
	spin.acquire();
	// assert((m_size+sz) < 192);
	memcpy(buffer, (uint8_t *)m_buffer, sz);
	memmove(m_buffer, m_buffer + sz, m_size);
	m_size -= sz;
	spin.release();
	return sz;
}
