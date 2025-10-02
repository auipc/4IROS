#pragma once
#include <kernel/vfs/vfs.h>

class TTY final : public VFSNode {
  public:
	TTY();
	virtual inline bool is_directory() override { return false; }
	virtual int write(void *buffer, size_t size) override;
	virtual int read(void *buffer, size_t size) override;
	virtual bool check_blocked() override;
	virtual bool check_blocked_write(size_t sz) override;
	virtual void block_if_required(size_t size) override;

  private:
	size_t size() override;
	void set_size(size_t size) override;

	char *m_buffer = nullptr;
	size_t m_size = 0;
};
