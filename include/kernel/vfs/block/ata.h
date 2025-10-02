#pragma once
#include <kernel/arch/amd64/x86_64.h>
#include <kernel/vfs/vfs.h>
#include <kernel/Debug.h>

class ATAManager {
  public:
	static void setup(VFSNode *parent);
};

enum DriveConfig {
	Master = 0x40,
	MasterSlave = 0x40 | (1 << 4),
	Slave = 0x50,
	SlaveMaster = 0x50 | (1 << 4),
};

class ATABlockNode : public VFSNode {
  public:
	ATABlockNode(const char *name, DriveConfig config);
	virtual int open(Vec<const char *> path) override;
	virtual int read(void *buffer, size_t size) override;
	virtual int write(void *buffer, size_t size) override;
	virtual int seek(size_t position) override {
		m_position = position;
		return 0;
	}

	virtual int seek_cur(size_t position) override {
		m_position += position;
		return 0;
	}

  private:
	int write_512(uint8_t *buffer, size_t under);
	int read_512(uint8_t *buffer, size_t under);
	int read_512_temp(uint8_t *buffer, size_t under);
	int read_block(uint16_t *buffer);
	DriveConfig m_config;
};
