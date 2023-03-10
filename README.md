# Memory-Allocator
A CMPSC 473 project to implement a memory allocator that supports both Buddy Allocation and Slab Allocation schemes.


### Overview
You will implement a memory allocator which supports Buddy Allocation and Slab Allocation schemes. The functioning/algorithm of each of these schemes has been discussed in class. Your memory allocator will support my_malloc() and my_free(), which is analogous to the C library's malloc() and free(). To test your memory allocator, the program will call my_malloc() and my_free(), based on the input file. The input file has a list of memory allocation requests and memory free requests. The generated output file has the result of your allocations. Your memory allocator will allocate memory only within the given starting address and size provided in my_setup().


### Hardware Model
This machine is a single CPU system. Only one thread will make requests to your memory allocator.


### Allocation Policies
The size parameter is given only in my_malloc and not in my_free. However, it is necessary to know the size of what is being freed. This should be handled by using the header of the allocated segment to contain the size. However, programs would assume that all the space, starting from the first byte of the returned pointer, is available to be used. Hence, you should ensure that the pointer that my_malloc() returns points to the byte immediately after these HEADER_SIZE bytes (header) containing the size.

#### Buddy Allocator:
- The minimum chunk of memory that the Buddy System can allocate is MIN_MEM_CHUNK_SIZE bytes defined at interface.h.
- Maintain the free (holes) list sorted by increasing addresses individually for different sizes (of powers of 2) as discussed in class.
- Whenever there is a choice of multiple holes that a scheme allows, always pick the hole with the lowest starting address.
- The HEADER_SIZE byte header is used to maintain the size of the allocated chunk within the chunk itself.

#### Slab Allocator:
- The number of objects per slab is fixed at N_OBJS_PER_SLAB defined at interface.h. Note that each slab will only contain the objects of the appropriate type/size. However the size of the allocated object itself should be accounted to include its HEADER_SIZE byte header. Hence, when using this scheme for allocating an object, there will be a HEADER_SIZE byte header for the object, and additionally a HEADER_SIZE byte header in the slab itself (where this object resides) which is allocated using Buddy.
- The Slab Descriptor Table itself is maintained as a separate data structure (you can use the c library malloc() for allocating it). Please use the data structure explained in class for the Slab Descriptor Table.
- Note that whenever you need to create a new slab, you need to call your Buddy Allocator from above to get the required slab (as discussed in class).


### Additional Implementation Requirements/Restrictions
- You cannot use additional libraries.
- You cannot modify main.c file.
- Write a brief report (pdf of around 2 pages) explaining the design of your memory allocator, any implementation choices/restrictions, any functions not implemented, things learned, distribution of the workload within the team, etc. Commit and push the report with the code into your repo before the deadline.