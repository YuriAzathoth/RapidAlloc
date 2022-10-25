/*
	RapidAlloc - Fast C allocators collections
	Copyright (C) 2022 Yuriy Zinchenko

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef RAPID_ALLOC_H
#define RAPID_ALLOC_H

#include <stdbool.h>
#include <stdint.h>

/// Free or busy memory block header in memory line
struct a3d_memory_block_header
{
	/// Previous typed block in memory line
	struct a3d_memory_block_header* type_prev;

	/// Next typed block in memory line
	struct a3d_memory_block_header* type_next;

	/// Memory block size without header
	uint32_t size;

	/// Previous memory block size
	uint32_t size_prev;

	/// Memory block stores data (not free)
	bool busy;

	/// Memory block is last in memory line (next block does not exist)
	bool last;
};

/// Memory line header
struct a3d_memory_line_header
{
	/// Busy memory blocks count
	uint32_t busy_blocks;
};

/// Free memory blocks tree node
struct a3d_free_blocks_rbtree_node
{
	/// Pointer to parent node
	struct a3d_free_blocks_rbtree_node* parent;

	/// Pointer to left child node (less memory block size)
	struct a3d_free_blocks_rbtree_node* left;

	/// Pointer to right child node (greater memory block size)
	struct a3d_free_blocks_rbtree_node* right;

	/// Pointer to memory block
	struct a3d_memory_block_header* block;

	/// Memory block size
	uint32_t size;

	/// Node color (false = black, true = red)
	bool red;
};

/// Free memory blocks tree empty node
struct a3d_free_blocks_rbtree_node_empty
{
	/// Pointer to next empty node
	struct a3d_free_blocks_rbtree_node_empty* next;
};

/// Free memory blocks tree, containing nodes in flat dynamic array
struct a3d_free_blocks_rbtree
{
	/// Tree nodes pool
	struct a3d_free_blocks_rbtree_node* nodes;

	/// Empty nodes (holes) list
	struct a3d_free_blocks_rbtree_node_empty* empties;

	/// First free node after last busy
	struct a3d_free_blocks_rbtree_node* first_free;

	/// Nodes dynamic array size
	uint32_t size;

	/// Nodes dynamic array capacity
	uint32_t capacity;
};

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#ifdef __cplusplus
}
#endif // __cplusplus

// =========================================
// IMPLEMENTATION
// Include this header to one .c or .cpp:
// #define A3D_IMPLEMENTATION
// #include "rapid_alloc.h"
// =========================================

//#ifdef A3D_IMPLEMENTATION
#include <stdlib.h>

#ifdef A3D_LEAKS_CHECK
int32_t g_allocs = 0;
inline static void* a3d_sys_alloc(size_t size)
{
	++g_allocs;
	return malloc(size);
}
inline static void a3d_sys_free(void* ptr)
{
	--g_allocs;
	free(ptr);
}
static void a3d_check()
{
	if (g_allocs > 0)
	{
		fprintf(stderr, "Memory leaks detected: allocated without free.\n");
		exit(-1);
		return;
	}
	else if (g_allocs < 0)
	{
		fprintf(stderr, "Memory leaks detected: free already deallocated memory.\n");
		exit(-1);
		return;
	}
}
#else // A3D_LEAKS_CHECK
#define a3d_sys_alloc(SIZE) malloc(SIZE)
#define a3d_sys_free(PTR) free(PTR)
#define a3d_check(...)
#endif // A3D_LEAKS_CHECK

#ifdef A3D_STRONGER_CHECKS
#include <stdarg.h>
#include <stdio.h>
inline static void a3d_assert(int result, const char* format, ...)
{
	if (result == 0)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		exit(-1);
	}
}
#else // A3D_STRONGER_CHECKS
#define a3d_assert(...)
#endif // A3D_STRONGER_CHECKS

// =========================================
// Free Memory Blocks Red-Black Tree
// =========================================

inline static struct a3d_free_blocks_rbtree_node*
a3d_free_blocks_rbtree_grandparent(struct a3d_free_blocks_rbtree_node* node)
{
	if (node != NULL && node->parent != NULL)
		return node->parent->parent;
	else
		return NULL;
}

inline static struct a3d_free_blocks_rbtree_node*
a3d_free_blocks_rbtree_uncle(struct a3d_free_blocks_rbtree_node* node)
{
	struct a3d_free_blocks_rbtree_node* grandparent = a3d_free_blocks_rbtree_grandparent(node);
	if (grandparent == NULL)
		return NULL;
	if (node->parent == grandparent->left)
		return grandparent->right;
	else
		return grandparent->left;
}

inline static struct a3d_free_blocks_rbtree_node*
a3d_free_blocks_rbtree_sibling(struct a3d_free_blocks_rbtree_node* node)
{
	if (node == node->parent->left)
		return node->parent->right;
	else
		return node->parent->left;
}

inline static void
a3d_free_blocks_rbtree_rotate_left(struct a3d_free_blocks_rbtree_node* node)
{
	struct a3d_free_blocks_rbtree_node* pivot = node->right;

	pivot->parent = node->parent;
	if (node->parent != NULL)
	{
		if (node->parent->left == node)
			node->parent->left = pivot;
		else
			node->parent->right = pivot;
	}

	node->right = pivot->left;
	if (pivot->left != NULL)
		pivot->left->parent = node;

	node->parent = pivot;
	pivot->left = node;
}

inline static void
a3d_free_blocks_rbtree_rotate_right(struct a3d_free_blocks_rbtree_node* node)
{
	struct a3d_free_blocks_rbtree_node* pivot = node->left;

	pivot->parent = node->parent;
	if (node->parent != NULL)
	{
		if (node->parent->left == node)
			node->parent->left = pivot;
		else
			node->parent->right = pivot;
	}

	node->left = pivot->right;
	if (pivot->right != NULL)
		pivot->right->parent = node;

	node->parent = pivot;
	pivot->right = node;
}

inline static void
a3d_free_blocks_rbtree_init(struct a3d_free_blocks_rbtree* tree, uint32_t capacity)
{
	tree->nodes = (struct a3d_free_blocks_rbtree_node*)a3d_sys_alloc(sizeof(struct a3d_free_blocks_rbtree_node) * (size_t)capacity);
	tree->empties = NULL;
	tree->first_free = tree->nodes;
	tree->size = 0;
	tree->capacity = capacity;
}

inline static void
a3d_free_blocks_rbtree_destroy(struct a3d_free_blocks_rbtree* tree)
{
	a3d_sys_free(tree->nodes);
}

inline static struct a3d_free_blocks_rbtree_node*
a3d_free_blocks_rbtree_find_min(struct a3d_free_blocks_rbtree* tree, uint32_t key)
{
	struct a3d_free_blocks_rbtree_node* node = tree->nodes;
	if (node == NULL)
		return NULL;

	while (true)
	{
		if (node->size == key)
			return node;
		else if (node->size > key)
		{
			if (node->left != NULL)
			{
				if (node->left->size < key)
					return node->left;
				else
					node = node->left;
			}
			else
				return node;
		}
		else
		{
			if (node->right != NULL)
			{
				if (node->right->size > key)
					return node->right;
				else
					node = node->right;
			}
			else
				return NULL;
		}
	}

	return node;
};

inline static void
a3d_free_blocks_rbtree_insert1(struct a3d_free_blocks_rbtree* tree,
							  uint32_t key,
							  struct a3d_memory_block_header* value)
{
};

// =========================================
// Memory Block
// =========================================

#define A3D_MB_DATA(HEADER) ((void*)((HEADER) + 1))
#define A3D_MB_HEADER_SIZE (sizeof(struct a3d_memory_block_header))
#define A3D_MB_NEXT_HEADER(DATA, SIZE) ((struct a3d_memory_block_header*)((uint8_t*)DATA + SIZE))
#define A3D_MB_SIBL_SIZE(HEADER, SIZE) ((int32_t)(HEADER->size - (int32_t)A3D_MB_HEADER_SIZE - SIZE))

inline static struct a3d_memory_block_header* a3d_memory_block_split(struct a3d_memory_block_header* block, size_t size)
{
	a3d_assert(size <= block->size, "Could not allocate %llu bytes from memory block with size %llu", size, block->size);

	const int32_t sibling_size = A3D_MB_SIBL_SIZE(block, size);

	const uint32_t old_size = block->size;
	block->size = size;
	block->busy = true;

	if (sibling_size > 0) // Block has got free memory rest
	{
		void* data_ptr = A3D_MB_DATA(block);
		struct a3d_memory_block_header* sibling = A3D_MB_NEXT_HEADER(data_ptr, size);
		sibling->size = (uint32_t)sibling_size;
		sibling->size_prev = size;
		sibling->busy = false;

		if (block->last == false) // There are blocks after merged block
		{
			struct a3d_memory_block_header* next = A3D_MB_NEXT_HEADER(data_ptr, old_size);
			next->size_prev = sibling_size;
		}
		else // This block is last
		{
			block->last = false;
			sibling->last = true;
		}

		return sibling;
	}
	else // Block memory has been fully allocated
		return NULL;
}

inline static uint32_t a3d_memory_block_merge(struct a3d_memory_block_header* left, struct a3d_memory_block_header* right)
{
	left->size += right->size + A3D_MB_HEADER_SIZE;
	left->busy = false;

	if (right->last == false) // There are blocks after merged block
	{
		struct a3d_memory_block_header* next = A3D_MB_NEXT_HEADER(A3D_MB_DATA(left), left->size);
		next->size_prev = left->size;
	}
	else // This block is last
		left->last = true;

	return left->size;
}

// =========================================
// Memory Line
// =========================================

#define A3D_ML_FIRST_MB(LINE) ((struct a3d_memory_block_header*)((LINE) + 1))
#define A3D_ML_SIZE(SIZE) (sizeof(struct a3d_memory_line_header) + A3D_MB_HEADER_SIZE + (SIZE))

inline static struct a3d_memory_line_header* a3d_memory_line_init(uint32_t size)
{
	// Memory line must be able to contain at least one memory block
	struct a3d_memory_line_header* line = (struct a3d_memory_line_header*)a3d_sys_alloc(A3D_ML_SIZE(size));
	line->busy_blocks = 0;

	struct a3d_memory_block_header* block = A3D_ML_FIRST_MB(line);
	block->size = size;
	block->size_prev = 0;
	block->busy = false;
	block->last = true;

	return line;
}

inline static void a3d_memory_line_destroy(struct a3d_memory_line_header* line)
{
	a3d_sys_free(line);
}

//#endif // A3D_IMPLEMENTATION

#endif // RAPID_ALLOC_H
