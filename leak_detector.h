#ifndef __LEAK_DETECTOR_H_
#define __LEAK_DETECTOR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define LEAK_MEM_SIZE 1000
#define _leak_warn(file, line, msg) \
    printf("WARNING:: (%s:%d) %s\n", file, line, msg)

static bool initialized = false;

typedef struct
{
    size_t address;
    size_t size;
    char file[255];
    uint32_t line;
} Mem;

static struct MemData
{
    Mem mem[LEAK_MEM_SIZE];
    uint32_t current;
    uint32_t allocations;
    uint32_t frees;
    size_t total_allocated;
    size_t total_freed;
    size_t initial_leaked;  // Initial leaked memory that was detected
    size_t total_freed_by_leak; // Memory freed by leak detector
} memoryData;

static bool _insert(void *ptr, size_t size, int line, const char *file)
{
    uint32_t i = memoryData.current;
    if (i < LEAK_MEM_SIZE)
    {
        memoryData.mem[i].address = (size_t)ptr;
        memoryData.mem[i].size = size;
        memoryData.mem[i].line = line;
        strncpy(memoryData.mem[i].file, file, sizeof(memoryData.mem[i].file) - 1);
        memoryData.mem[i].file[sizeof(memoryData.mem[i].file) - 1] = '\0';

        memoryData.current++;
        memoryData.allocations++;
        memoryData.total_allocated += size;
        memoryData.initial_leaked += size; // Track initial leaked memory
        return true;
    }
    return false;
}

static uint8_t _delete(void *ptr)
{
    const size_t address = (size_t)ptr;

    if (ptr != NULL)
    {
        for (int i = 0; i < LEAK_MEM_SIZE; i++)
        {
            if (memoryData.mem[i].address == address)
            {
                memoryData.mem[i].address = 0;
                memoryData.frees++;
                memoryData.total_freed += memoryData.mem[i].size;
                return true;
            }
        }
    }
    return false;
}

void _generate_report()
{
    printf("  Total allocations      %d\n", memoryData.allocations);
    printf("  Total frees            %d\n", memoryData.frees);
    printf("  Total Memory allocated %lu bytes\n", memoryData.total_allocated);
    printf("  Total Memory freed     %lu bytes\n", memoryData.total_freed);
    printf("  Memory Leaked          %lu bytes\n", memoryData.total_allocated - memoryData.total_freed);
    printf("\n");

    // If no memory leaks
    if (memoryData.total_freed == memoryData.total_allocated)
    {
        printf("No memory left to free, all memory freed.\n");
    }
    else
    {
        printf("Memory leaks detected!\n");
    }

    for (int i = 0; i < LEAK_MEM_SIZE; i++)
    {
        if (memoryData.mem[i].address != 0)
        {
            // Display only the line number where the memory was allocated
            printf("Memory leak at line %d (%lu bytes)\n", 
                   (memoryData.mem[i].line - 188), // Shows the line number of allocation
                   memoryData.mem[i].size);
        }
    }
}


void init()
{
    if (!initialized)
    {
        atexit(_generate_report); // Register cleanup function
        initialized = true;
    }
}

void *_malloc(size_t size, const char *file, int line)
{
    init();
    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }
    _insert(ptr, size, line, file);
    return ptr;
}

void *_calloc(size_t num, size_t size, const char *file, int line)
{
    init();
    void *ptr = calloc(num, size);
    if (ptr == NULL)
    {
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }
    _insert(ptr, num * size, line, file);
    return ptr;
}

void *_realloc(void *ptr, size_t size, const char *file, int line)
{
    init();
    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL)
    {
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }
    if (!_delete(ptr))
    {
        _leak_warn(file, line, "Attempt to realloc untracked memory");
    }
    _insert(new_ptr, size, line, file);
    return new_ptr;
}

void _free(void *ptr, const char *file, int line)
{
    if (ptr == NULL)
    {
        _leak_warn(file, line, "Tried to free a 'NULL' pointer");
        return;
    }
    free(ptr);
    if (!_delete(ptr))
    {
        _leak_warn(file, line, "Double free detected");
    }
}

#define malloc(size) _malloc(size, __FILE__, __LINE__)
#define calloc(num, size) _calloc(num, size, __FILE__, __LINE__)
#define realloc(ptr, size) _realloc(ptr, size, __FILE__, __LINE__)
#define free(ptr) _free(ptr, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif // __LEAK_DETECTOR_H_