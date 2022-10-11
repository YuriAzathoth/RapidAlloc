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

#define RA_IMPLEMENTATION
#define RA_LEAKS_CHECK
#define RA_STRONGER_CHECKS
#include <stdio.h>
#include "acutest/include/acutest.h"
#include "rapid_alloc.h"

#define TEST_LINE_SIZE 2048
#define TEST_ALLOC_SIZE 512

void test_line_create_destroy()
{
	struct ra_memory_line_header* line = ra_memory_line_init(TEST_LINE_SIZE);
	TEST_ASSERT(line->mem_first != NULL);
	TEST_ASSERT(line->mem_first->size == TEST_LINE_SIZE);
	TEST_ASSERT(line->mem_first->size_prev == 0);
	TEST_ASSERT(line->mem_first->busy == false);
	TEST_ASSERT(line->mem_first->last == true);
	ra_memory_line_destroy(line);
	ra_check();
}

void test_block_split()
{
	struct ra_memory_line_header* line = ra_memory_line_init(TEST_LINE_SIZE);
	struct ra_memory_block_header* first = line->mem_first;
	struct ra_memory_block_header* second = ra_memory_block_split(first, TEST_ALLOC_SIZE);
	TEST_ASSERT(first->size == TEST_ALLOC_SIZE);
	TEST_ASSERT(first->size_prev == 0);
	TEST_ASSERT(first->busy == true);
	TEST_ASSERT(first->last == false);
	TEST_ASSERT(second != NULL);
	TEST_ASSERT(second->size == TEST_LINE_SIZE - RA_MB_HEADER_SIZE - TEST_ALLOC_SIZE);
	TEST_ASSERT(second->size_prev == first->size);
	TEST_ASSERT(second->busy == false);
	TEST_ASSERT(second->last == true);
	ra_memory_line_destroy(line);
	ra_check();
}

void test_block_split_full()
{
	struct ra_memory_line_header* line = ra_memory_line_init(TEST_LINE_SIZE);
	struct ra_memory_block_header* first = line->mem_first;
	struct ra_memory_block_header* second = ra_memory_block_split(first, TEST_LINE_SIZE);
	TEST_ASSERT(first->size == TEST_LINE_SIZE);
	TEST_ASSERT(first->size_prev == 0);
	TEST_ASSERT(first->busy == true);
	TEST_ASSERT(first->last == true);
	TEST_ASSERT(second == NULL);
	ra_memory_line_destroy(line);
	ra_check();
}

void test_block_merge()
{
	struct ra_memory_line_header* line = ra_memory_line_init(TEST_LINE_SIZE);
	struct ra_memory_block_header* first = line->mem_first;
	struct ra_memory_block_header* second = ra_memory_block_split(first, TEST_ALLOC_SIZE);
	ra_memory_block_merge(first, second);
	TEST_ASSERT(first->size == TEST_LINE_SIZE);
	TEST_ASSERT(first->size_prev == 0);
	TEST_ASSERT(first->busy == false);
	TEST_ASSERT(first->last == true);
	ra_memory_line_destroy(line);
	ra_check();
}

void test_block_merge_2()
{
	struct ra_memory_line_header* line = ra_memory_line_init(TEST_LINE_SIZE);
	struct ra_memory_block_header* first = line->mem_first;
	struct ra_memory_block_header* second = ra_memory_block_split(first, TEST_ALLOC_SIZE);
	struct ra_memory_block_header* third = ra_memory_block_split(second, TEST_ALLOC_SIZE);
	ra_memory_block_merge(first, second);
	TEST_ASSERT(first->last == false);
	TEST_ASSERT(second->last == false); // We can, because 2nd is still in memory
	TEST_ASSERT(third->last == true);
	ra_memory_line_destroy(line);
	ra_check();
}

TEST_LIST =
{
	{ "Line Create/Destroy", test_line_create_destroy },
	{ "Block Split", test_block_split },
	{ "Block Split (Full)", test_block_split_full },
	{ "Block Merge", test_block_merge },
	{ "Block Merge (Two Times)", test_block_merge_2 },
	{ NULL, NULL }
};
