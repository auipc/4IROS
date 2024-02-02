#pragma once
#include <kernel/util/Vec.h>
#include <kernel/util/Singleton.h>
#include <kernel/string.h>


class VFSNode {
public:
	VFSNode();
	VFSNode(const char* m_name);
	~VFSNode();
	virtual inline bool is_directory() {
		return true;
	}

	virtual VFSNode* traverse(Vec<const char*> path, size_t path_index=0);

	void set_name(const char* name) {
		m_name = name;
	}

	virtual int read(char* buffer, size_t size) {
		(void)buffer;
		(void)size;
		return 1; 
	}
protected:
	friend class VFS;
	Vec<VFSNode*> m_nodes;
	const char* m_name;
};

class NullNode : public VFSNode {
public:
	NullNode(const char* name);

	virtual inline bool is_directory() override {
		return false;
	}

	virtual int read(char* buffer, size_t size) {
		memset(buffer, 0, size);
		return 0;
	}
};

class VFS {
SINGLETON(VFS)
public:
	VFS();
	~VFS();
	VFSNode* open(Vec<const char*> name);
private:
	VFSNode* m_root_vfs_node;
};
