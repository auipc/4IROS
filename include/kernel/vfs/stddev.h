#pragma once
#include <kernel/vfs/vfs.h>

class StdDev : public VFSNode {
  public:
	StdDev(bool echo = false);
	virtual inline bool is_directory() override { return false; }

	virtual int read(void *buffer, size_t size) override;
	virtual int write(void *buffer, size_t size) override;
	virtual bool check_blocked() override;
	virtual void block_if_required(size_t size) override;

  private:
	size_t m_bytes_written = 0;
	char *m_buffer;
	// FIXME
	size_t m_buffer_sz = 4096*80;
	bool m_blocked = false;
	bool m_echo;
};
