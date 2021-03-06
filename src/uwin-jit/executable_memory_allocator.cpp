/*
 *    Originally from Stack-less Just-In-Time compiler
 *
 *    Copyright Zoltan Herczeg (hzmester@freemail.hu). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER(S) OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
   This file contains a simple executable memory allocator

   It is assumed, that executable code blocks are usually medium (or sometimes
   large) memory blocks, and the allocator is not too frequently called (less
   optimized than other allocators). Thus, using it as a generic allocator is
   not suggested.

   How does it work:
     Memory is allocated in continuous memory areas called chunks by alloc_chunk()
     Chunk format:
     [ block ][ block ] ... [ block ][ block terminator ]

   All blocks and the block terminator is started with block_header. The block
   header contains the size of the previous and the next block. These sizes
   can also contain special values.
     Block size:
       0 - The block is a free_block, with a different size member.
       1 - The block is a block terminator.
       n - The block is used at the moment, and the value contains its size.
     Previous block size:
       0 - This is the first block of the memory chunk.
       n - The size of the previous block.

   Using these size values we can go forward or backward on the block chain.
   The unused blocks are stored in a chain list pointed by free_blocks. This
   list is useful if we need to find a suitable memory area when the allocator
   is called.

   When a block is freed, the new free block is connected to its adjacent free
   blocks if possible.

     [ free block ][ used block ][ free block ]
   and "used block" is freed, the three blocks are connected together:
     [           one big free block           ]
*/

/* --------------------------------------------------------------------- */
/*  System (OS) functions                                                */
/* --------------------------------------------------------------------- */

/* 64 KByte. */
// hopefully should be aligned enough for any host
#define CHUNK_SIZE	0x10000

struct chunk_header {
	void *executable;
};

/*
   alloc_chunk / free_chunk :
     * allocate executable system memory chunks
     * the size is always divisible by CHUNK_SIZE
   allocator_grab_lock / allocator_release_lock :
     * make the allocator thread safe
     * can be empty if the OS (or the application) does not support threading
     * only the allocator requires this lock, sljit is fully thread safe
       as it only uses local variables
*/

#include "uwin/uwin.h"
#include "uwin-jit/executable_memory_allocator.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <stdint.h>

namespace uwin {
    namespace jit {
        namespace xmem {

            // not very C++-ish code actually. But hopefully it works (well, in C++20 =))
            static inline struct chunk_header *alloc_chunk(size_t size) {
                struct chunk_header *retval;
                int fd;

                void *rw, *rx;

                int res = uw_jitmem_alloc(&rw, &rx, size);

                if (res)
                    return NULL;

                retval = (struct chunk_header *) rw;
                retval->executable = rx;

                return retval;
            }

            static inline void free_chunk(void *chunk, size_t size) {
                struct chunk_header *header = ((struct chunk_header *) chunk) - 1;

                void *rw = header, *rx = header->executable;

                uw_jitmem_free(rw, rx, size);
            }

/* --------------------------------------------------------------------- */
/*  Common functions                                                     */
/* --------------------------------------------------------------------- */

#define CHUNK_MASK    (~(CHUNK_SIZE - 1))

            struct block_header {
                size_t size;
                size_t prev_size;
                ptrdiff_t executable_offset;
            };

            struct free_block {
                struct block_header header;
                struct free_block *next;
                struct free_block *prev;
                size_t size;
            };

#ifdef __GNUC__
#define LIKELY(x)       __builtin_expect(!!(x), 1)
#define UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif

#define AS_BLOCK_HEADER(base, offset) \
    ((struct block_header*)(((uint8_t*)base) + offset))
#define AS_FREE_BLOCK(base, offset) \
    ((struct free_block*)(((uint8_t*)base) + offset))
#define MEM_START(base)        ((void*)((base) + 1))
#define ALIGN_SIZE(size)    (((size) + sizeof(struct block_header) + 7) & ~7)

            static struct free_block *free_blocks;
            static size_t allocated_size;
            static size_t total_size;

            static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

            static void allocator_grab_lock() {
                pthread_mutex_lock(&mut);
            }

            static void allocator_release_lock() {
                pthread_mutex_unlock(&mut);
            }

            static inline void insert_free_block(struct free_block *free_block, size_t size) {
                free_block->header.size = 0;
                free_block->size = size;

                free_block->next = free_blocks;
                free_block->prev = NULL;
                if (free_blocks)
                    free_blocks->prev = free_block;
                free_blocks = free_block;
            }

            static inline void remove_free_block(struct free_block *free_block) {
                if (free_block->next)
                    free_block->next->prev = free_block->prev;

                if (free_block->prev)
                    free_block->prev->next = free_block->next;
                else {
                    assert(free_blocks == free_block);
                    free_blocks = free_block->next;
                }
            }

