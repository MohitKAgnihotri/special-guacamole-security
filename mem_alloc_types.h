#ifndef   	_MEM_ALLOC_TYPES_H_
#define   	_MEM_ALLOC_TYPES_H_

/* Structure declaration for a free block */
typedef struct mem_free_block{
    //size of current block
    size_t size;
    //next block
    struct mem_free_block * next;
    //previous block
    struct mem_free_block * prev;
} mem_free_block_t; 


/* Specific metadata for used blocks */
typedef struct mem_used_block{
    size_t size;
} mem_used_block_t;

typedef mem_free_block_t* (*get_free_block)(size_t size);

#endif
