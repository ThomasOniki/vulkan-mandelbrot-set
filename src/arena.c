#include "arena.h"
#include <stdio.h>
#include <stdlib.h>

Arena MAIN_ARENA = {0};

typedef struct {
  uptr begin, end;
} Metadata;

void *arena_alloc_impl(Arena *arena, usize size, usize align) {
  --align;

  arena->top = (arena->top + align) & ~(align);
  uptr ret = arena->top;
  arena->top += size;

  if (arena->top > arena->end) {
    const usize old_size = (usize)(arena->end - arena->begin);
    const usize new_size = old_size * 2 + size + align + sizeof(Metadata);

    Metadata *metadata = malloc(new_size);
    metadata->begin = arena->begin;
    metadata->end = arena->end;

    arena->begin = (uptr)metadata;
    arena->end = arena->begin + new_size;

    arena->top = (uptr)(metadata + 1);
    arena->top = (arena->top + align) & ~(align);
    ret = arena->top;
    arena->top += size;
  }
  return (void *)ret;
}
