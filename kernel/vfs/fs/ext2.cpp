#include <kernel/util/ism.h>
#include <kernel/vfs/fs/ext2.h>

template <class T> constexpr T min(const T &a, const T &b) {
	return a < b ? a : b;
}

uint32_t block_size = 1024;

Ext2Entry::Ext2Entry(Ext2FileSystem *fs, INode *&&inode)
	: m_fs(fs), m_inode(inode) {}

Ext2Entry::~Ext2Entry() { delete m_inode; }

int Ext2Entry::read(void *buffer, size_t size) {
	return m_fs->read_from_inode(*m_inode, buffer, size, m_position);
}

Ext2FileSystem::Ext2FileSystem(VFSNode *block_dev)
	: VFSNode(block_dev->get_name()), m_block_dev(block_dev) {}

Ext2FileSystem::~Ext2FileSystem() { delete m_root_inode; }

INode *Ext2FileSystem::read_inode(size_t index) {
	uint32_t block_group_no = (index - 1) / block.inodes_per_group;
	uint32_t block_group_inode_idx = (index - 1) % block.inodes_per_group;

	m_block_dev->seek(2 * block_size);
	m_block_dev->seek_cur((block_group_no)*block_size);
	BlockGroupDescriptor bgd;
	m_block_dev->read((char *)&bgd, sizeof(BlockGroupDescriptor));

	m_block_dev->seek((bgd.start_block_inode) * block_size);
	m_block_dev->seek_cur(block_group_inode_idx * 128);
	INode *node = new INode();
	m_block_dev->read((char *)node, sizeof(INode));
	// assert(node->type == 0x41ed);
	return node;
}

void Ext2FileSystem::seek_block(size_t block_addr) {
	m_block_dev->seek(block_addr * block_size);
}

uint32_t Ext2FileSystem::read_from_inode(INode &inode, void *out, size_t size,
										 size_t position) {
	size_t actual_size = (size > inode.size_low) ? inode.size_low : size;
	size_t size_to_read = actual_size;
	size_t block_idx = position / block_size;
	if (size_to_read > inode.size_low) {
		// size_to_read = inode.size_low;
	}

	uint32_t *singly_blocks = nullptr;
	while (size_to_read) {
		switch (block_idx / 12) {
		case 0: {
			seek_block(inode.dbp[block_idx]);
			m_block_dev->seek((inode.dbp[block_idx] * block_size) +
							  (position % block_size));

			m_block_dev->read((char *)out + (block_idx * block_size),
							  min(size_to_read, (size_t)block_size));
		} break;
		case 1: {
			if (!singly_blocks) {
				singly_blocks = new uint32_t[12];
				seek_block(inode.sibp);
				m_block_dev->read((char *)singly_blocks, sizeof(uint32_t) * 12);
			}

			m_block_dev->seek((singly_blocks[block_idx - 12] * block_size) +
							  (position % block_size));

			m_block_dev->read((char *)out + (block_idx * block_size),
							  min(size_to_read, (size_t)block_size));
		} break;
		default:
			assert(!"Invalid block index");
		}
		size_to_read -= min(size_to_read, (size_t)block_size);
		block_idx++;
	}

	delete[] singly_blocks;
	return actual_size;
}

Vec<Directory> Ext2FileSystem::scan_dir_entries(INode &inode) {
	Vec<Directory> entries;
	seek_block(inode.dbp[0]);

	// A group of directory entries fits into a single block. If size remains
	// when all the directory entries are read, the last entry fills the rest of
	// the block by specifiying the remaining size.
	while (block_size > (m_block_dev->position() - inode.dbp[0] * block_size)) {
		DirEntry entry;
		m_block_dev->read((char *)&entry, sizeof(DirEntry));
		if (!entry.name_length)
			continue;

		char *name = new char[entry.name_length + 1];
		m_block_dev->read((char *)name, entry.name_length);
		name[entry.name_length] = '\0';

		m_block_dev->seek_cur(entry.size - sizeof(DirEntry) -
							  entry.name_length);
		entries.push(Directory{name, entry});
	}
	return entries;
}

void Ext2FileSystem::init() {
	m_block_dev->seek(1024);
	m_block_dev->read((char *)&block, sizeof(Ext2SuperBlock));
	m_root_inode = read_inode(2);
	block_size = block.log2_block_size > 0
					 ? ((uint32_t)block.log2_block_size) << 10
					 : block_size;
}

VFSNode *Ext2FileSystem::traverse_internal(INode *cur_inode,
										   Vec<const char *> &path,
										   size_t path_index) {
	auto nodes = m_nodes;

	if (path_index >= path.size())
		return nullptr;
	auto dir_entries = scan_dir_entries(*cur_inode);

	// Ext2 directories always contain at least 2 directories (. and ..).
	// I'd rather have the VFS layer handle these, so we skip over them.
	for (size_t i = 2; i < dir_entries.size(); i++) {
		if (!strcmp(dir_entries[i].name, path[path_index])) {
			auto inode = read_inode(dir_entries[i].entry.inode);
			return new Ext2Entry(this, move(inode));
		}
	}

	return nullptr;
}

VFSNode *Ext2FileSystem::traverse(Vec<const char *> &path, size_t path_index) {
	auto nodes = m_nodes;

	if (path_index >= path.size())
		return nullptr;

	return traverse_internal(m_root_inode, path, 0);
}
