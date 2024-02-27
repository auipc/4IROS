#include <kernel/vfs/fs/ext2.h>
#include <kernel/util/ism.h>

uint32_t block_size = 1024;

Ext2Entry::Ext2Entry(Ext2FileSystem* fs, INode*&& inode) 
	: m_fs(fs)
	, m_inode(inode)
{
}

Ext2Entry::~Ext2Entry() 
{
	delete m_inode;
}

int Ext2Entry::read(void* buffer, size_t size) {
	m_fs->read_from_inode(*m_inode, buffer, size);
	return 0;
}

Ext2FileSystem::Ext2FileSystem(VFSNode* block_dev)
	: VFSNode(block_dev->get_name()), m_block_dev(block_dev)
{
}

Ext2FileSystem::~Ext2FileSystem() {
	delete m_root_inode;
}

INode* Ext2FileSystem::read_inode(size_t index) {
	uint32_t block_group_no = (index - 1) / block.inodes_per_group;
	uint32_t block_group_inode_idx = (index - 1) % block.inodes_per_group;

	m_block_dev->seek(2*block_size);
	m_block_dev->seek_cur((block_group_no) * block_size);
	BlockGroupDescriptor bgd;
	m_block_dev->read((char*)&bgd, sizeof(BlockGroupDescriptor));

	m_block_dev->seek((bgd.start_block_inode) * block_size);
	m_block_dev->seek_cur(block_group_inode_idx * 128);
	INode* node = new INode();
	m_block_dev->read((char*)node, sizeof(INode));
	//assert(node->type == 0x41ed);
	return node;
}

void Ext2FileSystem::seek_block(size_t block_addr) {
	m_block_dev->seek(block_addr*block_size);
}

uint32_t Ext2FileSystem::read_from_inode(INode& inode, void* out, size_t size) {
	size_t size_to_read = size;
	if (size_to_read > inode.size_low) {
		//size_to_read = inode.size_low;
	}


	for (size_t i = 0; i < size_to_read; i+=block_size) {
		seek_block(inode.dbp[i]);
		m_block_dev->read(((char*)out)+i, size_to_read % block_size);
	}
	return size_to_read;
}

Vec<Directory> Ext2FileSystem::scan_dir_entries(INode& inode) {
	Vec<Directory> entries;
	seek_block(inode.dbp[0]);
	for (int i = 0; i <= inode.hardlinks_count; i++) {
		DirEntry entry;
		m_block_dev->read((char*)&entry, sizeof(DirEntry));

		char* name = new char[entry.name_length+1];
		m_block_dev->read((char*)name, entry.name_length);
		name[entry.name_length] = '\0';
		//m_block_dev->seek_cur(entry.name_length);

		// Directory entries must be aligned on disk to the 4 byte boundary.
		m_block_dev->seek_cur(4-(m_block_dev->position()%4));
		entries.push(Directory{name, entry});
	}
	return entries;
}

void Ext2FileSystem::init() {
	m_block_dev->seek(1024);
	m_block_dev->read((char*)&block, sizeof(Ext2SuperBlock));
	m_root_inode = read_inode(2);
	block_size = block.log2_block_size > 0 ? ((uint32_t)block.log2_block_size) << 10 : block_size;
}

VFSNode* Ext2FileSystem::traverse_internal(INode* cur_inode, Vec<const char*>& path, size_t path_index) {
	auto nodes = m_nodes;

	if (path_index >= path.size()) return nullptr;
	auto dir_entries = scan_dir_entries(*cur_inode);

	for (size_t i = 0; i < dir_entries.size(); i++) {
		if (!strcmp(dir_entries[i].name, path[path_index])) {
			printk("found\n");
			printk("inode index %d\n", dir_entries[i].entry.inode);
			auto inode = read_inode(dir_entries[i].entry.inode);
			return new Ext2Entry(this, move(inode));
		}
	}

	return nullptr;
}

VFSNode* Ext2FileSystem::traverse(Vec<const char*>& path, size_t path_index) {
	auto nodes = m_nodes;

	if (path_index >= path.size()) return nullptr;

	return traverse_internal(m_root_inode, path, 0);
}
