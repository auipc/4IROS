#pragma once
#include <kernel/util/Vec.h>
#include <kernel/util/Singleton.h>
#include <kernel/string.h>
#include <kernel/limits.h>


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

	const char* get_name() const {
		return m_name;
	}

	virtual int open(Vec<const char*> path) {
		(void)path;
		return -1; 
	}

	virtual int read(void* buffer, size_t size) {
		(void)buffer;
		(void)size;
		return -1; 
	}

	virtual int seek(size_t position) {
		m_position = position;
		return 0;
	}

	virtual int seek_cur(size_t position) {
		m_position += position;
		return 0;
	}

	virtual size_t position() {
		return m_position;
	}

	virtual void push(VFSNode* node) {
		m_nodes.push(node);
	}

	// FIXME: File size should be able to exceed integer width.
	virtual size_t size() {
		return 0;
	}
protected:
	friend class VFS;
	Vec<VFSNode*> m_nodes;
	const char* m_name;
	// FIXME: On second thought, it's probably not good to have a global position that every reader uses.
	size_t m_position;
	VFSNode* m_mounted_filesystem;
};

enum SeekMode {
	SEEK_SET = 0,
	SEEK_CUR = 1,
	SEEK_END = 2
};

class FileHandle {
public:
	FileHandle(VFSNode* node);
	int read(void* buffer, size_t size);
	int seek(size_t offset,	SeekMode origin);
	size_t tell();
private:
	size_t m_position;
	VFSNode* m_node;
};

class NullNode : public VFSNode {
public:
	NullNode(const char* name);

	virtual inline bool is_directory() override {
		return false;
	}

	virtual int read(void* buffer, size_t size) {
		memset((char*)buffer, 0, size);
		return 0;
	}
};

class VFS {
SINGLETON(VFS)
public:
	VFS();
	~VFS();
	VFSNode* open(Vec<const char*> name);
	FileHandle* open_fh(Vec<const char*> name);

	void print_fs(VFSNode* root, int depth = 0);

	inline VFSNode* get_root_fs() { return m_root_vfs_node; }
	inline VFSNode* get_dev_fs() { return m_dev_fs; }
private:
	VFSNode* m_root_vfs_node = nullptr;
	VFSNode* m_dev_fs = nullptr;
};
