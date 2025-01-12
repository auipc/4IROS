#pragma once

#include <kernel/vfs/vfs.h>

// FIXME get the framebuffer address from PCI bar
class BochsFramebuffer final : public VFSNode {
  public:
	BochsFramebuffer();
	virtual inline bool is_directory() override { return false; }
	virtual void init() override;
	virtual int write(void *buffer, size_t size) override;
	virtual int read(void *buffer, size_t size) override;

  private:
	static const uintptr_t FRAMEBUFFER_ADDR = 0xfd000000;
	static const uintptr_t FRAMEBUFFER_CONFIG_ADDR = 0xfebf8000 + 0x500;
};
