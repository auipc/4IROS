#include <kernel/minmax.h>
#include <kernel/util/ism.h>
#include <kernel/vfs/fs/ext2.h>
#include <kernel/Syscall.h>

uint32_t block_size = 1024;
uint32_t *s_singly_blocks;

Ext2Entry::Ext2Entry(Ext2FileSystem *fs, const char *name, INode *&&inode, size_t inode_no)
	: VFSNode(name), m_fs(fs), m_inode(inode), m_inode_no(inode_no) {}

Ext2Entry::~Ext2Entry() { delete m_inode; }

int Ext2Entry::read(void *buffer, size_t size) {
	int ret = m_fs->read_from_inode(*m_inode, buffer, size, m_position);
	if (ret < 0) return ret;
	m_position += size;
	return ret;
}

bool Ext2Entry::check_blocked() {
	return __atomic_load_n(&write_lock, __ATOMIC_RELAXED);
}

bool Ext2Entry::check_blocked_write(size_t) {
	// FIXME change memorder
	return __atomic_load_n(&write_lock, __ATOMIC_RELAXED);
}

int Ext2Entry::write(void *buffer, size_t size) {
	(void)buffer;
	// FIXME multiple cores will make the locking process more headache inducing
	__atomic_store_n(&write_lock, 1, __ATOMIC_RELAXED);
	size_t ret = m_fs->write_to_inode(*m_inode, m_inode_no, buffer, size, m_position);
	m_position += ret;
	__atomic_store_n(&write_lock, 0, __ATOMIC_RELAXED);
	return ret;
}

void Ext2Entry::set_size(size_t sz) {
	m_inode->size_low = sz;
	m_fs->write_inode(m_inode, m_inode_no);
}

Ext2FileSystem::Ext2FileSystem(VFSNode *block_dev)
	: VFSNode(block_dev->name()), m_block_dev(block_dev) {
	s_singly_blocks = new uint32_t[256];
}

Ext2FileSystem::~Ext2FileSystem() { delete m_root_inode; }

// FIXME an allocated INode should be passed to the function, not allocated 
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

BlockGroupDescriptor Ext2FileSystem::bgd_from_inode(size_t index) {
	uint32_t block_group_no = (index - 1) / block.inodes_per_group;
	m_block_dev->seek(2 * block_size);
	m_block_dev->seek_cur((block_group_no)*block_size);
	BlockGroupDescriptor bgd;
	m_block_dev->read((char *)&bgd, sizeof(BlockGroupDescriptor));
	return bgd;
}

void Ext2FileSystem::write_inode(INode* node, size_t index) {
	uint32_t block_group_no = (index - 1) / block.inodes_per_group;
	uint32_t block_group_inode_idx = (index - 1) % block.inodes_per_group;

	m_block_dev->seek(2 * block_size);
	m_block_dev->seek_cur((block_group_no)*block_size);
	BlockGroupDescriptor bgd;
	m_block_dev->read((char *)&bgd, sizeof(BlockGroupDescriptor));

	m_block_dev->seek((bgd.start_block_inode) * block_size);
	m_block_dev->seek_cur(block_group_inode_idx * 128);
	m_block_dev->write((char *)node, sizeof(INode));
}

// FIXME subtract block from BGD
int Ext2FileSystem::scan_block_bitmap(const BlockGroupDescriptor& bgd) { 
	int block_index = 0;
	uint8_t* block_bitmap = new uint8_t[block.blocks_per_group/8];
	seek_block(bgd.block_address_bm);
	m_block_dev->read(block_bitmap, block.blocks_per_group/8);
	for (uint32_t i = 0; i < block.blocks_per_group/8; i++) {
		if (block_bitmap[i] == UCHAR_MAX) continue;
		for (uint32_t j = 0; j < 8; j++) {
			if (!(block_bitmap[i] & (1<<j))) {
				block_index = ((i*8)+j);
				goto done;
			}
		}
	}

	return 0;
done:

	block_bitmap[(block_index-1)/8] |= (1<<((block_index-1)%8));
	seek_block(bgd.block_address_bm);
	m_block_dev->write(block_bitmap, block.blocks_per_group/8);
	delete[] block_bitmap;
	return block_index;
	return 0;
}

