#include <kernel/util/Spinlock.h>
#include <kernel/vfs/block/ata.h>
#include <kernel/vfs/fs/ext2.h>
#include <kernel/vfs/vfs.h>

static VFS *s_vfs;

VFSNode::VFSNode() : m_name("") {}

VFSNode::VFSNode(const char *name) : m_name(name) {}

VFSNode::~VFSNode() {}

// FIXME: a deep enough filesystem will cause a stack overflow.
VFSNode *VFSNode::traverse(Vec<const char *> &path, size_t path_index) {
	auto nodes = m_nodes;

	if (path_index >= path.size())
		return nullptr;

	for (size_t i = 0; i < nodes.size(); i++) {
		if (!strcmp(nodes[i]->m_name, path[path_index])) {
			if (path_index == path.size() - 1)
				return nodes[i];
			VFSNode *node = nodes[i]->traverse(path, path_index + 1);
			if (node)
				return node;
		}
	}

	if (m_mounted_filesystem) {
		return m_mounted_filesystem->traverse(path, 0);
	}

	return nullptr;
}

ZeroNode::ZeroNode(const char *name) : VFSNode(name) {}

FileHandle::FileHandle(VFSNode *node) : m_node(node) {}

int FileHandle::read(void *buffer, size_t size) {
	assert(m_node);
	// FIXME: This is a race condition waiting to happen!
	// m_node->seek(m_position);
	return m_node->read(buffer, size);
}

int FileHandle::write(void *buffer, size_t size) {
	assert(m_node);
	return m_node->write(buffer, size);
}

int FileHandle::seek(int64_t offset, SeekMode origin) {
	assert(m_node);
	switch (origin) {
	case SeekMode::SEEK_SET:
		m_position = (offset > 0) ? offset : 0;
		break;
	case SeekMode::SEEK_CUR:
		m_position += offset;
		break;
	case SeekMode::SEEK_END:
		m_position = m_node->size();
		break;
	default:
		assert(false);
		break;
	}
	return 0;
}

size_t FileHandle::tell() {
	assert(m_node);
	return m_position;
}

VFS::VFS() {
	m_root_vfs_node = new VFSNode("");
	m_dev_fs = new VFSNode("dev");
	m_root_vfs_node->m_nodes.push(m_dev_fs);
	m_root_vfs_node->m_nodes[0]->m_nodes.push(new ZeroNode("zero"));
	ATAManager::setup(m_dev_fs);
	Vec<const char *> path;
	path.push("hd0");
	auto fs = new Ext2FileSystem(m_dev_fs->m_nodes[1]); // traverse(path));
	fs->init();
	m_root_vfs_node->m_mounted_filesystem = fs;
	print_fs(m_root_vfs_node);
}

VFS &VFS::the() { return *s_vfs; }

void VFS::init() { s_vfs = new VFS(); }

VFS::~VFS() {}

Vec<const char *> VFS::parse_path(const char *path) {
	Vec<const char *> path_vec;
	size_t accum_idx = 0;
	// FIXME: we really need a string class.
	char *accum = new char[256];

	for (size_t i = 0; i < strlen(path); i++) {
		if (path[i] == '/') {
			if (accum_idx > 0) {
				accum[accum_idx] = '\0';
				path_vec.push(accum);
				accum = new char[256];
			}
			accum_idx = 0;
			continue;
		}
		accum[accum_idx++] = path[i];
		if (accum_idx >= 256)
			goto end;
	}

	if (accum_idx > 0) {
		accum[accum_idx] = '\0';
		path_vec.push(accum);
	}

end:
	return path_vec;
}

void VFS::print_fs(VFSNode *fs, int depth) {
	for (int i = 0; i < depth; i++) {
		printk(" ");
	}
	printk("-%s\n", fs->m_name);
	for (size_t i = 0; i < fs->m_nodes.size(); i++) {
		print_fs(fs->m_nodes[i], depth + 1);
	}
	if (fs->m_mounted_filesystem) {
		for (size_t i = 0; i < fs->m_mounted_filesystem->m_nodes.size(); i++) {
			print_fs(fs->m_mounted_filesystem->m_nodes[i], depth + 1);
		}
	}
}

VFSNode *VFS::open(Vec<const char *> &name) {
	VFSNode *node = m_root_vfs_node->traverse(name);
	if (!node)
		return nullptr;
	node->init(); 
	return node;
}

FileHandle *VFS::open_fh(Vec<const char *> &name) {
	VFSNode *node = open(name);
	if (!node)
		return nullptr;
	return new FileHandle(node);
}
