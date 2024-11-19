#pragma once

#include <kernel/vfs/vfs.h>
#include <stdint.h>

#define EXT2_DIR 0x4000

struct [[gnu::packed]] Ext2SuperBlock {
	uint32_t total_inodes;
	uint32_t total_blocks;
	uint32_t superuser_blocks;
	uint32_t empty_blocks;
	uint32_t empty_inodes;
	uint32_t superblock_block;
	uint32_t log2_block_size;
	uint32_t log2_frag_size;
	uint32_t blocks_per_group;
	uint32_t frags_per_group;
	uint32_t inodes_per_group;
	uint32_t mount_time;
	uint32_t write_time;
	uint16_t last_fsck_count;
	uint16_t fsck_threshold;
	uint16_t ext2_sig;
	uint16_t state;
	uint16_t error;
	uint16_t minor;
	uint32_t fsck_time;
	uint32_t fsck_time_threshold;
	uint32_t os_id;
	uint32_t major;
	uint16_t reserved_user_id;
	uint16_t reserved_group_id;
};

static_assert(sizeof(Ext2SuperBlock) == 84);

struct [[gnu::packed]] BlockGroupDescriptor {
	uint32_t block_address_bm;
	uint32_t block_address_in_bm;
	uint32_t start_block_inode;
	uint16_t unallocated_blocks_group;
	uint16_t unallocated_inodes_group;
	uint16_t directories_group;
	char unused[14];
};

struct [[gnu::packed]] INode {
	uint16_t type;
	uint16_t uid;
	uint32_t size_low;
	uint32_t access_time;
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t deletion_time;
	uint16_t group_id;
	uint16_t hardlinks_count;
	uint32_t sector_count;
	uint32_t flags;
	uint32_t osval;
	uint32_t dbp[12];
	uint32_t sibp;
	uint32_t dibp;
	uint32_t tibp;
	uint32_t gen_no;
	uint32_t reserved[2];
	uint32_t frag_block_addr;
	uint32_t reserved_2[3];
};

struct [[gnu::packed]] DirEntry {
	uint32_t inode;
	uint16_t size;
	uint8_t name_length;
	uint8_t type;
};

struct Directory {
	const char *name;
	DirEntry entry;
};

class Ext2FileSystem;

class Ext2Entry : public VFSNode {
  public:
	Ext2Entry(Ext2FileSystem *fs, const char* name, INode *&&inode);
	~Ext2Entry();

	virtual inline bool is_directory() override { return false; }

	virtual int read(void *buffer, size_t size) override;

	virtual size_t size() override { return m_inode->size_low; }

  private:
	Ext2FileSystem *m_fs;
	INode *m_inode;
};

class Ext2FileSystem : public VFSNode {
  public:
	Ext2FileSystem(VFSNode *block_dev);
	~Ext2FileSystem();
	void init();
	virtual VFSNode *traverse(Vec<const char *> &path, size_t path_index = 0);

  private:
	void read_singly(INode &inode, size_t singly_position, void* out, size_t block_idx, size_t size_to_read, size_t position);
	VFSNode *traverse_internal(INode *cur_inode, Vec<const char *> &path,
							   size_t path_index = 0);
	INode *read_inode(size_t index);
	void seek_block(size_t block_addr);
	Vec<Directory> scan_dir_entries(INode &inode);
	uint32_t read_from_inode(INode &inode, void *out, size_t size,
							 size_t position);
	VFSNode *m_block_dev;
	Ext2SuperBlock block;
	INode *m_root_inode;
	friend class Ext2Entry;
};