int Ext2FileSystem::scan_inode_bitmap(const BlockGroupDescriptor& bgd) { 
	int inode_index = 0;
	uint8_t* inode_bitmap = new uint8_t[block.inodes_per_group/8];
	printk("bgd.block_address_inode_bm %x\n", bgd.block_address_inode_bm);
	seek_block(bgd.block_address_inode_bm);
	m_block_dev->read(inode_bitmap, block.inodes_per_group/8);
	for (uint32_t i = 0; i < block.inodes_per_group/8; i++) {
		if (inode_bitmap[i] == UCHAR_MAX) continue;
		for (uint32_t j = 0; j < 8; j++) {
			if (!(inode_bitmap[i] & (1<<j))) {
				inode_index = 1+((i*8)+j);
				goto done;
			}
		}
	}

	return 0;
done:

	inode_bitmap[(inode_index-1)/8] |= (1<<((inode_index-1)%8));
	seek_block(bgd.block_address_inode_bm);
	m_block_dev->write(inode_bitmap, block.inodes_per_group/8);
	delete[] inode_bitmap;
	return inode_index;
}

VFSNode *Ext2FileSystem::create(const char* name) {
	auto path = VFS::the().parse_path(name);
	if (traverse(path)) return nullptr;

	int bgd_index = 0;
	BlockGroupDescriptor bgd;
	while (1) {
		m_block_dev->seek(2 * block_size);
		m_block_dev->seek_cur(bgd_index*block_size);
		m_block_dev->read((char *)&bgd, sizeof(BlockGroupDescriptor));
		if (bgd.unallocated_inodes_group) break;
		bgd_index++;
	}

	// FIXME find BGD with unallocated blocks, not unallocted inodes
	int inode_index = scan_inode_bitmap(bgd);
	if (!inode_index) panic("Couldn't find free inode");

	// Insert directory entry
	auto& inode = *read_inode(2);
	DirEntry entry;
	size_t entry_pos = 0;
	seek_block(inode.dbp[0]);
	while (block_size > (m_block_dev->position() - inode.dbp[0] * block_size)) {
		entry_pos = m_block_dev->position();
		m_block_dev->read((char *)&entry, sizeof(DirEntry));
		if (!entry.name_length)
			continue;

		m_block_dev->seek_cur(entry.name_length);
		m_block_dev->seek_cur(entry.size - sizeof(DirEntry) -
							  entry.name_length);
	}

	// Rewrite last dir entry's size
	entry.size = sizeof(DirEntry)+entry.name_length;
	entry.size += 4-(entry.size%4);
	m_block_dev->seek(entry_pos);
	m_block_dev->write(&entry, sizeof(DirEntry));

	printk("inode_index %x\n", inode_index);
	entry_pos += sizeof(DirEntry)+entry.name_length;
	entry_pos += 4-(entry_pos%4);
	// FIXME bounds check
	//assert(entry.inode == 0 && entry.name_length == 0);
	m_block_dev->seek(entry_pos);

	// Write dir entry
	entry.type = 1;
	entry.inode = inode_index;
	entry.name_length = strlen(name);

	entry.size = block_size-(m_block_dev->position() - inode.dbp[0] * block_size);
	// Zero dir entry
	uint8_t lmao = 0;
	for (int i = 0; i < entry.size; i++)
		m_block_dev->write(&lmao, 1);
	
	m_block_dev->seek(entry_pos);
	m_block_dev->write(&entry, sizeof(DirEntry));
	m_block_dev->write((char*)name, entry.name_length);

	// Create Inode
	m_block_dev->seek((bgd.start_block_inode) * block_size);
	m_block_dev->seek_cur((inode_index-1) * 128);
	INode *node = new INode();
	node->type = 0x8000;
	node->access_time = node->creation_time = node->modification_time = Syscall::timeofday();
	m_block_dev->write((char *)node, sizeof(INode));

	// Subtract inode from total count
	bgd.unallocated_inodes_group--;
	m_block_dev->seek(2 * block_size);
	m_block_dev->seek_cur(bgd_index*block_size);
	m_block_dev->write((char *)&bgd, sizeof(BlockGroupDescriptor));

	auto entry_node = new Ext2Entry(this, name, move(node), inode_index);
	push(entry_node);
	return entry_node;
}

void Ext2FileSystem::seek_block(size_t block_addr) {
	m_block_dev->seek(block_addr * block_size);
}