            void *alloc(size_t size) {
                struct chunk_header *chunk_header;
                struct block_header *header;
                struct block_header *next_header;
                struct free_block *free_block;
                size_t chunk_size;
                size_t executable_offset;

                allocator_grab_lock();
                if (size < (64 - sizeof(struct block_header)))
                    size = (64 - sizeof(struct block_header));
                size = ALIGN_SIZE(size);

                free_block = free_blocks;
                while (free_block) {
                    if (free_block->size >= size) {
                        chunk_size = free_block->size;
                        if (chunk_size > size + 64) {
                            /* We just cut a block from the end of the free block. */
                            chunk_size -= size;
                            free_block->size = chunk_size;
                            header = AS_BLOCK_HEADER(free_block, chunk_size);
                            header->prev_size = chunk_size;
                            header->executable_offset = free_block->header.executable_offset;
                            AS_BLOCK_HEADER(header, size)->prev_size = size;
                        } else {
                            remove_free_block(free_block);
                            header = (struct block_header *) free_block;
                            size = chunk_size;
                        }
                        allocated_size += size;
                        header->size = size;
                        allocator_release_lock();
                        return MEM_START(header);
                    }
                    free_block = free_block->next;
                }

                chunk_size = sizeof(struct chunk_header) + sizeof(struct block_header);
                chunk_size = (chunk_size + size + CHUNK_SIZE - 1) & CHUNK_MASK;

                chunk_header = alloc_chunk(chunk_size);
                if (!chunk_header) {
                    allocator_release_lock();
                    return NULL;
                }

                executable_offset = (ptrdiff_t) ((uint8_t *) chunk_header->executable - (uint8_t *) chunk_header);

                chunk_size -= sizeof(struct chunk_header) + sizeof(struct block_header);
                total_size += chunk_size;

                header = (struct block_header *) (chunk_header + 1);

                header->prev_size = 0;
                header->executable_offset = executable_offset;
                if (chunk_size > size + 64) {
                    /* Cut the allocated space into a free and a used block. */
                    allocated_size += size;
                    header->size = size;
                    chunk_size -= size;

                    free_block = AS_FREE_BLOCK(header, size);
                    free_block->header.prev_size = size;
                    free_block->header.executable_offset = executable_offset;
                    insert_free_block(free_block, chunk_size);
                    next_header = AS_BLOCK_HEADER(free_block, chunk_size);
                } else {
                    /* All space belongs to this allocation. */
                    allocated_size += chunk_size;
                    header->size = chunk_size;
                    next_header = AS_BLOCK_HEADER(header, chunk_size);
                }
                next_header->size = 1;
                next_header->prev_size = chunk_size;
                next_header->executable_offset = executable_offset;
                allocator_release_lock();
                return MEM_START(header);
            }

            void free(void *ptr) {
                struct block_header *header;
                struct free_block *free_block;

                allocator_grab_lock();
                header = AS_BLOCK_HEADER(ptr, -(ptrdiff_t) sizeof(struct block_header));
                //header = AS_BLOCK_HEADER(header, -header->executable_offset);
                allocated_size -= header->size;

                /* Connecting free blocks together if possible. */

                /* If header->prev_size == 0, free_block will equal to header.
                   In this case, free_block->header.size will be > 0. */
                free_block = AS_FREE_BLOCK(header, -(ptrdiff_t) header->prev_size);
                if (UNLIKELY(!free_block->header.size)) {
                    free_block->size += header->size;
                    header = AS_BLOCK_HEADER(free_block, free_block->size);
                    header->prev_size = free_block->size;
                } else {
                    free_block = (struct free_block *) header;
                    insert_free_block(free_block, header->size);
                }

                header = AS_BLOCK_HEADER(free_block, free_block->size);
                if (UNLIKELY(!header->size)) {
                    free_block->size += ((struct free_block *) header)->size;
                    remove_free_block((struct free_block *) header);
                    header = AS_BLOCK_HEADER(free_block, free_block->size);
                    header->prev_size = free_block->size;
                }

                /* The whole chunk is free. */
                if (UNLIKELY(!free_block->header.prev_size && header->size == 1)) {
                    /* If this block is freed, we still have (allocated_size / 2) free space. */
                    if (total_size - free_block->size > (allocated_size * 3 / 2)) {
                        total_size -= free_block->size;
                        remove_free_block(free_block);
                        free_chunk(free_block, free_block->size +
                                               sizeof(struct chunk_header) +
                                               sizeof(struct block_header));
                    }
                }

                allocator_release_lock();
            }

            void free_unused_regions(void) {
                struct free_block *free_block;
                struct free_block *next_free_block;

                allocator_grab_lock();

                free_block = free_blocks;
                while (free_block) {
                    next_free_block = free_block->next;
                    if (!free_block->header.prev_size &&
                        AS_BLOCK_HEADER(free_block, free_block->size)->size == 1) {
                        total_size -= free_block->size;
                        remove_free_block(free_block);
                        free_chunk(free_block, free_block->size +
                                               sizeof(struct chunk_header) +
                                               sizeof(struct block_header));
                    }
                    free_block = next_free_block;
                }

                assert((total_size && free_blocks) || (!total_size && !free_blocks));
                allocator_release_lock();
            }

            ptrdiff_t executable_offset(void *ptr) {
                return ((struct block_header *) (ptr))[-1].executable_offset;
            }

        }
    }
}