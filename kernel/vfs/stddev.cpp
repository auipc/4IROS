#include <kernel/mem/Paging.h>
#include <kernel/vfs/stddev.h>

StdDev::StdDev(bool echo) : VFSNode(), m_echo(echo) {
	m_buffer = new char[m_buffer_sz];
	memset(m_buffer, 0, sizeof(char) * m_buffer_sz);
}

// FIXME: Blocking is such a nightmare.
bool StdDev::check_blocked() { return (m_bytes_written <= m_position); }

void StdDev::block_if_required(size_t) {}

int StdDev::read(void *buffer, size_t size) {
	for (size_t idx = 0; idx < size; idx++) {
		((char *)buffer)[idx] = m_buffer[(m_position + idx) % m_buffer_sz];
	}
	m_position += size;
	return size;
}

int StdDev::write(void *buffer, size_t size) {
	// Resize buffer
	if (!size)
		return 0;

	for (size_t idx = 0; idx < size; idx++) {
		m_buffer[(m_bytes_written + idx) % m_buffer_sz] = ((char *)buffer)[idx];
	}

	if (m_echo) {
		for (size_t i = 0; i < size; i++) {
			printk("%c", ((char *)buffer)[i]);
		}
	}

	m_bytes_written += size;
	return size;
}
