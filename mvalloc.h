#ifndef   	_MV_ALLOC_H_
#define   	_MV_ALLOC_H_

#include <stdlib.h>

#define MEM_ALIGNMENT 8
#define MEMORY_SIZE 1024

#define ALIGN4(s) (((((s) - 1) >> 2) << 2) + 4)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN_FREE_BLOCK_SIZE (sizeof(mem_free_block_t))
#define MIN_USED_BLOCK_SIZE (sizeof(mem_used_block_t))

enum ALGORITHM
{
  FIRST_FIT = 0,
  NEXT_FIT = 1,
  BEST_FIT = 2,
  WORST_FIT = 3,
};

void mavalloc_init(size_t size, enum ALGORITHM algorithm);
void *mavalloc_alloc(size_t size);
void mavalloc_free(void *pointer);
size_t mavalloc_get_block_size(void *addr);
void mavalloc_destroy( );
int mavalloc_size( );

#endif 	    /* !_MV_ALLOC_H_ */
