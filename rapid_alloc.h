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

#include <stdint.h>

/// Free or busy memory block header in memory line
struct ra_memory_block_header
{
	/// Previous sequented block in memory line
	struct ra_memory_block_header* mem_prev;

	/// Next sequented block in memory line
	struct ra_memory_block_header* mem_next;

	/// Previous typed block in memory line
	struct ra_memory_block_header* type_prev;

	/// Next typed block in memory line
	struct ra_memory_block_header* type_next;

	/// Memory block size without header
	uint32_t size;

	/// Memory block stores data (not free)
	uint32_t busy;
};

/// Memory line header
struct ra_memory_line_header
{
	/// First sequented memory block
	struct ra_memory_block_header* mem_first;
};

/// Initialize memory line with fixed size
struct ra_memory_line_header* ra_memory_line_init(uint32_t size);

/// Destroy memory line
void ra_memory_line_destroy(struct ra_memory_line_header* line);

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
#else // RA_LEAKS_CHECK
#define ra_sys_alloc(SIZE) malloc(SIZE)
#define ra_sys_free(PTR) free(PTR)
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
#define RA_MB_SIBL_SIZE(HEADER, SIZE) ((ptrdiff_t)(HEADER->size - RA_MB_HEADER_SIZE - SIZE))

inline static struct ra_memory_block_header* ra_memory_block_split(struct ra_memory_block_header* block, size_t size)
{
	ra_assert(size <= block->size, "Could not allocate %llu bytes from memory block with size %llu", size, block->size);

	const ptrdiff_t sibling_size = RA_MB_SIBL_SIZE(block, size);
	block->busy = 1;
	block->size = size;

	if (sibling_size > 0) // Block has got free memory rest
	{
		struct ra_memory_block_header* old_next = block->mem_next;
		struct ra_memory_block_header* new_next = RA_MB_NEXT_HEADER(RA_MB_DATA(block), size);
		block->mem_next = new_next;
		new_next->mem_prev = block;
		new_next->mem_next = old_next;
		new_next->size = sibling_size;
		new_next->busy = 0;
		if (old_next != NULL)
			old_next->mem_prev = new_next;
		return new_next;
	}
	else // Block memory is fully allocated
		return NULL;
}

inline static uint32_t ra_memory_block_merge(struct ra_memory_block_header* left, struct ra_memory_block_header* right)
{
	left->mem_next = right->mem_next;
	left->size += right->size + RA_MB_HEADER_SIZE;
	left->busy = 0;
	return left->size;
}

// =========================================
// Memory Line
// =========================================

#define RA_FIRST_MEMORY_BLOCK(LINE) ((struct ra_memory_block_header*)((LINE) + 1))
#define RA_MEMORY_LINE_SIZE(SIZE) (sizeof(struct ra_memory_line_header) + (SIZE))

struct ra_memory_line_header* ra_memory_line_init(uint32_t size)
{
	struct ra_memory_line_header* line = (struct ra_memory_line_header*)ra_sys_alloc(RA_MEMORY_LINE_SIZE(size));
	line->mem_first = RA_FIRST_MEMORY_BLOCK(line);
	line->mem_first->mem_prev = NULL;
	line->mem_first->mem_next = NULL;
	line->mem_first->size = size;
	line->mem_first->busy = 0;
	return line;
}

void ra_memory_line_destroy(struct ra_memory_line_header* line)
{
	ra_sys_free(line);
}

//#endif // RA_IMPLEMENTATION

#endif // RAPID_ALLOC_H
