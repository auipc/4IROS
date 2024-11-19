#pragma once
#include <kernel/vfs/vfs.h>

class VGAFramebuffer final : public VFSNode {
  public:
	virtual void init() override;
	virtual int write(void *buffer, size_t size) override;
  private:
	static void write_misc_port(uint8_t byte);
	static void write_fixed_port(uint8_t index, uint8_t byte);
	static uint8_t read_fixed_port(uint8_t index);
	static void write_index_port(uint16_t port, uint8_t index, uint8_t byte);
	static const uintptr_t FRAMEBUFFER_ADDR = 0xA0000;
	static const uintptr_t VGA_REGISTER_1 = 0x3C4;
	static const uintptr_t VGA_REGISTER_2 = 0x3CE;
	static const uintptr_t VGA_REGISTER_3 = 0x3D4;
};
