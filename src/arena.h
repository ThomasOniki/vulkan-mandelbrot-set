#include "common.h"

typedef struct {
  uptr begin, top, end;
} Arena;

void *arena_alloc_impl(Arena *arena, usize size, usize align);

#define arena_alloc(arena, type, count)                                        \
  arena_alloc_impl(arena, sizeof(type) * count, alignof(type))

extern Arena MAIN_ARENA;
