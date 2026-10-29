#include "array.h"
#include "arena.h"
#include <string.h>

typedef struct {
  usize len, capacity;
} ArrayHeader;

static ArrayHeader *arr_header(void *arr) { return ((ArrayHeader *)arr) - 1; }

usize arr_capacity(void *arr) { return arr ? arr_header(arr)->capacity : 0; }
usize arr_len(void *arr) { return arr ? arr_header(arr)->len : 0; }

void *arr_reserve_impl(void *arr, usize size, usize align, usize capacity) {
  if (!arr) {
    ArrayHeader *header = arena_alloc_impl(
        &MAIN_ARENA, size * capacity + sizeof(ArrayHeader), align);
    header->len = 0;
    header->capacity = capacity;
    arr = header + 1;
  } else if (capacity > arr_capacity(arr)) {
    ArrayHeader *header = arena_alloc_impl(
        &MAIN_ARENA, size * capacity + sizeof(ArrayHeader), align);
    header->len = arr_len(arr);
    header->capacity = capacity;
    arr = memcpy(header + 1, arr, arr_len(arr) * size);
  }
  return arr;
}

void *arr_resize_impl(void *arr, usize size, usize align, usize len) {
  arr = arr_reserve_impl(arr, size, align, len);
  arr_header(arr)->len = len;
  return arr;
}
