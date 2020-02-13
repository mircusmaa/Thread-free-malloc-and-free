#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>

struct segment{
  size_t size;
  struct segment* free_next;
  struct segment* free_prev;
  char first_byte;
};
typedef struct segment space;

void* ts_malloc_lock(size_t size);

void ts_free_lock(void *ptr);

void* ts_malloc_nolock(size_t size);

void ts_free_nolock(void *ptr);
