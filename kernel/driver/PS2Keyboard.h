#pragma once
#include <kernel/vfs/vfs.h>

class PS2Keyboard : public VFSNode {
  public:
	PS2Keyboard();
	~PS2Keyboard();
	static void init();
	virtual bool check_blocked() override;
	virtual int read(void *buffer, size_t size) override;

	static char *s_buffer;
	static size_t s_buffer_pos;
	static size_t s_writer_pos;

	static constexpr uint16_t KEYBD_BUFFER_SZ = 4096;
	static bool s_initialized;
};
