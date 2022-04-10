#include "mvalloc.h"
#include <stdio.h>
#include <string.h>
#include "mem_alloc_types.h"

/* pointer to the beginning of the memory region to manage */
void *mvalloc_heap_pointer;

/* Pointer to the first free block in the heap */
mem_free_block_t *mvalloc_free_list_pointer;

/* Function pointer to the get_free_block function*/
static get_free_block pf_get_free_block_t;

// Currently active algorithm
enum ALGORITHM active_algorithm;

mem_free_block_t *next_free_next_fit;

mem_free_block_t *get_free_block_first_fit(size_t requested_size) {
  mem_free_block_t *p_free_block = mvalloc_free_list_pointer;
  //check every next position while don't find enough one which is free
  while (p_free_block
      && !((p_free_block->size + MIN_FREE_BLOCK_SIZE)
          >= MAX(requested_size + MIN_USED_BLOCK_SIZE, MIN_FREE_BLOCK_SIZE)))
    p_free_block = p_free_block->next;

  return p_free_block;
}

mem_free_block_t *get_free_block_best_fit(size_t requested_size) {
  mem_free_block_t *tmp = mvalloc_free_list_pointer;
  mem_free_block_t *best_one = NULL;

  while (tmp) {
    if (tmp->size + MIN_FREE_BLOCK_SIZE >= MAX(requested_size + MIN_USED_BLOCK_SIZE, MIN_FREE_BLOCK_SIZE)) {
      if (!best_one || tmp->size < best_one->size)
        best_one = tmp;
    }
    tmp = tmp->next;
  }

  return best_one;
}

mem_free_block_t *get_free_block_worst_fit(size_t requested_size) {
  mem_free_block_t *p_iterator = mvalloc_free_list_pointer;
  mem_free_block_t *worst_fit_block = NULL;

  while (p_iterator) {
    if (p_iterator->size + MIN_FREE_BLOCK_SIZE >= MAX(requested_size + MIN_USED_BLOCK_SIZE, MIN_FREE_BLOCK_SIZE)) {
      if (!worst_fit_block || p_iterator->size > worst_fit_block->size)
        worst_fit_block = p_iterator;
    }
    p_iterator = p_iterator->next;
  }

  return worst_fit_block;
}

mem_free_block_t *get_free_block_next_fit(size_t requested_size) {
  mem_free_block_t *p_iterator = next_free_next_fit;
  int flag = 0;

  while (p_iterator
      && !(p_iterator->size + MIN_FREE_BLOCK_SIZE >= MAX(requested_size + MIN_USED_BLOCK_SIZE, MIN_FREE_BLOCK_SIZE))) {
    if (flag && (unsigned char *) p_iterator == (unsigned char *) next_free_next_fit)
      return NULL;

    if (p_iterator->next)
      p_iterator = p_iterator->next;
    else {
      flag = 1;
      p_iterator = mvalloc_free_list_pointer;
    }
  }

  return p_iterator;
}

void insert_free_block(mem_free_block_t *new_b, mem_free_block_t *left, mem_free_block_t *right) {
  if (right)
    right->prev = new_b;
  new_b->next = right;
  new_b->prev = left;
  if (left)
    left->next = new_b;
  else
    mvalloc_free_list_pointer = new_b;
}

void delete_free_block(mem_free_block_t *block) {
  if (!block->prev)
    mvalloc_free_list_pointer = block->next;
  else
    block->prev->next = block->next;

  if (block->next)
    block->next->prev = block->prev;
}

void split_memory_block(mem_free_block_t *current, size_t desired_size) {
  mem_free_block_t *new_b =
      (mem_free_block_t *) ((unsigned char *) current + MAX(MIN_USED_BLOCK_SIZE + desired_size, MIN_FREE_BLOCK_SIZE));
  new_b->size = current->size - MAX(MIN_USED_BLOCK_SIZE + desired_size, MIN_FREE_BLOCK_SIZE);

  insert_free_block(new_b, current, current->next);
  delete_free_block(current);

  if (active_algorithm == NEXT_FIT) {
    next_free_next_fit = new_b;
  }
}

void merge_memory_blocks(mem_free_block_t *block) {
  if (block->prev
      && (unsigned char *) block == (unsigned char *) block->prev + block->prev->size + MIN_FREE_BLOCK_SIZE) {
    block->prev->size += MIN_FREE_BLOCK_SIZE + block->size;

    delete_free_block(block);

    block = block->prev;
  }

  if (block->next && (unsigned char *) block->next == (unsigned char *) block + block->size + MIN_FREE_BLOCK_SIZE) {
    block->size += MIN_FREE_BLOCK_SIZE + block->next->size;
    if (active_algorithm == NEXT_FIT) {
      if ((unsigned char *) block->next == (unsigned char *) next_free_next_fit)
        next_free_next_fit = block;
    }
    delete_free_block(block->next);
  }
}

