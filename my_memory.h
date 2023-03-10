#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include "interface.h"

// Declare your own data structures and functions here...
typedef struct buddy_tree_struct BUDDYTREE;
typedef struct tree_node_struct TREENODE;
typedef struct slab_ptr_struct SLABPTR;
typedef struct slab_descriptor_table_entry_struct SDENTRY;
typedef struct slab_descriptor_table_struct SDTABLE;


////////////////////////////////////////////////////////////////////////////////
//
// Structure     : tree_node_struct
// Description   : a node inside the buddy system tree
//                  
//
// Variables     : root - refernce to the root node of the tree
//               : totalMemSize - total amount of bytes in the tree handles

struct buddy_tree_struct{
    TREENODE* root;
    int totalMemSize;
};


////////////////////////////////////////////////////////////////////////////////
//
// Structure     : tree_node_struct
// Description   : a node inside the buddy system tree
//                  
//
// Variables     : isHole - boolean for whether this node is a hole
//               : isMem - boolean for whether this node contains memory for the user
//               : size - amount of bytes that the node contains
//               : startAddr - starting address of node
//               : parent - refernce to the node's parent in the tree
//               : left - refernce to the node's left child in the tree
//               : right - refernce to the node's right child in the tree
//               : slabBitField - bit field for which parts of slab are allocated

struct tree_node_struct{
    bool isHole;
    bool isMem;
    int size;
    void* startAddr;
    TREENODE* parent;
    TREENODE* left;
    TREENODE* right;
    unsigned int* slabBitField;
};


////////////////////////////////////////////////////////////////////////////////
//
// Structure     : slab_ptr_struct
// Description   : Models a slab
//                  
//
// Variables     : startAddr - the starting address of the slab (pre header)
//               : slabBitMap - an integer array of (1|0)'s modeling which
//                      memory chunks in the slab are in use or holes
//               : next - the next slab in the linked list

struct slab_ptr_struct {
    void* slabStartAddr;
    unsigned int* slabBitMap;
    SLABPTR* next;
};


////////////////////////////////////////////////////////////////////////////////
//
// Structure     : slab_descriptor_table_entry_struct (SDENTRY)
// Description   : models an entry (a slab descriptor) inside a slab descriptor table
//                  
//
// Variables     : type - size of each object inside slab
//               : size - total size of this type of slab in bytes
//               : objTotal - total number of objects of "type" inside slab 
//               : objUsed - current number of objects used inside slab
//               : slabPtr - the head of a linked list of slabs
//               : nextEntry - the entry following this one in the slab descriptor table

struct slab_descriptor_table_entry_struct {
    int type;
    int size;
    int objTotal;
    int objUsed;
    SLABPTR* slabPtr; 
    SDENTRY* nextEntry;
};


////////////////////////////////////////////////////////////////////////////////
//
// Structure     : slab_descriptor_table_struct
// Description   : models a table of slab descriptors
//                  
//
// Variables     : SDTableEntries - array of table entries

struct slab_descriptor_table_struct {
    SDENTRY *headEntry;
};


BUDDYTREE* init_buddy_tree(int memSize, void *startOfMemory);
    // initializes a buddy system tree

SDENTRY* init_sd_entry(void* newSlabStartAddr, int type);
    // initializes a slab descriptor entry for given type

SDTABLE* init_sd_table();
    // initializes a slab descriptor table

SDENTRY* sd_table_search(SDTABLE* sdTable, int type);
    // finds entry in table for given type, returns NULL if no entry exists

void sd_table_insert(SDTABLE* sdTable, SDENTRY* entry);
    // inserts an entry into the slab descriptor table

void sd_table_delete(SDTABLE* sdTable, SDENTRY* entry);
    // deletes an entry from the slab descriptor table
    
void* add_new_memory_to_slab(SDENTRY* entry);
    // returns the address to the first available hole in a slab, returns null if none available

void put_size_in_header(void* startMemBlockAddr, int size);
    // stores the size of the memory block in the header

int get_size_in_header(void* startMemBlockAddr);
    // returns the size of given memory block

void add_slab_to_entry(SDENTRY* entry, void* startAddr);
    // add a slab_ptr to linked list for given slab entry

void remove_slab_from_entry(SDENTRY* entry, SLABPTR* slab);
    // removes a slab from an entry in the slab descriptor table

TREENODE* create_new_memory_node(BUDDYTREE* buddyTree, int chunkSize);
    // creates a new node in the tree containing chunkSize memory

TREENODE* find_placement_node(TREENODE* node, int chunkSize);
    // finds the smallest leftmost hole that is greater than or equal to requested size

TREENODE* split_node(TREENODE* node, int chunkSize);
    // splits a hole until there is a hole of given size

TREENODE* find_node_by_address(TREENODE* node, void *ptr);
    // find the corresponding tree node given its start address

void check_for_merge(TREENODE* node, int memSize);
    // if both childs of the node are holes, remove the children and set itself as a hole

int next_power_of_two(int num);
    // returns a power of two greater than or equal to given number

#endif
