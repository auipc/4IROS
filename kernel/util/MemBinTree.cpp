#include <kernel/printk.h>
#include <kernel/util/MemBinTree.h>

// FIXME: stack overflow here but idc
void MemBinTree::debug_print(MemBinTreeNode *root, size_t depth) {
	if (!root)
		return;

	for (size_t i = 0; i < depth; i++)
		printk("  ");

	printk("%x %x %d\n", root->ptr, root->size, root->occupied);
	if (root->occupied)
		return;
	debug_print(root->left, depth + 1);
	debug_print(root->right, depth + 1);
}

// FIXME: Another stack overflow bug, somewhere we need to add a queue system
// for trees.
void MemBinTree::populate(MemBinTreeNode *node, size_t size) {
	if (s_granularity > size) {
		return;
	}

	node->left = new MemBinTreeNode(node->ptr, size / 2);
	node->right = new MemBinTreeNode(node->ptr + (size / 2), size / 2);
	populate(node->left, size / 2);
	populate(node->right, size / 2);
}

bool MemBinTree::is_it_really_free(MemBinTreeNode *node) {
	if (!node)
		return true;

	if (!node->occupied) {
		bool l = is_it_really_free(node->left);
		bool r = is_it_really_free(node->right);
		return l || r;
	} else {
		return false;
	}
}

MemBinTree::MemBinTreeNode *
MemBinTree::search_recurse(MemBinTreeNode *node, size_t size, size_t align) {
	if (!node) {
		return nullptr;
	}
	if (node->occupied) {
		return nullptr;
	}
	if (!is_it_really_free(node)) {
		return nullptr;
	}

	// The left side has the greater quantity so we only have to check that
	bool jelly = node->left ? (node->left->size < size) : true;

	if (node->size >= size && jelly && !(node->ptr % align)) {
		return node;
	}

	MemBinTreeNode *l = search_recurse(node->left, size, align);
	MemBinTreeNode *r = search_recurse(node->right, size, align);

	return l ? l : r;
}

void MemBinTree::free_recurse(MemBinTreeNode *node, void *ptr) {
	if (!node)
		return;

	if (node->ptr == (size_t)ptr && node->occupied) {
		node->occupied = false;
		free_recurse(node->left, ptr);
		return;
	}

	free_recurse(node->left, ptr);
	free_recurse(node->right, ptr);
}

void MemBinTree::free(void *ptr) { return free_recurse(m_root, ptr); }

void *MemBinTree::search(size_t size) {
	auto node = search_recurse(m_root, size);
	if (!node)
		return nullptr;
	node->occupied = true;
	return (void *)node->ptr;
}

void MemBinTree::block(void *ptr) { block_recurse(m_root, ptr); }

void MemBinTree::block_recurse(MemBinTreeNode *node, void *ptr) {
	if (!node)
		return;
	if (node->ptr == (size_t)ptr && !node->left) {
		node->occupied = true;
	}

	block_recurse(node->left, ptr);
	block_recurse(node->right, ptr);
}

void *MemBinTree::search_align(size_t size, size_t align) {
	auto node = search_recurse(m_root, size, align);
	if (!node)
		return nullptr;

	printk("node:%x\n", node->ptr);
	node->occupied = true;
	return (void *)node->ptr;
}

MemBinTree::MemBinTree(size_t begin, size_t mem) {
	m_root = new MemBinTreeNode(begin, mem);
	m_root->left = new MemBinTreeNode(begin, mem / 2);
	m_root->right = new MemBinTreeNode(begin + (mem / 2), mem - (mem / 2));
	populate(m_root->left, mem / 2);
	populate(m_root->right, mem - (mem / 2));
}
