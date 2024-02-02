#include <kernel/vfs/vfs.h>
#include <kernel/util/Spinlock.h>

VFSNode::VFSNode() {}

VFSNode::VFSNode(const char* name) 
	: m_name(name)
{
}

VFSNode::~VFSNode() {
}


VFSNode* VFSNode::traverse(Vec<const char*> path, size_t path_index) {
	for (size_t i = 0; i < m_nodes.size(); i++) {
		printk("%s\n", m_nodes[i]->m_name);
		if (!strcmp(m_nodes[i]->m_name, path[path_index])) {
			if (path_index == path.size()-1) return m_nodes[i];
			VFSNode* node = m_nodes[i]->traverse(path, path_index+1);
			if (node) return node;
		}
	}
	return nullptr;
	/*
	size_t path_index = 0;
	VFSNode* current_node = this;
	VFSNode* last_current_node = this;
	while (true) {
		last_current_node = current_node;
		for (size_t i = 0; i < m_nodes.size(); i++) {
			if (m_nodes[i]->m_name == path[path_index]) {
				current_node = m_nodes[i];
				path_index++;
			}
		}

		if (last_current_node == current_node) return nullptr;
		if (path_index == m_nodes.size()-1) {
			return current_node;
		}
	}*/
}

NullNode::NullNode(const char* name)
	: VFSNode(name)
{
}

VFS::VFS() {
	m_root_vfs_node = new VFSNode();
	m_root_vfs_node->m_nodes.push(new VFSNode("dev"));
	m_root_vfs_node->m_nodes[0]->m_nodes.push(new NullNode("null"));
}

VFS::~VFS() {
}


VFSNode* VFS::open(Vec<const char*> name) {
	VFSNode* node = m_root_vfs_node->traverse(name);
	if (!node) return nullptr;
	return node;
}
