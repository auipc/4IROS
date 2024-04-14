#pragma once
#include <stddef.h>
#include <stdint.h>

/* A hacky binary tree for memory allocation.
 * Very close to the buddy allocator, except it doesn't allow for byte runs of
 * unknown size. I don't care how buggy it is, I just want to allocate memory in
 * a semi-efficient manner.
 * Not computationally efficient.
 */
class MemBinTree {
  public:
	MemBinTree(size_t begin, size_t mem);
	~MemBinTree() = delete;
	MemBinTree(MemBinTree &) = delete;

	void *search(size_t size);
	void *search_align(size_t size, size_t align);
	void free(void *ptr);
	void block(void *ptr);

	struct MemBinTreeNode {
		size_t ptr;
		size_t size;
		bool occupied;
		MemBinTreeNode *left;
		MemBinTreeNode *right;
		MemBinTreeNode(size_t p, size_t s) : ptr(p), size(s), occupied(false) {}
	};

  private:
	void debug_print(MemBinTreeNode *root, size_t depth = 0);

	MemBinTreeNode *m_root;
	bool is_it_really_free(MemBinTreeNode *node);
	void find_deepest_for_size(size_t size);
	void block_recurse(MemBinTreeNode *node, void *ptr);
	void free_recurse(MemBinTreeNode *node, void *ptr);
	MemBinTreeNode *search_recurse(MemBinTreeNode *node, size_t size,
								   size_t align = 1);
	void populate(MemBinTreeNode *node, size_t size);

	// The smallest possible container
	static constexpr size_t s_granularity = 4096;
};
