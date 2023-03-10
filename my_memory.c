#include "my_memory.h"

// Memory allocator implementation
// Implement all other functions here...


////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_buddy_tree
// Description  : initializes a buddy system tree
//                  
//
// Inputs       : memSize - total amount of memory the tree is meant to manage
//              : startOfMemory - void pointer of the start of the memory the tree is meant to manage
// Outputs      : BUDDYTREE instance that is created from the parameters

BUDDYTREE* init_buddy_tree(int memSize, void *startOfMemory){
    // initialize a root node as a hole of (memSize) size
    TREENODE* rootNode = malloc(sizeof(TREENODE));
    rootNode->isHole = true;
    rootNode->isMem = false;
    rootNode->size = memSize;
    rootNode->startAddr = startOfMemory;
    rootNode->parent = NULL;
    rootNode->left = NULL;
    rootNode->right = NULL;

    // initialize the buddy system tree
    BUDDYTREE* buddySystemTree = malloc(sizeof(BUDDYTREE));
    buddySystemTree->root = rootNode;
    buddySystemTree->totalMemSize = memSize;

    // return the tree
    return buddySystemTree;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_sd_table
// Description  : initializes an empty slab descriptor table
//                  
//
// Inputs       : None
// Outputs      : SDTABLE instance

SDTABLE* init_sd_table() {
    SDTABLE* sdTable = malloc(sizeof(SDTABLE));
    sdTable->headEntry = NULL;
    return sdTable;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_sd_entry
// Description  : initializes and returns an entry for the slab descriptor table of "type" size
//                  
//
// Inputs       : newSlabStartAddr - start address of the slab for the new slab entry
//              : type - the key that is used for lookups in the SDT "type" represents size of each chunk of memory in a slab
// Outputs      : SDENTRY instance

SDENTRY* init_sd_entry(void* newSlabStartAddr, int type) {
    unsigned int* bitMap; // initializes array of ints repr. each slot in slab init. to 0
    bitMap = calloc(N_OBJS_PER_SLAB, sizeof(unsigned int));

    SLABPTR* slabPtr = malloc(sizeof(SLABPTR));
    slabPtr->slabStartAddr = newSlabStartAddr;
    slabPtr->slabBitMap = bitMap;
    slabPtr->next = NULL;
    
    SDENTRY* sdEntry = malloc(sizeof(SDENTRY));
    sdEntry->type = type;
    sdEntry->objTotal = N_OBJS_PER_SLAB;
    sdEntry->objUsed = 0;
    sdEntry->size = type * N_OBJS_PER_SLAB;
    sdEntry->slabPtr = slabPtr;
    sdEntry->nextEntry = NULL;

    return sdEntry;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : sd_table_search
// Description  : searches for an entry in the slab descriptor table with a given type
//                  
//
// Inputs       : sdTable - an instance of a slab descriptor table
//              : type - the type of entry that we are looking for
// Outputs      : SDENTRY instance matching given type

SDENTRY* sd_table_search(SDTABLE* sdTable, int type) {
    SDENTRY* travPointer = sdTable->headEntry;

    // if the table is empty
    if(travPointer == NULL) {
        return NULL;
    }
    // if the head of the table is our target
    else if(travPointer->type == type) {
        return travPointer;
    } else {
        // traverse linked list until the next entry is either null (we reached end) or our target
        while( (travPointer->nextEntry != NULL) && (travPointer->nextEntry->type != type) ) {
            travPointer = travPointer->nextEntry;
        }
        // will return target entry if it was found, else it will return null (thus entry is not in table)
        return travPointer->nextEntry;
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : sd_table_insert
// Description  : inserts a given entry into the given slab descriptor table
//                  
//
// Inputs       : sdTable - an instance of a slab descriptor table
//              : entry - an instance of a slab descriptor entry
// Outputs      : None

void sd_table_insert(SDTABLE* sdTable, SDENTRY* entry) {
    SDENTRY* travPointer = sdTable->headEntry;
    // if the table is empty
    if(sdTable->headEntry == NULL) {
        sdTable->headEntry = entry;
    } else {
        // traverse linked list until we reach the end and then insert entry
        while(travPointer->nextEntry != NULL) {
            travPointer = travPointer->nextEntry;
        }
        travPointer->nextEntry = entry;
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : sd_table_delete
// Description  : searches for and deletes an entry in the slab descriptor table with a given type
//                  
//
// Inputs       : sdTable - an instance of a slab descriptor table
//              : entry - an instance of a slab descriptor entry
// Outputs      : None

void sd_table_delete(SDTABLE* sdTable, SDENTRY* entry) {
    if(sdTable->headEntry == entry) {
        sdTable->headEntry = sdTable->headEntry->nextEntry;
        free(entry);
        return;
    }

    SDENTRY* travPointer = sdTable->headEntry;
    while(travPointer->nextEntry != entry) {
        travPointer = travPointer->nextEntry;
    }
    travPointer->nextEntry = travPointer->nextEntry->nextEntry;
    free(entry);

}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : add_new_memory_to_slab
// Description  : given an entry, searches through the slabs in the entry for a hole and allocates it
//                  
//
// Inputs       : entry - an instacne of SDENTRY
// Outputs      : returns a pointer to the allocated address
//              : NULL if there was no open spots in the slabs inside the given slab entry

void* add_new_memory_to_slab(SDENTRY* entry) {
    void* openSlabAddr = NULL;
    SLABPTR* currentSlabPtr = entry->slabPtr;
    // looping over all the allocated slabPtr's (eg. all the slabs of a given entry)
    while(currentSlabPtr != NULL) {
        // looping over the integers in a specific slabPtr's bitmap
        for(int i=0; i < N_OBJS_PER_SLAB; i++) {
            // if an element in bitmap is 0, we have an open space
            if(currentSlabPtr->slabBitMap[i] == 0) {
                // flip the bit because it will now be used
                currentSlabPtr->slabBitMap[i] = 1;
                // must calculate address differently if its the first in array
                if(i == 0) {
                    return currentSlabPtr->slabStartAddr + 2 * HEADER_SIZE;
                } else {
                    return currentSlabPtr->slabStartAddr + 2 * HEADER_SIZE + (i * entry->type);
                }
            }
        }
        // If the for loop doesnt return, go to next slab
        currentSlabPtr = currentSlabPtr->next;
    }
    // if we exit while loop, there are no open slots in the slab, return NULL
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_size_in_header
// Description  : given a memory address to a memory block, stores given size in blocks header
//                  
//
// Inputs       : startMemBlockAddr - the start address to a block of memory
//              : size - the size of the memory block to be stored in the header
// Outputs      : None

void put_size_in_header(void* startMemBlockAddr, int size) {
    // Use pointer arithmetic to traverse to the start of the the header
    void* headerStart = (char*)startMemBlockAddr - HEADER_SIZE;
    // Store the size of the memBLock in the first 4 bytes of the header
    *(int*)headerStart = size;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_size_in_header
// Description  : given the start address to a block of memory, returns the size of the block
//                  
//
// Inputs       : startMemBlockAddr - the start address of requested memory block
// Outputs      : returns an integer value of the size of the memory block

int get_size_in_header(void* startMemBlockAddr) {
    // Use pointer arithmetic to traverse to the start of the header
    void* headerStart = (char*)startMemBlockAddr - HEADER_SIZE;
    return *(int*)headerStart;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : add_slab_to_entry
// Description  : given an entry to the slab descriptor table, initializes a new slab
//                  and adds it the end of the entry's slab linked list
//                  
//
// Inputs       : entry - an entry for the slab descriptor table
//              : startAddr - the address that the new slab starts at
// Outputs      : None

void add_slab_to_entry(SDENTRY* entry, void* startAddr) {
    SLABPTR* newSlab = malloc(sizeof(SLABPTR));
    SLABPTR* travPointer = entry->slabPtr;
    unsigned int* bitMap;
    bitMap = calloc(N_OBJS_PER_SLAB, sizeof(unsigned int));
    newSlab->next = NULL;
    newSlab->slabBitMap = bitMap;
    newSlab->slabStartAddr = startAddr;

    // traverse to end of the linked list of slabs
    while(travPointer->next != NULL) {
        travPointer = travPointer->next;
    }

    travPointer->next = newSlab;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : remove_slab_from_entry
// Description  : given an entry and a slab instance, removes the slab instance
//                  from the entry's slab linked list
//                  
//
// Inputs       : entry - an entry in the slab descriptor table
//              : slab - a slab in the entry's linked list
// Outputs      : None

void remove_slab_from_entry(SDENTRY* entry, SLABPTR* slab) {
    if(entry->slabPtr == slab) {
        entry->slabPtr = entry->slabPtr->next;
        free(slab);
        return;
    }
    SLABPTR* travPointer = entry->slabPtr;
    while(travPointer->next != slab) {
        travPointer = travPointer->next;
    }
    travPointer->next = travPointer->next->next;
    free(slab);
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : create_new_memory_node
// Description  : creates a new node in the tree containing chunkSize memory
//                  
//
// Inputs       : buddyTree - tree to make the new node in
//              : chunkSize - size of requested node
// Outputs      : TREENODE instance of the new node containing chunkSize memory
//              : NULL if unsuccessful

TREENODE* create_new_memory_node(BUDDYTREE* buddyTree, int chunkSize){
    TREENODE* placementNode = find_placement_node(buddyTree->root, chunkSize);

    if (placementNode == NULL){
        return NULL;
    }

    if (placementNode->size > chunkSize){
        placementNode = split_node(placementNode, chunkSize);
    }

    placementNode->isHole = false;
    placementNode->isMem = true;

    return placementNode;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : find_placement_node
// Description  : finds the smallest leftmost hole that is greater than or 
//                equal to requested size (let's call it the "placement node")
//                  
//
// Inputs       : node - TREENODE instance of starting node to find a placement node from
//              : chunkSize - size of requested hole
// Outputs      : TREENODE instance of the smallest leftmost hole that is greater than or equal to requested size
//              : NULL if there is no such node

TREENODE* find_placement_node(TREENODE* node, int chunkSize){
    // this shouldnt ever pass, since every node in the tree will always have two children or be a leaf
    if (node == NULL){
        return NULL;
    }

    // if the node is a memory leaf node, return NULL, since nothing can be placed here
    if (node->isMem == true){
        return NULL;
    }

    // if the node is a hole that would fit the required size, return itself
    if ((node->isHole == true) && (node->size >= chunkSize)){
        return node;
    }

    // do not go deeper in the tree than the required chunk size
    if (node->size <= chunkSize){
        return NULL;
    }

    // find a placement node for each child
    TREENODE* leftPlacement = find_placement_node(node->left, chunkSize);
    TREENODE* rightPlacement = find_placement_node(node->right, chunkSize);

    // in the case where both sides find a placement hole, return the smaller 
    if ((leftPlacement != NULL) && (rightPlacement != NULL)){
        if (leftPlacement->size <= rightPlacement->size){
            return leftPlacement;
        } else {
            return rightPlacement;
        }
    }

    if (leftPlacement != NULL){
        return leftPlacement;
    }

    if (rightPlacement != NULL){
        return rightPlacement;
    }

    // no placement node found, return null
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : split_node
// Description  : splits a hole until there is a hole of given size
//                  
//
// Inputs       : node - TREENODE instance the hole to split
//              : chunkSize - size of hole wanted
// Outputs      : TREENODE instance of the hole with size requested

TREENODE* split_node(TREENODE* node, int chunkSize){
    // if the node is already requested size, return it
    if (node->size == chunkSize){
        return node;
    }

    // initialize left child
    TREENODE* leftChild = malloc(sizeof(TREENODE));
    leftChild->isHole = true;
    leftChild->isMem = false;
    leftChild->size = (node->size / 2);
    leftChild->startAddr = node->startAddr;
    leftChild->parent = node;
    leftChild->left = NULL;
    leftChild->right = NULL;

    // initialize right child
    TREENODE* rightChild = malloc(sizeof(TREENODE));
    rightChild->isHole = true;
    rightChild->isMem = false;
    rightChild->size = (node->size / 2);
    rightChild->startAddr = node->startAddr + (node->size / 2);
    rightChild->parent = node;
    rightChild->left = NULL;
    rightChild->right = NULL;

    // set node's children to initialized nodes, and set itself as no longer a hole
    node->left = leftChild;
    node->right = rightChild;
    node->isHole = false;

    // make the left child split further until it reaches requested size
    return split_node(node->left, chunkSize);
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : find_node_by_address
// Description  : given the start address of a node, find the corresponding tree node (let's call it the removal node)
//                  
//
// Inputs       : node - TREENODE instance of starting node to try to find the node from
//              : ptr - pointer to the node's start address
// Outputs      : TREENODE instance that starts at the given start adress
//              : NULL null if there is no such node

TREENODE* find_node_by_address(TREENODE* node, void *ptr){
    // this shouldnt ever pass, since every node in the tree will always have two children or be a leaf
    if (node == NULL){
        return NULL;
    }

    // a node is a leaf, check if it is the one we are looking for
    if (node->isHole || node->isMem){
        if (node->startAddr == ptr){
            return node;
        } else {
            return NULL;
        }
    }

    // look for the node in both children
    TREENODE* leftRemovalNode = find_node_by_address(node->left, ptr);
    TREENODE* rightRemovalNode = find_node_by_address(node->right, ptr);

    if (leftRemovalNode != NULL){
        return leftRemovalNode;
    }

    if (rightRemovalNode != NULL){
        return rightRemovalNode;
    }

    // if the node is not found from starting node, return null
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : check_for_merge
// Description  : if both childs of the node are holes, remove the children and set itself as a hole
//                  
//
// Inputs       : node - TREENODE instance of a potential merged hole
//              : memSize - the memory's total size (also the maximum size a hole can be)
// Outputs      : none

void check_for_merge(TREENODE* node, int memSize){
    // if the node contains either a hole or memory, stop trying to merge it
    if (node->isMem || node->isHole){
        return;
    }

    // only continue with the merge if both children are holes
    if (!node->left->isHole || !node->right->isHole){
        return;
    }

    // remove children and set itself as a hole
    free(node->left);
    node->left = NULL;
    free(node->right);
    node->right = NULL;
    node->isHole = true;

    // if the new hole is size of the total memory, do not merge any more
    if (node->size == memSize){
        return;
    }

    // now that this node is a hole, make the node's parent check if it can merge its children
    check_for_merge(node->parent, memSize);
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : next_power_of_two
// Description  : returns a power of two greater than or equal to given number
//                  
//
// Inputs       : num - integer value
// Outputs      : integer value of a power of two greater than or equal to num

int next_power_of_two(int num){
    int result = MIN_MEM_CHUNK_SIZE;
    while(num > result){
        result = result * 2;
    }
    return result;
}
