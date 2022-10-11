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
struct ra_memory_block_header
{
	/// Previous typed block in memory line
	struct ra_memory_block_header* type_prev;

	/// Next typed block in memory line
	struct ra_memory_block_header* type_next;

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
struct ra_memory_line_header
{
	/// Busy memory blocks count
	uint32_t busy_blocks;
};

//#ifdef RA_IMPLEMENTATION
#include <stdlib.h>

#ifdef RA_LEAKS_CHECK
int32_t g_allocs = 0;
inline static void* ra_sys_alloc(size_t size)
{
	++g_allocs;
	return malloc(size);
}
inline static void ra_sys_free(void* ptr)
{
	--g_allocs;
	free(ptr);
}
static void ra_check()
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
#else // RA_LEAKS_CHECK
#define ra_sys_alloc(SIZE) malloc(SIZE)
#define ra_sys_free(PTR) free(PTR)
#define ra_check(...)
#endif // RA_LEAKS_CHECK

#ifdef RA_STRONGER_CHECKS
#include <stdarg.h>
#include <stdio.h>
inline static void ra_assert(int result, const char* format, ...)
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
#else // RA_STRONGER_CHECKS
#define ra_assert(...)
#endif // RA_STRONGER_CHECKS

// =========================================
// Memory Block
// =========================================

#define RA_MB_DATA(HEADER) ((void*)((HEADER) + 1))
#define RA_MB_HEADER_SIZE (sizeof(struct ra_memory_block_header))
#define RA_MB_NEXT_HEADER(DATA, SIZE) ((struct ra_memory_block_header*)((uint8_t*)DATA + SIZE))
#define RA_MB_SIBL_SIZE(HEADER, SIZE) ((int32_t)(HEADER->size - (int32_t)RA_MB_HEADER_SIZE - SIZE))

inline static struct ra_memory_block_header* ra_memory_block_split(struct ra_memory_block_header* block, size_t size)
{
	ra_assert(size <= block->size, "Could not allocate %llu bytes from memory block with size %llu", size, block->size);

	const int32_t sibling_size = RA_MB_SIBL_SIZE(block, size);

	const uint32_t old_size = block->size;
	block->size = size;
	block->busy = true;

	if (sibling_size > 0) // Block has got free memory rest
	{
		void* data_ptr = RA_MB_DATA(block);
		struct ra_memory_block_header* sibling = RA_MB_NEXT_HEADER(data_ptr, size);
		sibling->size = (uint32_t)sibling_size;
		sibling->size_prev = size;
		sibling->busy = false;

		if (block->last == false) // There are blocks after merged block
		{
			struct ra_memory_block_header* next = RA_MB_NEXT_HEADER(data_ptr, old_size);
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

inline static uint32_t ra_memory_block_merge(struct ra_memory_block_header* left, struct ra_memory_block_header* right)
{
	left->size += right->size + RA_MB_HEADER_SIZE;
	left->busy = false;

	if (right->last == false) // There are blocks after merged block
	{
		struct ra_memory_block_header* next = RA_MB_NEXT_HEADER(RA_MB_DATA(left), left->size);
		next->size_prev = left->size;
	}
	else // This block is last
		left->last = true;

	return left->size;
}

// =========================================
// Memory Line
// =========================================

#define RA_ML_FIRST_MB(LINE) ((struct ra_memory_block_header*)((LINE) + 1))
#define RA_ML_SIZE(SIZE) (sizeof(struct ra_memory_line_header) + RA_MB_HEADER_SIZE + (SIZE))

inline static struct ra_memory_line_header* ra_memory_line_init(uint32_t size)
{
	// Memory line must be able to contain at least one memory block
	struct ra_memory_line_header* line = (struct ra_memory_line_header*)ra_sys_alloc(RA_ML_SIZE(size));
	line->busy_blocks = 0;

	struct ra_memory_block_header* block = RA_ML_FIRST_MB(line);
	block->size = size;
	block->size_prev = 0;
	block->busy = false;
	block->last = true;

	return line;
}

inline static void ra_memory_line_destroy(struct ra_memory_line_header* line)
{
	ra_sys_free(line);
}

//#endif // RA_IMPLEMENTATION

#endif // RAPID_ALLOC_H
