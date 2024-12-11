#include <kernel/mem/Paging.h>
#include <kernel/vfs/stddev.h>

StdDev::StdDev(bool echo) : VFSNode(), m_echo(echo) {
	m_buffer = new char[m_buffer_sz];
	memset(m_buffer, 0, sizeof(char)*m_buffer_sz);
}

// FIXME: Blocking is such a nightmare.
bool StdDev::check_blocked() { return (m_bytes_written <= m_position); }
void StdDev::block_if_required(size_t) {
	if (!(m_bytes_written - m_position)) {
		m_blocked = true;
	}
}

int StdDev::read(void *buffer, size_t size) {
	size_t size_avail = m_bytes_written - m_position;
	if (!size_avail) {
		return -1;
	}

	size_t size_read = (size_avail > size) ? size : size_avail;
	memcpy(buffer, m_buffer + m_position, size_read);
	m_position += size_read;
	return size_read;
}

int StdDev::write(void *buffer, size_t size) {
	// Resize buffer
	if ((m_position + size) >= m_buffer_sz) {
		char *tmp = new char[m_buffer_sz + 4096];
		memcpy(tmp, m_buffer, m_buffer_sz);
		m_buffer_sz += 4096;
		delete[] m_buffer;
		m_buffer = tmp;
	}

	memcpy(m_buffer + m_bytes_written, buffer, size);

	if (m_echo) {
		for (size_t i = 0; i < size; i++) {
			printk("%c", ((char *)buffer)[i]);
		}
	}

	m_bytes_written += size;
	if ((m_bytes_written - m_position) > 0) {
		m_blocked = false;
	}

	return size;
}
