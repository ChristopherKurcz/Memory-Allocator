#include "interface.h"
#include "my_memory.h"

// Interface implementation
// Implement APIs here...

// Global Data
enum malloc_type policy;
BUDDYTREE* buddyTree;
SDTABLE* sdTable;


void my_setup(enum malloc_type type, int mem_size, void *start_of_memory)
{
    // initialize global variables
    policy = type;
    buddyTree = init_buddy_tree(mem_size, start_of_memory);
    sdTable = init_sd_table();
}


void *my_malloc(int size)
{
    switch (policy)
    {
    case MALLOC_SLAB: ;
        // initialize local variables
        void* memAddr;
        int objSize = (HEADER_SIZE + size);

        // check to see if we have a slab descriptor entry in table for this size
        SDENTRY* sdEntry = sd_table_search(sdTable, objSize);

        // if an entry was found, try to add the new object to the the slab
        if (sdEntry != NULL){
            memAddr = add_new_memory_to_slab(sdEntry);

            // if the slab is not full, return the found address
            if (memAddr != NULL){
                put_size_in_header(memAddr, size);
                return memAddr;
            }
        }

        // slab size is made up of slab header and N objs including their headers all rounded up
        int slabSize = next_power_of_two(HEADER_SIZE + (objSize) * N_OBJS_PER_SLAB);

        // create a new node in the tree containing the memory for the new slab
        TREENODE* newSlabNode = create_new_memory_node(buddyTree, slabSize);

        if (newSlabNode == NULL){
            return NULL; // should return -1 here
        }

        // check whether a new slab entry for the slab descriptor table must be created or not
        if (sdEntry == NULL){
            // since there is no entry in the table for slabs of type objSize, create one and add it to the table
            sdEntry = init_sd_entry(newSlabNode->startAddr, objSize);
            sd_table_insert(sdTable, sdEntry);
        } else {
            // since there is already an entry in the table for slabs of type objSize, jsut add the new slab to the entry
            add_slab_to_entry(sdEntry, newSlabNode->startAddr);
        }

        // allocate a spot of memory in the slab
        memAddr = add_new_memory_to_slab(sdEntry);

        put_size_in_header(memAddr, size);
        return memAddr;

    case MALLOC_BUDDY: ;
        // find out how big of a chunk user will need
        int chunkSize = next_power_of_two(size + HEADER_SIZE);

        // add a new node to the tree containing chunksize memory
        TREENODE* newNode = create_new_memory_node(buddyTree, chunkSize);

        if (newNode == NULL){
            return NULL; // should return -1 here
        }

        // place the size of the user's memory in the header bytes
        put_size_in_header(newNode->startAddr + HEADER_SIZE, size);

        // return the start of the user's usable memory
        return newNode->startAddr + HEADER_SIZE;

    default:
        break;
    }
}


void my_free(void *ptr)
{
    switch (policy)
    {
    case MALLOC_SLAB: ;
        // Retrieve information about what we're trying to free
        int type = get_size_in_header(ptr);
        int objectSize = (HEADER_SIZE + type);
        SDENTRY* entry = sd_table_search(sdTable, objectSize);
        SLABPTR* slab = NULL;
        int slabSize = HEADER_SIZE + (objectSize) * N_OBJS_PER_SLAB;
        if(entry == NULL) {
            return;
        }
        // Traverse the slabs looking for which contains the chunk we want to free
        SLABPTR* travPointer = entry->slabPtr;
        while(travPointer != NULL) {
        void* slabStartAddress = travPointer->slabStartAddr;
            if( (ptr > slabStartAddress) && (ptr < slabStartAddress + slabSize) ) {
                slab = entry->slabPtr;
                break;
            }
            travPointer = travPointer->next;
        }
        // If there none of our slabs contain what were trying to free
        if(slab == NULL) {
            return;
        }

        int slabBitMapIndex = -1;
        // Iterate through slab until we find the index of the address we are trying to free
        for(int i = 0; i < N_OBJS_PER_SLAB; i++) {
            void* chunkAddr = ( slab->slabStartAddr + 2 * HEADER_SIZE + (i*objectSize) );
            if(chunkAddr == ptr) {
                slabBitMapIndex = i;
                break;
            }
        }
        // This shouldnt happen, but just in case
        if(slabBitMapIndex == -1) {
            return;
        }

        // Flip the bit to a 0 to represent it as a hole
        slab->slabBitMap[slabBitMapIndex] = 0;

        // Iterate through slabs bit map to check to see if slab is now empty
        bool emptySlab = true;
        for(int i = 0; i < N_OBJS_PER_SLAB; i++) {
            if(slab->slabBitMap[i] == 1) {
                emptySlab = false;
                break;
            }
        }

        // If slab is now empty, remove it from slab linked list and update hole tree
        if(emptySlab) {
            TREENODE* removalNode = find_node_by_address(buddyTree->root, slab->slabStartAddr);
            remove_slab_from_entry(entry, slab);
            removalNode->isMem = false;
            removalNode->isHole = true;
            check_for_merge(removalNode->parent, buddyTree->totalMemSize);
        }

        // If there are no longer any slabs corresponing to the type of the memory we freed, delete entry in table
        if(entry->slabPtr == NULL) {
            sd_table_delete(sdTable, entry);
        }
        break;
    
    case MALLOC_BUDDY: ;
        // find the node in the tree trying to be removed
        TREENODE* removalNode = find_node_by_address(buddyTree->root, ptr - HEADER_SIZE);

        // set the node as a hole
        removalNode->isMem = false;
        removalNode->isHole = true;

        // merge any holes next to each other in the tree
        check_for_merge(removalNode->parent, buddyTree->totalMemSize);

        break;

    default:
        break;
    }
}