void mavalloc_init(size_t size, enum ALGORITHM algorithm) {
  // update size by minimum free block size
  size += MIN_FREE_BLOCK_SIZE;
  mvalloc_free_list_pointer = malloc(size);
  mvalloc_free_list_pointer->size = size - MIN_FREE_BLOCK_SIZE;
  mvalloc_free_list_pointer->next = NULL;
  mvalloc_free_list_pointer->prev = NULL;

  if (algorithm == NEXT_FIT) {
    next_free_next_fit = mvalloc_free_list_pointer;
  }

  mvalloc_heap_pointer = mvalloc_free_list_pointer;

  if (algorithm == FIRST_FIT)
    pf_get_free_block_t = get_free_block_first_fit;
  else if (algorithm == BEST_FIT)
    pf_get_free_block_t = get_free_block_best_fit;
  else if (algorithm == NEXT_FIT)
    pf_get_free_block_t = get_free_block_next_fit;
  else if (algorithm == WORST_FIT)
    pf_get_free_block_t = get_free_block_worst_fit;

  active_algorithm = algorithm;
}

void *mavalloc_alloc(size_t size) {

  mem_free_block_t *new_b = pf_get_free_block_t(size);

  if (!new_b) {
    fprintf(stderr, "%s", "Sufficiently large block cannot be allocated\n");
    exit(0);
  }
  mem_used_block_t *used_b = (mem_used_block_t *) new_b;

  if (new_b->size >= MAX(MIN_USED_BLOCK_SIZE + size, MIN_FREE_BLOCK_SIZE)) {
    split_memory_block(new_b, size);
    used_b->size = size;
  } else {
    delete_free_block(new_b);

    used_b->size = new_b->size + MIN_FREE_BLOCK_SIZE - MIN_USED_BLOCK_SIZE;

    if (active_algorithm == NEXT_FIT) {
      mem_free_block_t *prev_next = new_b->next;

      if (prev_next)
        next_free_next_fit = prev_next;
      else if (mvalloc_free_list_pointer)
        next_free_next_fit = mvalloc_free_list_pointer;
      else
        next_free_next_fit = NULL;
    }

  }

  used_b++;
  return used_b;
}

void mavalloc_free(void *p) {
  mem_free_block_t *new_b = (mem_free_block_t *) ((unsigned char *) p - MIN_USED_BLOCK_SIZE);

  new_b->size = MAX(mavalloc_get_block_size(p) + MIN_USED_BLOCK_SIZE, MIN_FREE_BLOCK_SIZE) - MIN_FREE_BLOCK_SIZE;
  new_b->prev = NULL;
  new_b->next = NULL;

  mem_free_block_t *tmp = mvalloc_free_list_pointer;
  mem_free_block_t *prev = NULL;
  while (tmp && tmp < new_b) {
    prev = tmp;
    tmp = tmp->next;
  }

  if (tmp) {
    insert_free_block(new_b, tmp->prev, tmp);
  } else {
    if (mvalloc_free_list_pointer)
      insert_free_block(new_b, prev, NULL);
    else {
      new_b->next = NULL;
      new_b->prev = NULL;
      mvalloc_free_list_pointer = new_b;
      if (active_algorithm == NEXT_FIT) {
        next_free_next_fit = mvalloc_free_list_pointer;
      }
    }
  }

  merge_memory_blocks(new_b);
}

size_t mavalloc_get_block_size(void *addr) {
  mem_used_block_t *used_b = (mem_used_block_t *) ((unsigned char *) addr - MIN_USED_BLOCK_SIZE);
  return used_b->size;
}

#if 0
int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "%s", "Wrong number of arguments\n");
    exit(0);
  }

  size_t size = atoi(argv[1]);
  enum ALGORITHM algorithm = atoi(argv[2]);


  /*Initialize the memory for the my_malloc to fuction*/
  mavalloc_init(ALIGN4(size), algorithm);

  fprintf(stderr, "Memory : [%lu %lu] (%lu bytes)\n", (long unsigned int) mvalloc_heap_pointer,
          (long unsigned int) (mvalloc_heap_pointer + MEMORY_SIZE), (long unsigned int) (MEMORY_SIZE));

  int i;
  for (i = 0; i < 10; i++) {
    char *b = my_malloc_alloc(rand() % 8);
    my_malloc_free(b);
  }

  char *a = my_malloc_alloc(1025);
  my_malloc_free(a);

  a = my_malloc_alloc(10);
  my_malloc_free(a);

  fprintf(stderr, "%lu\n", (long unsigned int) (my_malloc_alloc(9)));
  return EXIT_SUCCESS;
}
#endif

void mavalloc_destroy(void) {
  free(mvalloc_heap_pointer);
}

int mavalloc_size(void) {
  int number_of_blocks = 0;
  mem_free_block_t *p_iterator = mvalloc_free_list_pointer;
  while (p_iterator)
  {
    number_of_blocks++;
    p_iterator = p_iterator->next;
  }
  return number_of_blocks;
}
