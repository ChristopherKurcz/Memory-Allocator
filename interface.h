#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

// Macros for memory allocator configuration
#ifndef MY_EXTERNAL_DEFINE
#define MEMORY_SIZE 8 * 1024 * 1024
#define HEADER_SIZE 8
#define MIN_MEM_CHUNK_SIZE 512
#define N_OBJS_PER_SLAB 64
#endif

// Allocation type
enum malloc_type
{
    MALLOC_BUDDY = 0, // Buddy allocator
    MALLOC_SLAB = 1,  // Slab allocator
};

// APIs
void my_setup(enum malloc_type type, int mem_size, void *start_of_memory);
void *my_malloc(int size);
void my_free(void *ptr);

#endif
