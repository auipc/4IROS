#pragma once
#include <kernel/vfs/vfs.h>

class Serial final : public VFSNode {
  public:
	Serial();
	virtual inline bool is_directory() override { return false; }
	virtual void init() override;
	virtual bool check_blocked() override;
	virtual int read(void *buffer, size_t size) override;
	virtual bool check_blocked_write(size_t) override;
	virtual int write(void *buffer, size_t size) override;
};
