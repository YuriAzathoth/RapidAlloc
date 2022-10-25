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

void a3d_free_blocks_rbtree_init(struct a3d_free_blocks_rbtree* tree, uint32_t capacity);
void a3d_free_blocks_rbtree_destroy(struct a3d_free_blocks_rbtree* tree);

struct a3d_memory_block_header* a3d_memory_block_split(struct a3d_memory_block_header* block, size_t size);
uint32_t a3d_memory_block_merge(struct a3d_memory_block_header* left, struct a3d_memory_block_header* right);

struct a3d_memory_line_header* a3d_memory_line_init(uint32_t size);
void a3d_memory_line_destroy(struct a3d_memory_line_header* line);

#ifndef NDEBUG
void a3d_memory_check();
#else // NDEBUG
#define a3d_memory_check()
#endif // NDEBUG

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // RAPID_ALLOC_H
