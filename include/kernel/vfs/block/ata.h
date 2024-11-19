#pragma once
#include <kernel/vfs/vfs.h>
#include <kernel/arch/amd64/x86_64.h>

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

  private:
	int read_512(uint16_t *buffer);
	DriveConfig m_config;
};