void Ext2FileSystem::read_singly(uint32_t *singly_blocks, INode &inode,
								 size_t singly_position, void *out,
								 size_t block_idx, size_t size_to_read,
								 size_t position) {
	if (!inode.sibp)
		panic("No sibp\n");
	/*if (!singly_blocks) {
		singly_blocks = new uint32_t[256];*/
	seek_block(singly_position);
	m_block_dev->read((char *)singly_blocks, sizeof(uint32_t) * 256);
	//}

	m_block_dev->seek((singly_blocks[block_idx] * block_size) +
					  (position % block_size));
	m_block_dev->read((char *)out, min(size_to_read, (size_t)block_size));
}

void Ext2FileSystem::read_indirect_singly(INode &inode, size_t singly_position,
										  void *out, size_t block_idx,
										  size_t size_to_read,
										  size_t position) {
	read_singly(s_singly_blocks, inode, singly_position, out, block_idx,
				size_to_read, position);
}

uint32_t Ext2FileSystem::write_to_inode(INode &inode, size_t inode_no, void *out, size_t size,
						 size_t position) {
	(void)inode_no;
	(void)out;
	size_t actual_size = size;
	size_t size_to_read =
		actual_size + (block_size - (actual_size % block_size));
	size_t block_idx = position / block_size;
	assert(size < 1024);
	inode.size_low += size&UINT_MAX;
	while (size_to_read) {
		switch (block_idx) {
		case 0 ... 11: {
			if (!inode.dbp[block_idx]) {
				inode.dbp[block_idx] = scan_block_bitmap(bgd_from_inode(inode_no));
				write_inode(&inode, inode_no);
				m_block_dev->seek((inode.dbp[block_idx] * block_size)+(position%block_size));
				char c = 0;
				for (size_t i = 0; i < block_size; i++)
					m_block_dev->write(&c, 1);
			}
			seek_block(inode.dbp[block_idx]);
			m_block_dev->seek((inode.dbp[block_idx] * block_size)+(position%block_size));
			m_block_dev->write(out, size);
		} break;
		default:
			printk("%d\n", block_idx);
			panic("Help\n");
			break;
		}
		size_to_read -= min(size_to_read, (size_t)block_size);
	}
	write_inode(&inode, inode_no);
	return actual_size;
}

