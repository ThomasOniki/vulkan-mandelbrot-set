#pragma once

#include "common.h"
#include <stdalign.h>
#include <stdbool.h>
#include <string.h>

#define arr_t(elem_t) elem_t *

void *arr_reserve_impl(void *arr, usize size, usize align, usize capacity);
void *arr_resize_impl(void *arr, usize size, usize align, usize len);

usize arr_capacity(void *arr);
usize arr_len(void *arr);

#define arr_reserve(arr, capacity)                                             \
  arr = arr_reserve_impl(arr, sizeof((arr)[0]), alignof((arr)[0]), capacity)

#define arr_resize(arr, capacity)                                              \
  arr = arr_resize_impl(arr, sizeof((arr)[0]), alignof((arr)[0]), capacity)

#define arr_push(arr, elem)                                                    \
  do {                                                                         \
    arr_resize(arr, arr_len(arr) + 1);                                         \
    (arr)[arr_len(arr) - 1] = elem;                                            \
  } while (false)

#define arr_push_elems(arr, elems, count)                                      \
  do {                                                                         \
    arr_resize(arr, arr_len(arr) + (count));                                   \
    memcpy((arr) + arr_len(arr) - (count), elems,                              \
           (count) * sizeof((elems)[0]));                                      \
  } while (false)

#define arr_push_carr(arr, carr)                                               \
  arr_push_elems(arr, carr, sizeof(carr) / sizeof((carr)[0]))

#define arr_pop(arr) arr_resize(arr, arr_len(arr) - 1)

#define arr_clear(arr) arr_resize(arr, 0)
