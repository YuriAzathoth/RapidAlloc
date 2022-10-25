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

#include <stdio.h>
#include "acutest/include/acutest.h"
#include "rapid_alloc.h"

#define A3D_MB_DATA(HEADER) ((void*)((HEADER) + 1))
#define A3D_MB_HEADER_SIZE (sizeof(struct a3d_memory_block_header))
#define A3D_MB_NEXT_HEADER(DATA, SIZE) ((struct a3d_memory_block_header*)((uint8_t*)DATA + SIZE))
#define A3D_MB_SIBL_SIZE(HEADER, SIZE) ((int32_t)(HEADER->size - (int32_t)A3D_MB_HEADER_SIZE - SIZE))
#define A3D_ML_FIRST_MB(LINE) ((struct a3d_memory_block_header*)((LINE) + 1))
#define A3D_ML_SIZE(SIZE) (sizeof(struct a3d_memory_line_header) + A3D_MB_HEADER_SIZE + (SIZE))

#define TEST_LINE_SIZE 2048
#define TEST_ALLOC_SIZE 512

void test_line_create_destroy()
{
	struct a3d_memory_line_header* line = a3d_memory_line_init(TEST_LINE_SIZE);
	struct a3d_memory_block_header* block = A3D_ML_FIRST_MB(line);
	TEST_ASSERT(block != NULL);
	TEST_ASSERT(block->size == TEST_LINE_SIZE);
	TEST_ASSERT(block->size_prev == 0);
	TEST_ASSERT(block->busy == false);
	TEST_ASSERT(block->last == true);
	a3d_memory_line_destroy(line);
	a3d_memory_check();
}

void test_block_split()
{
	struct a3d_memory_line_header* line = a3d_memory_line_init(TEST_LINE_SIZE);
	struct a3d_memory_block_header* first = A3D_ML_FIRST_MB(line);
	struct a3d_memory_block_header* second = a3d_memory_block_split(first, TEST_ALLOC_SIZE);
	TEST_ASSERT(first->size == TEST_ALLOC_SIZE);
	TEST_ASSERT(first->size_prev == 0);
	TEST_ASSERT(first->busy == true);
	TEST_ASSERT(first->last == false);
	TEST_ASSERT(second != NULL);
	TEST_ASSERT(second->size == TEST_LINE_SIZE - sizeof(struct a3d_memory_block_header) - TEST_ALLOC_SIZE);
	TEST_ASSERT(second->size_prev == first->size);
	TEST_ASSERT(second->busy == false);
	TEST_ASSERT(second->last == true);
	a3d_memory_line_destroy(line);
	a3d_memory_check();
}

void test_block_split_full()
{
	struct a3d_memory_line_header* line = a3d_memory_line_init(TEST_LINE_SIZE);
	struct a3d_memory_block_header* first = A3D_ML_FIRST_MB(line);
	struct a3d_memory_block_header* second = a3d_memory_block_split(first, TEST_LINE_SIZE);
	TEST_ASSERT(first->size == TEST_LINE_SIZE);
	TEST_ASSERT(first->size_prev == 0);
	TEST_ASSERT(first->busy == true);
	TEST_ASSERT(first->last == true);
	TEST_ASSERT(second == NULL);
	a3d_memory_line_destroy(line);
	a3d_memory_check();
}

void test_block_merge()
{
	struct a3d_memory_line_header* line = a3d_memory_line_init(TEST_LINE_SIZE);
	struct a3d_memory_block_header* first = A3D_ML_FIRST_MB(line);
	struct a3d_memory_block_header* second = a3d_memory_block_split(first, TEST_ALLOC_SIZE);
	a3d_memory_block_merge(first, second);
	TEST_ASSERT(first->size == TEST_LINE_SIZE);
	TEST_ASSERT(first->size_prev == 0);
	TEST_ASSERT(first->busy == false);
	TEST_ASSERT(first->last == true);
	a3d_memory_line_destroy(line);
	a3d_memory_check();
}

void test_block_merge_2()
{
	struct a3d_memory_line_header* line = a3d_memory_line_init(TEST_LINE_SIZE);
	struct a3d_memory_block_header* first = A3D_ML_FIRST_MB(line);
	struct a3d_memory_block_header* second = a3d_memory_block_split(first, TEST_ALLOC_SIZE);
	struct a3d_memory_block_header* third = a3d_memory_block_split(second, TEST_ALLOC_SIZE);
	a3d_memory_block_merge(first, second);
	TEST_ASSERT(first->last == false);
	TEST_ASSERT(second->last == false); // We can, because 2nd is still in memory
	TEST_ASSERT(third->last == true);
	a3d_memory_line_destroy(line);
	a3d_memory_check();
}

void test_rbtree_create_destroy()
{
	struct a3d_free_blocks_rbtree tree;
	a3d_free_blocks_rbtree_init(&tree, 128);
	a3d_free_blocks_rbtree_destroy(&tree);
	a3d_memory_check();
}

TEST_LIST =
{
	{ "Line Create/Destroy", test_line_create_destroy },
	{ "Block Split", test_block_split },
	{ "Block Split (Full)", test_block_split_full },
	{ "Block Merge", test_block_merge },
	{ "Block Merge (Two Times)", test_block_merge_2 },
	{ "RBTree Create/Destroy", test_rbtree_create_destroy },
	{ NULL, NULL }
};