int Ext2FileSystem::read_from_inode(INode &inode, void *out, size_t size,
										 size_t position) {
	if (position >= inode.size_low) return -1;
	size_t actual_size = min(size, (size_t)(inode.size_low-position));
	size_t size_to_read =
		actual_size + (block_size - (actual_size % block_size)); // actual_size;
	size_t block_idx = position / block_size;
	size_t output_ptr = 0;
	// FIXME Temporary fix for a bug involving directly copying to the output
	// buffer. Something goes wrong when the position isn't aligned by 1024
	// bytes.
	char *temp_buf =
		new char[size_to_read + (block_size - (size_to_read % block_size))];

	uint32_t *doubly_blocks = nullptr;
	while (size_to_read) {
		switch (block_idx) {
		case 0 ... 11: {
			seek_block(inode.dbp[block_idx]);
			m_block_dev->seek((inode.dbp[block_idx] * block_size));
			m_block_dev->read((char *)temp_buf + output_ptr, block_size);
		} break;
		// https://www.nongnu.org/ext2-doc/ext2.html
		// With a 1KiB block size, blocks 13 to 268 of the file data are
		// contained in this indirect block.
		case 12 ... 267: {
			read_singly(s_singly_blocks, inode, inode.sibp,
						(void *)((uintptr_t)temp_buf + output_ptr),
						block_idx - 12, size_to_read, 0);
		} break;
		case 268 ... 65803: {
			if (!inode.dibp)
				panic("No dibp\n");
			if (!doubly_blocks) {
				doubly_blocks = new uint32_t[256];
				seek_block(inode.dibp);
				m_block_dev->read((char *)doubly_blocks,
								  sizeof(uint32_t) * 256);
			}
			// printk("dbl %x\n", doubly_blocks[(block_idx-268)/256]);
			read_indirect_singly(inode, doubly_blocks[(block_idx - 268) / 256],
								 (void *)((uintptr_t)temp_buf + output_ptr),
								 (block_idx - 268) % 256, size_to_read, 0);
		} break;
		default:
			printk("%d\n", block_idx);
			panic("Help\n");
			break;
		}
		output_ptr += block_size; // - (position % block_size);
		// position += min(size_to_read, (size_t)block_size);
		size_to_read -= min(size_to_read, (size_t)block_size);
		block_idx++;
	}

	memcpy(out, ((char *)temp_buf) + (position % block_size), actual_size);

#if 0
	while (size_to_read) {
		switch (block_idx) {
		case 0 ... 11: {
			seek_block(inode.dbp[block_idx]);
			m_block_dev->seek((inode.dbp[block_idx] * block_size) +
							  (position % block_size));
			m_block_dev->read((char *)out + output_ptr,
							  min(size_to_read, (size_t)block_size));
		} break;
		// https://www.nongnu.org/ext2-doc/ext2.html
		// With a 1KiB block size, blocks 13 to 268 of the file data are
		// contained in this indirect block.
		case 12 ... 267: {
			read_singly(inode, inode.sibp,
						(void *)((uintptr_t)out + output_ptr), block_idx - 12,
						size_to_read, position);
		} break;
#if 0
		case 268 ... 65803: {
			if (!inode.dibp)
				panic("No dibp\n");
			if (!doubly_blocks) {
				doubly_blocks = new uint32_t[256];
				seek_block(inode.dibp);
				m_block_dev->read((char *)doubly_blocks,
								  sizeof(uint32_t) * 256);
			}
			// printk("dbl %x\n", doubly_blocks[(block_idx-268)/256]);
			read_singly(inode, doubly_blocks[(block_idx - 268) / 256],
						(void *)((uintptr_t)out + output_ptr),
						(block_idx - 268) % 256, size_to_read, position);
		} break;
#endif
		default:
			panic("Unsupported block index!\n");
			break;
		}
		output_ptr += block_size - (position % block_size);
		position = 0;
		// position += min(size_to_read, (size_t)block_size);
		size_to_read -= min(size_to_read, (size_t)block_size);
		block_idx++;
	}
#endif

	if (doubly_blocks)
		delete[] doubly_blocks;
	delete[] temp_buf;
	return actual_size;
}

Vec<Directory> Ext2FileSystem::scan_dir_entries(INode &inode) {
	Vec<Directory> entries;
	for (int i = 0; i < 12; i++) {
		if (!inode.dbp[i]) break;
		seek_block(inode.dbp[i]);

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
			assert(entries[entries.size() - 1].name == name);
		}
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
	if (block_size != 1024) {
		panic("Unsupported ext2 block size!");
	}

	printk("MAGIC %x\n", block.ext2_sig);

	auto dir_entries = scan_dir_entries(*m_root_inode);

	// Ext2 directories always contain at least 2 directories (. and ..).
	// I'd rather have the VFS layer handle these, so we skip over them.
	for (size_t i = 2; i < dir_entries.size(); i++) {
		auto inode = read_inode(dir_entries[i].entry.inode);
		push(new Ext2Entry(this, dir_entries[i].name, move(inode), dir_entries[i].entry.inode));
	}
}

VFSNode *Ext2FileSystem::traverse_internal(INode *cur_inode,
										   Vec<const char *> &path,
										   size_t path_index) {
	auto nodes = m_nodes;

	if (path_index >= path.size())
		return nullptr;

	for (size_t i = 0; i < nodes.size(); i++) {
		auto node = nodes[i];
		if (!strcmp(node->name(), path[path_index])) {
			if (node->is_directory()) {
				return traverse_internal(static_cast<Ext2Entry*>(node)->m_inode, path, path_index + 1);
			}
			return node;
		}
	}

	auto dir_entries = scan_dir_entries(*cur_inode);

	// Ext2 directories always contain at least 2 directories (. and ..).
	// I'd rather have the VFS layer handle these, so we skip over them.
	for (size_t i = 2; i < dir_entries.size(); i++) {
		if (!strcmp(dir_entries[i].name, path[path_index])) {
			auto inode = read_inode(dir_entries[i].entry.inode);
			if (inode->type & EXT2_DIR) {
				return traverse_internal(inode, path, path_index + 1);
			}
			return new Ext2Entry(this, dir_entries[i].name, move(inode), dir_entries[i].entry.inode);
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
