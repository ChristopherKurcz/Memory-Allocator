#include <sys/stat.h>
#include <libgen.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "interface.h"

#define MAX_LINE_LEN 1024
FILE *output_file;

struct handle
{
    char name;           // name
    void **addresses;    // list of addresses returned from my_malloc()
    int num_allocs;      // number of allocations
    struct handle *next; // pointer to next entry
};
typedef struct handle handle_t;

struct ops
{
    char name;  // name
    int numops; // number of allocation requests OR index
    char type;  // 'M' OR 'F'
    int size;   // size of allocation request
};
typedef struct ops ops_t;

bool read_next_op(FILE *fd, ops_t *op);
void call_my_malloc(handle_t **handles, ops_t *op, void *RAM);
void call_my_free(handle_t *handles, ops_t *op, void *RAM);

// Main function
// Read input file and call functions accordingly
int main(int argc, char *argv[])
{
    printf("%s: Hello Project 2!\n", __func__);
    if (argc < 3)
    {
        fprintf(stderr, "Not enough parameters specified.  Usage: ./proj2 <allocation_type> <input_file>\n");
        fprintf(stderr, "  Allocation type: 0 - Buddy Allocator\n");
        fprintf(stderr, "  Allocation type: 1 - Slab Allocator\n");
        return -1;
    }

    // Verify allocator type
    int type = atoi(argv[1]);
    if (type != MALLOC_BUDDY && type != MALLOC_SLAB)
    {
        fprintf(stderr, "Invalid option\n");
        return -1;
    }

    // Open input file
    FILE *input_file = fopen(argv[2], "r");
    if (input_file == NULL)
    {
        perror("fopen() error");
        return errno;
    }

    // Open output file
    char output_filename[512] = {0};
    mkdir("output", 0755);
    strcat(output_filename, "output/result-");
    strcat(output_filename, argv[1]);
    strcat(output_filename, "-");
    strcat(output_filename, basename(argv[2]));
    output_file = fopen(output_filename, "w");
    if (output_file == NULL)
    {
        perror("fopen() error");
        return errno;
    }

    // Print #define macro info
    printf("%s: MEMORY_SIZE: %d, HEADER_SIZE: %d, MIN_MEM_CHUNK_SIZE: %d, N_OBJS_PER_SLAB: %d\n",
           __func__, MEMORY_SIZE, HEADER_SIZE, MIN_MEM_CHUNK_SIZE, N_OBJS_PER_SLAB);

    // Allocate memory and setup your memory allocator
    int RAM_SIZE = MEMORY_SIZE;
    void *RAM = malloc(RAM_SIZE);
    if (RAM == NULL)
    {
        perror("malloc() error");
        return errno;
    }
    my_setup(type, RAM_SIZE, RAM);

    // List of handles
    handle_t *handles = NULL;

    // Read operation and call interface function
    ops_t *op = (ops_t *)malloc(sizeof(ops_t));
    while (read_next_op(input_file, op))
    {
        if (op->type == 'M')
        {
            call_my_malloc(&handles, op, RAM);
        }
        else if (op->type == 'F')
        {
            call_my_free(handles, op, RAM);
        }
        else
        {
            // Should not reach here... validation done in read_next_op()
            fprintf(stderr, "Incorrect operation type in input file\n");
            return -1;
        }
    }

    fclose(input_file);
    fclose(output_file);
    free(op);
    free(RAM);

    printf("%s: Output file: %s\n", __func__, output_filename);
    printf("%s: Bye!\n", __func__);
    return 0;
}

// Read and parse one line
bool read_next_op(FILE *fd, ops_t *op)
{
    char line[MAX_LINE_LEN];
    if (!fgets(line, MAX_LINE_LEN, fd))
        return false;

    char *token;
    char delim[2] = " ";
    char *rest = line;

    token = strtok_r(rest, delim, &rest);
    if (token)
        op->name = *token;
    else
        goto err;

    token = strtok_r(rest, delim, &rest);
    if (token)
        op->numops = atoi(token);
    else
        goto err;

    token = strtok_r(rest, delim, &rest);
    if (token)
        op->type = *token;
    else
        goto err;

    token = strtok_r(rest, delim, &rest);
    if (token)
        op->size = atoi(token);
    // else
    // 	goto err;

    // Request (input file) validation
    if (op->type != 'M' && op->type != 'F')
    {
        fprintf(stderr, "%s: Invalid type in input file.\n", __func__);
        exit(EXIT_FAILURE);
    }
    if (op->numops <= 0)
    {
        fprintf(stderr, "%s: Invalid number in input file.\n", __func__);
        exit(EXIT_FAILURE);
    }
    if (op->type == 'M' && op->size <= 0)
    {
        fprintf(stderr, "%s: Invalid size in input file.\n", __func__);
        exit(EXIT_FAILURE);
    }

    return true;

err:
    fprintf(stderr, "%s: Invalid line in input file.\n", __func__);
    exit(EXIT_FAILURE);
}

// Save operation in list and call my_malloc() accordingly
void call_my_malloc(handle_t **handles, ops_t *op, void *RAM)
{
    bool first = false;

    // Allocate an handle for this operation
    handle_t *new_entry;
    new_entry = (handle_t *)malloc(sizeof(handle_t));
    new_entry->name = op->name;
    new_entry->num_allocs = 0;
    new_entry->addresses = (void **)malloc(sizeof(void *) * (op->numops + 1));
    new_entry->next = NULL;

    // Add to handles list
    handle_t *temp = *handles;
    if (temp == NULL)
    {
        *handles = new_entry;
        first = true;
    }
    else
    {
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = new_entry;
    }

    // For given NumOps, try to allocate memory
    for (int i = 1; i <= op->numops; i++)
    {
        void *x = my_malloc(op->size);
        if (x == NULL || (intptr_t)x == -1)
        {
            // my_malloc() request failed

            if (new_entry != NULL && new_entry->num_allocs == 0)
            {
                free(new_entry->addresses);
                free(new_entry);

                if (temp)
                    temp->next = NULL;
                else
                    *handles = NULL;
            }

            // Print the error to output_file
            fprintf(output_file, "Allocation Error %c\n", op->name);
            break;
        }
        else
        {
            // my_malloc() successful

            // Save the returned pointer
            *(new_entry->addresses + i) = x;
            new_entry->num_allocs += 1;

            // Print to output_file
            if (first)
                fprintf(output_file, "Start of first Chunk %c is: %d\n", op->name, (int)((void *)(*(new_entry->addresses + i)) - RAM));
            else
                fprintf(output_file, "Start of Chunk %c is: %d\n", op->name, (int)((void *)(*(new_entry->addresses + i)) - RAM));
        }
    }
}

// Search for corresponding name in list and call my_free() accordingly
void call_my_free(handle_t *handles, ops_t *op, void *RAM)
{
    handle_t *hp1 = handles;
    while (hp1 != NULL)
    {
        if (hp1->name == op->name)
        {
            // entry found in handle list
            int index = op->numops;
            void *ptr_to_free = *(hp1->addresses + index);
            if (ptr_to_free == NULL)
            {
                fprintf(stderr, "%s: Invalid 'F' request in input file.\n", __func__);
                exit(EXIT_FAILURE);
            }

            my_free(ptr_to_free);

            // The ptr is now freed
            *(hp1->addresses + index) = NULL;
            hp1->num_allocs -= 1;

            // Print to output_file
            fprintf(output_file, "freed object %c at %d\n", op->name, (int)((void *)(ptr_to_free)-RAM));
            break;
        }
        hp1 = hp1->next;
    }
}
